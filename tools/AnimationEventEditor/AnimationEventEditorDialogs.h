#pragma once

#include <Windows.h>

#include <optional>
#include <string>

namespace AnimationEventEditorTool
{
	std::optional<std::string> ShowOpenFbxDialog(HWND hwnd);
	std::optional<std::string> ShowOpenEventDialog(HWND hwnd, const std::string& initialPath);
	std::optional<std::string> ShowSaveJsonDialog(HWND hwnd, const std::string& initialPath);
}
