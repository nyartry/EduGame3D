#pragma once

#include <cstdint>
#include <string_view>

class IAudioService;
class IEffectCatalog;
class IEffectPlayer;
class IRenderDevice;
class IUiService;
class SceneManager;

// The game module only describes game content and scenes. The launcher owns
// platform and engine implementations and connects them at the application edge.
class Game
{
public:
	Game(
		IRenderDevice& renderDevice,
		IAudioService& audio,
		IEffectCatalog& effectCatalog,
		IEffectPlayer& effects,
		IUiService& ui,
		std::uint32_t width,
		std::uint32_t height);

	void RegisterContent() const;
	void RegisterScenes(SceneManager& scenes) const;
	std::string_view GetInitialSceneName() const;

private:
	IRenderDevice& m_renderDevice;
	IAudioService& m_audio;
	IEffectCatalog& m_effectCatalog;
	IEffectPlayer& m_effects;
	IUiService& m_ui;
	std::uint32_t m_width{};
	std::uint32_t m_height{};
};
