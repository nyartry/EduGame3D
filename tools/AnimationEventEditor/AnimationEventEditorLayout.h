#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace AnimationEventEditorTool
{
	std::string_view GetDefaultImGuiLayoutIni();
	std::filesystem::path MakeEditorSettingsDirectory();
	std::string PathToUtf8String(const std::filesystem::path& path);
	bool WriteDefaultImGuiLayoutIni(const std::string& path);
	void EnsureDefaultImGuiLayoutIniExists(const std::string& path);
}
