#include "Game/App/Game.h"

#include "Framework/Audio/IAudioService.h"
#include "Framework/Effects/IEffectService.h"
#include "Framework/Rendering/Core/IRenderDevice.h"
#include "Framework/Scene/Core/SceneManager.h"
#include "Framework/UI/IUiService.h"
#include "Game/Content/GameContent.h"
#include "Game/Scenes/GameScene.h"
#include "Game/Scenes/TitleScene.h"

#include <memory>

namespace
{
	constexpr std::string_view InitialSceneName = "Title";
}

Game::Game(
	IRenderDevice& renderDevice,
	IAudioService& audio,
	IEffectCatalog& effectCatalog,
	IEffectPlayer& effects,
	IUiService& ui,
	std::uint32_t width,
	std::uint32_t height)
	: m_renderDevice(renderDevice)
	, m_audio(audio)
	, m_effectCatalog(effectCatalog)
	, m_effects(effects)
	, m_ui(ui)
	, m_width(width)
	, m_height(height)
{
}

void Game::RegisterContent() const
{
	m_audio.RegisterBgm(GameContent::TitleBgm, GameContent::TitleBgmPath);
	m_audio.RegisterBgm(GameContent::GameBgm, GameContent::GameBgmPath);
	m_audio.RegisterSe(GameContent::ButtonSe, GameContent::ButtonSePath);
	m_effectCatalog.RegisterEffect(GameContent::JumpEffect, GameContent::JumpEffectPath);
}

void Game::RegisterScenes(SceneManager& scenes) const
{
	scenes.RegisterScene("Game", [this]()
	{
		return std::make_unique<GameScene>(m_renderDevice, m_audio, m_effects, m_width, m_height);
	});
	scenes.RegisterScene("Title", [this]()
	{
		return std::make_unique<TitleScene>(m_renderDevice, m_audio, m_ui, m_width, m_height);
	});
}

std::string_view Game::GetInitialSceneName() const
{
	return InitialSceneName;
}
