#pragma once

#include <string_view>

namespace GameContent
{
	inline constexpr std::string_view TitleBgm = "edugame3d.audio.title-bgm";
	inline constexpr std::string_view GameBgm = "edugame3d.audio.game-bgm";
	inline constexpr std::string_view ButtonSe = "edugame3d.audio.button-se";
	inline constexpr std::string_view JumpEffect = "edugame3d.effect.jump";

	inline constexpr std::string_view ResolveCue(std::string_view cue)
	{
		// Event files created before the rename can still refer to the old catalog IDs.
		if (cue == "open-campus.audio.title-bgm") return TitleBgm;
		if (cue == "open-campus.audio.game-bgm") return GameBgm;
		if (cue == "open-campus.audio.button-se") return ButtonSe;
		if (cue == "open-campus.effect.jump") return JumpEffect;
		return cue;
	}

	inline constexpr std::string_view TitleBgmPath = "Content\\Audio\\BGM\\title_theme.wav";
	inline constexpr std::string_view GameBgmPath = "Content\\Audio\\BGM\\game_theme.wav";
	inline constexpr std::string_view ButtonSePath = "Content\\Audio\\SE\\button_click.wav";
	inline constexpr std::string_view JumpEffectPath = "Content\\Effects\\Effekseer\\Samples\\Laser01.efkefc";
}
