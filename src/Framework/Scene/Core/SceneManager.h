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

class IRenderDevice;
class IRenderer;
class IRenderResourceLifetime;

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
	~SceneManager();

	void Initialize(
		IRenderDevice& renderDevice,
		IRenderResourceLifetime& resourceLifetime,
		std::uint32_t width,
		std::uint32_t height);

	void RegisterScene(const std::string& name, SceneFactory factory);

	bool LoadScene(
		const std::string& name,
		SceneLoadType loadType = SceneLoadType::Synchronous,
		SceneLoadMode loadMode = SceneLoadMode::Single);

	void Update(float deltaTime, const Input& input);
	void RenderWorld(IRenderer& renderer) const;
	void RenderOverlay(IRenderer& renderer) const;

	RenderView GetRenderView() const;
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

	void ClearActiveScenes();
	void RetireActiveScenes();
	void CommitLoadedScene(std::unique_ptr<IScene> scene, SceneLoadMode mode);
	void StartPendingLoad();
	void PollAsyncLoad();
	void UpdateFadeOverlay(float alpha);

	IRenderResourceLifetime* m_resourceLifetime{};
	std::uint32_t m_width{};
	std::uint32_t m_height{};
	std::unordered_map<std::string, SceneFactory> m_sceneFactories;
	std::vector<std::unique_ptr<IScene>> m_activeScenes;
	std::unique_ptr<PendingLoad> m_pendingLoad;
	LoadingOverlay m_loadingOverlay;
	SpriteBatch m_fadeOverlay;
};
