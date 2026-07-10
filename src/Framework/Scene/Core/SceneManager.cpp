#include "Framework/Scene/Core/SceneManager.h"

#include "Framework/Rendering/Core/Dx12Renderer.h"

#include <algorithm>
#include <chrono>
#include <utility>

using namespace DirectX;

namespace
{
	constexpr float MinimumLoadingSeconds = 0.75f;
	constexpr float FadeOutSeconds = 0.45f;
	constexpr float FadeInSeconds = 0.45f;
	constexpr UINT RetiredSceneKeepAliveFrames = Dx12Renderer::FrameCount + 1;
}

void SceneManager::Initialize(ID3D12Device* device, ID3D12CommandQueue* commandQueue, IAudioService* audio, UINT width, UINT height)
{
	m_context.device = device;
	m_context.commandQueue = commandQueue;
	m_context.audio = audio;
	m_context.width = width;
	m_context.height = height;
	m_loadingOverlay.Initialize(device, width, height);
	m_fadeOverlay.Initialize(device, 1);
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
		return false;
	}

	if (loadMode == SceneLoadMode::Single && loadType == SceneLoadType::Synchronous)
	{
		ClearActiveScenes();
	}

	if (loadType == SceneLoadType::Synchronous)
	{
		std::unique_ptr<IScene> scene = sceneFactory->second();
		scene->Load(m_context);
		CommitLoadedScene(std::move(scene), loadMode);
		return true;
	}

	auto pendingLoad = std::make_unique<PendingLoad>();
	pendingLoad->mode = loadMode;
	pendingLoad->factory = sceneFactory->second;

	m_pendingLoad = std::move(pendingLoad);
	return true;
}

void SceneManager::Update(float deltaTime, const Input& input)
{
	ReleaseRetiredScenes();

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
			scene->Update(deltaTime, input);
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
		scene->Update(deltaTime, input);
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

void SceneManager::Render(Dx12Renderer& renderer) const
{
	if (m_pendingLoad != nullptr && m_pendingLoad->phase == PendingLoadPhase::Loading)
	{
		m_loadingOverlay.Render(renderer);
		return;
	}

	for (const std::unique_ptr<IScene>& scene : m_activeScenes)
	{
		scene->Render(renderer);
	}

	if (m_pendingLoad != nullptr)
	{
		m_fadeOverlay.Render(renderer);
	}
}

XMMATRIX SceneManager::GetViewProjectionMatrix() const
{
	if (m_activeScenes.empty())
	{
		return XMMatrixIdentity();
	}

	return m_activeScenes.back()->GetViewProjectionMatrix();
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
		m_retiredScenes.push_back({ std::move(scene), RetiredSceneKeepAliveFrames });
	}
	m_activeScenes.clear();
}

void SceneManager::ReleaseRetiredScenes()
{
	for (RetiredScene& retiredScene : m_retiredScenes)
	{
		if (retiredScene.framesRemaining > 0)
		{
			--retiredScene.framesRemaining;
		}
	}

	const auto removeBegin = std::remove_if(
		m_retiredScenes.begin(),
		m_retiredScenes.end(),
		[](RetiredScene& retiredScene)
		{
			if (retiredScene.framesRemaining > 0)
			{
				return false;
			}

			retiredScene.scene->Unload();
			return true;
		});
	m_retiredScenes.erase(removeBegin, m_retiredScenes.end());
}

void SceneManager::CommitLoadedScene(std::unique_ptr<IScene> scene, SceneLoadMode mode)
{
	if (mode == SceneLoadMode::Single)
	{
		RetireActiveScenes();
	}

	m_activeScenes.push_back(std::move(scene));
}

void SceneManager::StartPendingLoad()
{
	RetireActiveScenes();
	m_pendingLoad->phase = PendingLoadPhase::Loading;
	m_pendingLoad->elapsedTime = 0.0f;
	const SceneLoadContext context = m_context;
	const SceneFactory factory = m_pendingLoad->factory;
	m_pendingLoad->future = std::async(std::launch::async, [context, factory]()
	{
		std::unique_ptr<IScene> scene = factory();
		scene->Load(context);
		return scene;
	});
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

	std::unique_ptr<IScene> scene = m_pendingLoad->future.get();
	CommitLoadedScene(std::move(scene), m_pendingLoad->mode);
	m_pendingLoad->phase = PendingLoadPhase::FadeIn;
	m_pendingLoad->elapsedTime = 0.0f;
	UpdateFadeOverlay(1.0f);
}

void SceneManager::UpdateFadeOverlay(float alpha)
{
	const XMFLOAT4 color{ 0.0f, 0.0f, 0.0f, alpha };
	m_fadeOverlay.Clear();
	m_fadeOverlay.DrawRectangle(0.0f, 0.0f, static_cast<float>(m_context.width), static_cast<float>(m_context.height), color);
	m_fadeOverlay.Upload();
}
