#include "Framework/Scene/Core/SceneManager.h"

#include "Framework/Rendering/Core/IRenderResourceLifetime.h"
#include "Framework/Rendering/Core/IRenderer.h"
#include "Framework/Core/Diagnostics/Diagnostics.h"
#include "Framework/Core/Diagnostics/ExceptionUtils.h"

#include <algorithm>
#include <chrono>
#include <utility>
#include <stdexcept>

using namespace DirectX;

namespace
{
	constexpr float MinimumLoadingSeconds = 0.75f;
	constexpr float FadeOutSeconds = 0.45f;
	constexpr float FadeInSeconds = 0.45f;

	std::unique_ptr<IScene> PrepareScene(const SceneManager::SceneFactory& factory)
	{
		auto scene = factory();
		if (!scene) throw std::runtime_error("Scene factory returned null");
		scene->Prepare();
		return scene;
	}

	void UnloadScene(IScene& scene) noexcept
	{
		try { scene.Unload(); }
		catch (...)
		{
			// Cleanup must reach every scene, including during stack unwinding.
			// Formatting the diagnostic can fail independently of the sink.
			try { Diagnostics::Write("Scene unload failed: " + DescribeException()); }
			catch (...) { Diagnostics::Write("Scene unload failed (error details unavailable)"); }
		}
	}
}

SceneManager::~SceneManager()
{
	// Finish CPU preparation before destroying services captured by factories.
	m_pendingLoad.reset();
	ClearActiveScenes();
}

void SceneManager::Initialize(
	IRenderDevice& renderDevice,
	IRenderResourceLifetime& resourceLifetime,
	std::uint32_t width,
	std::uint32_t height)
{
	if (width == 0 || height == 0) throw std::invalid_argument("SceneManager requires a nonzero render size");
	m_resourceLifetime = &resourceLifetime;
	m_width = width;
	m_height = height;
	m_loadingOverlay.Initialize(renderDevice, width, height);
	m_fadeOverlay.Initialize(renderDevice, 1);
}

void SceneManager::RegisterScene(const std::string& name, SceneFactory factory)
{
	m_sceneFactories[name] = std::move(factory);
}

bool SceneManager::LoadScene(const std::string& name, SceneLoadType loadType, SceneLoadMode loadMode)
{
	if (m_pendingLoad != nullptr)
	{
		return false;
	}

	const auto sceneFactory = m_sceneFactories.find(name);
	if (sceneFactory == m_sceneFactories.end())
	{
		RecoverLoadFailure("Scene is not registered: " + name);
		return false;
	}

	m_lastLoadError.clear();
	const char* operation = "preparation";
	try
	{
		if (loadType == SceneLoadType::Synchronous)
		{
			auto scene = PrepareScene(sceneFactory->second);
			operation = "resize";
			scene->OnResize(m_width, m_height);
			operation = "activation";
			scene->Activate();
			operation = "commit";
			CommitLoadedScene(std::move(scene), loadMode);
			return true;
		}

		operation = "queue preparation";
		auto pendingLoad = std::make_unique<PendingLoad>();
		pendingLoad->name = name;
		pendingLoad->mode = loadMode;
		pendingLoad->factory = sceneFactory->second;

		m_pendingLoad = std::move(pendingLoad);
		UpdateFadeOverlay(0.0f);
		return true;
	}
	catch (...) { RecoverLoadException(name, operation); }
	return false;
}

void SceneManager::Resize(std::uint32_t width, std::uint32_t height)
{
	if (width == 0 || height == 0 || (width == m_width && height == m_height)) return;
	m_width = width;
	m_height = height;
	m_loadingOverlay.Resize(width, height);
	UpdateFadeOverlay(m_fadeAlpha);
	for (const auto& scene : m_activeScenes) scene->OnResize(width, height);
	// The pending candidate belongs to Prepare's worker until future.get().
	// Its first notification uses these latest dimensions immediately before activation.
}

void SceneManager::Update(float deltaTime, const Input& input)
{
	if (m_pendingLoad && m_pendingLoad->phase != PendingLoadPhase::FadeIn) return;
	for (const auto& scene : m_activeScenes) scene->Update(deltaTime, input);
}

void SceneManager::UpdateFrame(float deltaTime, const Input& input)
{
	if (m_pendingLoad != nullptr)
	{
		m_pendingLoad->elapsedTime += deltaTime;

		if (m_pendingLoad->phase == PendingLoadPhase::FadeOut)
		{
			const float fadeAlpha = std::min(m_pendingLoad->elapsedTime / FadeOutSeconds, 1.0f);
			UpdateFadeOverlay(fadeAlpha);
			if (m_pendingLoad->elapsedTime >= FadeOutSeconds)
			{
				StartPendingLoad();
			}
			return;
		}

		if (m_pendingLoad->phase == PendingLoadPhase::Loading)
		{
			m_loadingOverlay.Update(deltaTime);
			PollAsyncLoad();
			return;
		}

		for (const std::unique_ptr<IScene>& scene : m_activeScenes)
		{
			scene->UpdateFrame(deltaTime, input);
		}

		const float fadeAlpha = 1.0f - std::min(m_pendingLoad->elapsedTime / FadeInSeconds, 1.0f);
		UpdateFadeOverlay(fadeAlpha);
		if (m_pendingLoad->elapsedTime >= FadeInSeconds)
		{
			m_pendingLoad.reset();
		}
		return;
	}

	std::string requestedSceneName;
	bool shouldLoadAsync = false;
	for (const std::unique_ptr<IScene>& scene : m_activeScenes)
	{
		scene->UpdateFrame(deltaTime, input);
		if (requestedSceneName.empty())
		{
			requestedSceneName = scene->GetRequestedSceneName();
			shouldLoadAsync = scene->ShouldLoadRequestedSceneAsync();
		}
	}

	if (!requestedSceneName.empty())
	{
		const SceneLoadType loadType = shouldLoadAsync ? SceneLoadType::Asynchronous : SceneLoadType::Synchronous;
		LoadScene(requestedSceneName, loadType, SceneLoadMode::Single);
	}
}

void SceneManager::RenderWorld(IRenderer& renderer) const
{
	if (m_pendingLoad != nullptr && m_pendingLoad->phase == PendingLoadPhase::Loading)
	{
		return;
	}

	for (const std::unique_ptr<IScene>& scene : m_activeScenes)
	{
		scene->RenderWorld(renderer);
	}
}

void SceneManager::RenderOverlay(IRenderer& renderer) const
{
	if (m_pendingLoad != nullptr && m_pendingLoad->phase == PendingLoadPhase::Loading)
	{
		m_loadingOverlay.Render(renderer);
		return;
	}

	for (const std::unique_ptr<IScene>& scene : m_activeScenes)
	{
		scene->RenderOverlay(renderer);
	}

	if (m_pendingLoad != nullptr)
	{
		m_fadeOverlay.Render(renderer);
	}
}

RenderView SceneManager::GetRenderView() const
{
	if (m_activeScenes.empty() || (m_pendingLoad && m_pendingLoad->phase == PendingLoadPhase::Loading))
	{
		return {};
	}

	return m_activeScenes.back()->GetRenderView();
}

bool SceneManager::IsLoading() const
{
	return m_pendingLoad != nullptr;
}

void SceneManager::ClearActiveScenes()
{
	for (const std::unique_ptr<IScene>& scene : m_activeScenes)
	{
		UnloadScene(*scene);
	}
	m_activeScenes.clear();
}

void SceneManager::RetireActiveScenes()
{
	for (std::unique_ptr<IScene>& scene : m_activeScenes)
	{
		std::shared_ptr<IScene> retiredScene(std::move(scene));
		if (m_resourceLifetime != nullptr)
		{
			m_resourceLifetime->DeferRelease([retiredScene]()
			{
				UnloadScene(*retiredScene);
			});
		}
		else
		{
			UnloadScene(*retiredScene);
		}
	}
	m_activeScenes.clear();
}

void SceneManager::CommitLoadedScene(std::unique_ptr<IScene> scene, SceneLoadMode mode)
{
	// Allocate the destination slot before retiring any active scene.
	m_activeScenes.reserve(m_activeScenes.size() + 1);
	if (mode == SceneLoadMode::Single)
	{
		RetireActiveScenes();
	}

	m_activeScenes.push_back(std::move(scene));
}

void SceneManager::StartPendingLoad()
{
	// Keep the current scenes alive until CPU preparation and GPU activation
	// both succeed. Peak memory includes one current set and one pending set.
	try
	{
		const SceneFactory factory = m_pendingLoad->factory;
		m_pendingLoad->future = std::async(std::launch::async, [factory]()
		{
			return PrepareScene(factory);
		});
		m_pendingLoad->phase = PendingLoadPhase::Loading;
		m_pendingLoad->elapsedTime = 0.0f;
	}
	catch (...) { RecoverLoadException(m_pendingLoad->name, "start preparation"); }
}

void SceneManager::PollAsyncLoad()
{
	if (m_pendingLoad == nullptr)
	{
		return;
	}

	if (m_pendingLoad->elapsedTime < MinimumLoadingSeconds)
	{
		return;
	}

	const char* operation = "preparation";
	try
	{
		if (m_pendingLoad->future.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
		{
			return;
		}
		std::unique_ptr<IScene> scene = m_pendingLoad->future.get();
		operation = "resize";
		scene->OnResize(m_width, m_height);
		operation = "activation";
		scene->Activate();
		operation = "commit";
		CommitLoadedScene(std::move(scene), m_pendingLoad->mode);
	}
	catch (...) { RecoverLoadException(m_pendingLoad->name, operation); return; }
	m_pendingLoad->phase = PendingLoadPhase::FadeIn;
	m_pendingLoad->elapsedTime = 0.0f;
	UpdateFadeOverlay(1.0f);
}

void SceneManager::RecoverLoadException(const std::string& name, const char* operation)
{
	RecoverLoadFailure("Scene [" + name + "] " + operation + ": " + DescribeException());
}

void SceneManager::RecoverLoadFailure(std::string message)
{
	m_lastLoadError = std::move(message);
	Diagnostics::Write("Scene load failed: " + m_lastLoadError);
	m_pendingLoad.reset();
	for (const auto& scene : m_activeScenes)
	{
		try { scene->OnSceneLoadFailed(); }
		catch (...)
		{
			// A secondary observer failure must not replace the load error or
			// prevent the remaining active scenes from resuming.
			Diagnostics::Write("Scene load failure notification failed: " + DescribeException());
		}
	}
}

void SceneManager::UpdateFadeOverlay(float alpha)
{
	m_fadeAlpha = alpha;
	const XMFLOAT4 color{ 0.0f, 0.0f, 0.0f, alpha };
	m_fadeOverlay.Clear();
	m_fadeOverlay.DrawRectangle(0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height), color);
	m_fadeOverlay.Upload();
}
