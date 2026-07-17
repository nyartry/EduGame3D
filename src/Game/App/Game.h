#pragma once

#include <string_view>

class IAudioService;
class IEffectService;
class SceneManager;

// The game module only describes game content and scenes. The launcher owns
// platform and engine implementations and connects them at the application edge.
class Game
{
public:
	void RegisterContent(IAudioService& audio, IEffectService& effects) const;
	void RegisterScenes(SceneManager& scenes) const;
	std::string_view GetInitialSceneName() const;
};
