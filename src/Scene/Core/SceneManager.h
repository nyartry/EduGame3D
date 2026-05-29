#pragma once

#include "Scene/Core/IScene.h"
#include "Scene/Overlays/LoadingOverlay.h"

#include <DirectXMath.h>

#include <functional>
#include <future>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

enum class SceneLoadType
{
	Synchronous,
	Asynchronous
};

enum class SceneLoadMode
{
	Single,
	Additive
};

class SceneManager
{
public:
	using SceneFactory = std::function<std::unique_ptr<IScene>()>;

	void Initialize(ID3D12Device* device, UINT width, UINT height);

	void RegisterScene(const std::string& name, SceneFactory factory);

	template <typename TScene>
	void AddScene(const std::string& name)
	{
		RegisterScene(name, []()
		{
			return std::make_unique<TScene>();
		});
	}

	bool LoadScene(
		const std::string& name,
		SceneLoadType loadType = SceneLoadType::Synchronous,
		SceneLoadMode loadMode = SceneLoadMode::Single);

	void Update(float deltaTime, const Input& input);
	void Render(Dx12Renderer& renderer) const;

	DirectX::XMMATRIX GetViewProjectionMatrix() const;
	bool IsLoading() const;

private:
	struct PendingLoad
	{
		SceneLoadMode mode{ SceneLoadMode::Single };
		std::future<std::unique_ptr<IScene>> future;
		float elapsedTime{};
	};

	struct RetiredScene
	{
		std::unique_ptr<IScene> scene;
		UINT framesRemaining{};
	};

	void ClearActiveScenes();
	void RetireActiveScenes();
	void ReleaseRetiredScenes();
	void CommitLoadedScene(std::unique_ptr<IScene> scene, SceneLoadMode mode);
	void PollAsyncLoad();

	SceneLoadContext m_context{};
	std::unordered_map<std::string, SceneFactory> m_sceneFactories;
	std::vector<std::unique_ptr<IScene>> m_activeScenes;
	std::vector<RetiredScene> m_retiredScenes;
	std::unique_ptr<PendingLoad> m_pendingLoad;
	LoadingOverlay m_loadingOverlay;
};
