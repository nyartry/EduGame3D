#include "Game/App/Game.h"

#include "Framework/Audio/IAudioService.h"
#include "Framework/Effects/IEffectService.h"
#include "Framework/Scene/Core/SceneManager.h"
#include "Game/Content/GameContent.h"
#include "Game/Scenes/GameScene.h"
#include "Game/Scenes/TitleScene.h"

namespace
{
	constexpr std::string_view InitialSceneName = "Title";
}

void Game::RegisterContent(IAudioService& audio, IEffectService& effects) const
{
	audio.RegisterBgm(GameContent::TitleBgm, GameContent::TitleBgmPath);
	audio.RegisterBgm(GameContent::GameBgm, GameContent::GameBgmPath);
	audio.RegisterSe(GameContent::ButtonSe, GameContent::ButtonSePath);
	effects.RegisterEffect(GameContent::JumpEffect, GameContent::JumpEffectPath);
}

void Game::RegisterScenes(SceneManager& scenes) const
{
	scenes.AddScene<GameScene>("Game");
	scenes.AddScene<TitleScene>("Title");
}

std::string_view Game::GetInitialSceneName() const
{
	return InitialSceneName;
}
