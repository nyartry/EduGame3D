#include "Scene/Core/SceneManager.h"

#include "Rendering/Core/Dx12Renderer.h"

#include <chrono>
#include <utility>

using namespace DirectX;

void SceneManager::Initialize(ID3D12Device* device, UINT width, UINT height)
{
	m_context.device = device;
	m_context.width = width;
	m_context.height = height;
	m_loadingOverlay.Initialize(device, width, height);
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

	if (loadMode == SceneLoadMode::Single)
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
	const SceneLoadContext context = m_context;
	const SceneFactory factory = sceneFactory->second;
	pendingLoad->future = std::async(std::launch::async, [context, factory]()
	{
		std::unique_ptr<IScene> scene = factory();
		scene->Load(context);
		return scene;
	});

	m_pendingLoad = std::move(pendingLoad);
	return true;
}

void SceneManager::Update(float deltaTime, const Input& input)
{
	if (IsLoading())
	{
		m_loadingOverlay.Update(deltaTime);
	}

	PollAsyncLoad();

	for (const std::unique_ptr<IScene>& scene : m_activeScenes)
	{
		scene->Update(deltaTime, input);
	}
}

void SceneManager::Render(Dx12Renderer& renderer) const
{
	for (const std::unique_ptr<IScene>& scene : m_activeScenes)
	{
		scene->Render(renderer);
	}

	if (IsLoading())
	{
		m_loadingOverlay.Render(renderer);
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

void SceneManager::CommitLoadedScene(std::unique_ptr<IScene> scene, SceneLoadMode mode)
{
	if (mode == SceneLoadMode::Single)
	{
		ClearActiveScenes();
	}

	m_activeScenes.push_back(std::move(scene));
}

void SceneManager::PollAsyncLoad()
{
	if (m_pendingLoad == nullptr)
	{
		return;
	}

	if (m_pendingLoad->future.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
	{
		return;
	}

	CommitLoadedScene(m_pendingLoad->future.get(), m_pendingLoad->mode);
	m_pendingLoad.reset();
}
