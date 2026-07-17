#pragma once

#include "Framework/Scene/Core/IScene.h"
#include "Framework/Scene/Overlays/LoadingOverlay.h"
#include "Framework/Rendering/Sprites/SpriteBatch.h"

#include <DirectXMath.h>

#include <cstdint>
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

	void Initialize(
		IRenderDevice& renderDevice,
		IAudioService* audio,
		IEffectService* effects,
		IUiService* ui,
		std::uint32_t width,
		std::uint32_t height);

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
	void Render(IRenderer& renderer) const;

	DirectX::XMMATRIX GetViewProjectionMatrix() const;
	bool IsLoading() const;

private:
	enum class PendingLoadPhase
	{
		FadeOut,
		Loading,
		FadeIn
	};

	struct PendingLoad
	{
		SceneLoadMode mode{ SceneLoadMode::Single };
		SceneFactory factory;
		std::future<std::unique_ptr<IScene>> future;
		float elapsedTime{};
		PendingLoadPhase phase{ PendingLoadPhase::FadeOut };
	};

	struct RetiredScene
	{
		std::unique_ptr<IScene> scene;
		std::uint32_t framesRemaining{};
	};

	void ClearActiveScenes();
	void RetireActiveScenes();
	void ReleaseRetiredScenes();
	void CommitLoadedScene(std::unique_ptr<IScene> scene, SceneLoadMode mode);
	void StartPendingLoad();
	void PollAsyncLoad();
	void UpdateFadeOverlay(float alpha);

	SceneLoadContext m_context{};
	std::unordered_map<std::string, SceneFactory> m_sceneFactories;
	std::vector<std::unique_ptr<IScene>> m_activeScenes;
	std::vector<RetiredScene> m_retiredScenes;
	std::unique_ptr<PendingLoad> m_pendingLoad;
	LoadingOverlay m_loadingOverlay;
	SpriteBatch m_fadeOverlay;
};
