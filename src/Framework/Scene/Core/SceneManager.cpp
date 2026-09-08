#include "Framework/Scene/Core/SceneManager.h"

#include "Framework/Rendering/Core/IRenderResourceLifetime.h"
#include "Framework/Rendering/Core/IRenderer.h"
#include "Framework/Core/Diagnostics/Diagnostics.h"

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
	if (loadType == SceneLoadType::Synchronous)
	{
		try
		{
			std::unique_ptr<IScene> scene = sceneFactory->second();
			if (!scene) throw std::runtime_error("Scene factory returned null");
			scene->Prepare();
			scene->Activate();
			CommitLoadedScene(std::move(scene), loadMode);
			return true;
		}
		catch (const std::exception& error) { RecoverLoadFailure(error.what()); }
		catch (...) { RecoverLoadFailure("Unknown scene load failure"); }
		return false;
	}

	auto pendingLoad = std::make_unique<PendingLoad>();
	pendingLoad->mode = loadMode;
	pendingLoad->factory = sceneFactory->second;

	m_pendingLoad = std::move(pendingLoad);
	return true;
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
		scene->Unload();
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
				retiredScene->Unload();
			});
		}
		else
		{
			retiredScene->Unload();
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
	m_pendingLoad->phase = PendingLoadPhase::Loading;
	m_pendingLoad->elapsedTime = 0.0f;
	const SceneFactory factory = m_pendingLoad->factory;
	try
	{
		m_pendingLoad->future = std::async(std::launch::async, [factory]()
		{
			std::unique_ptr<IScene> scene = factory();
			if (!scene) throw std::runtime_error("Scene factory returned null");
			scene->Prepare();
			return scene;
		});
	}
	catch (const std::exception& error) { RecoverLoadFailure(error.what()); }
	catch (...) { RecoverLoadFailure("Unable to start scene preparation"); }
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

	if (m_pendingLoad->future.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
	{
		return;
	}

	try
	{
		std::unique_ptr<IScene> scene = m_pendingLoad->future.get();
		scene->Activate();
		CommitLoadedScene(std::move(scene), m_pendingLoad->mode);
	}
	catch (const std::exception& error) { RecoverLoadFailure(error.what()); return; }
	catch (...) { RecoverLoadFailure("Unknown asynchronous scene load failure"); return; }
	m_pendingLoad->phase = PendingLoadPhase::FadeIn;
	m_pendingLoad->elapsedTime = 0.0f;
	UpdateFadeOverlay(1.0f);
}

void SceneManager::RecoverLoadFailure(std::string message)
{
	m_lastLoadError = std::move(message);
	Diagnostics::Write("Scene load failed: " + m_lastLoadError);
	m_pendingLoad.reset();
	for (const auto& scene : m_activeScenes) scene->OnSceneLoadFailed();
}

void SceneManager::UpdateFadeOverlay(float alpha)
{
	const XMFLOAT4 color{ 0.0f, 0.0f, 0.0f, alpha };
	m_fadeOverlay.Clear();
	m_fadeOverlay.DrawRectangle(0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height), color);
	m_fadeOverlay.Upload();
}
