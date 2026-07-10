#pragma once

#include "AnimationEventData.h"

#include <optional>
#include <string>
#include <string_view>

namespace AnimationEventEditorTool
{
	std::string JsonEscape(std::string_view text);
	std::string MakeDefaultEventPath(const std::string& fbxPath);
	std::optional<AnimationEventFileData> LoadAnimationEventFile(const std::string& path, std::string& error);
}
