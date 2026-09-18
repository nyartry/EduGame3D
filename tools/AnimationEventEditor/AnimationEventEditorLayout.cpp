#include "AnimationEventEditorLayout.h"

#include <Windows.h>
#include <shlobj.h>

#include <array>
#include <fstream>
#include <system_error>

namespace AnimationEventEditorTool
{
	namespace
	{
		constexpr std::string_view DefaultImGuiLayoutIni = R"ini([Window][WindowOverViewport_11111111]
Pos=0,19
Size=1920,990
Collapsed=0

[Window][Debug##Default]
Pos=60,60
Size=400,400
Collapsed=0

[Window][Asset]
Pos=0,19
Size=358,756
Collapsed=0
DockId=0x00000005,1

[Window][Viewport Controls]
Pos=0,19
Size=358,756
Collapsed=0
DockId=0x00000005,0

[Window][Timeline]
Pos=361,777
Size=1559,232
Collapsed=0
DockId=0x00000004,0

[Window][Event Properties]
Pos=1548,19
Size=372,756
Collapsed=0
DockId=0x00000008,0

[Window][Status]
Pos=0,777
Size=359,232
Collapsed=0
DockId=0x00000003,0

[Docking][Data]
DockSpace       ID=0x08BD597D Window=0x1BBC0F80 Pos=0,19 Size=1920,990 Split=Y
  DockNode      ID=0x00000001 Parent=0x08BD597D SizeRef=1920,756 Split=X
    DockNode    ID=0x00000005 Parent=0x00000001 SizeRef=358,756 Selected=0xF0D94A78
    DockNode    ID=0x00000006 Parent=0x00000001 SizeRef=1560,756 Split=X
      DockNode  ID=0x00000007 Parent=0x00000006 SizeRef=1186,756 CentralNode=1
      DockNode  ID=0x00000008 Parent=0x00000006 SizeRef=372,756 Selected=0xFA1CB93A
  DockNode      ID=0x00000002 Parent=0x08BD597D SizeRef=1920,232 Split=X Selected=0x4F89F0DC
    DockNode    ID=0x00000003 Parent=0x00000002 SizeRef=359,232 Selected=0x96319633
    DockNode    ID=0x00000004 Parent=0x00000002 SizeRef=1559,232 Selected=0x4F89F0DC

)ini";

		std::filesystem::path GetLocalAppDataPath()
		{
			PWSTR knownFolderPath = nullptr;
			if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE, nullptr, &knownFolderPath)))
			{
				std::filesystem::path result = knownFolderPath;
				CoTaskMemFree(knownFolderPath);
				return result;
			}

			std::array<wchar_t, MAX_PATH> fallbackPath{};
			const DWORD length = GetEnvironmentVariableW(L"LOCALAPPDATA", fallbackPath.data(), static_cast<DWORD>(fallbackPath.size()));
			if (length > 0 && length < fallbackPath.size())
			{
				return fallbackPath.data();
			}

			return std::filesystem::current_path();
		}
	}

	std::string_view GetDefaultImGuiLayoutIni()
	{
		return DefaultImGuiLayoutIni;
	}

	std::filesystem::path MakeEditorSettingsDirectory()
	{
		const std::filesystem::path localAppData = GetLocalAppDataPath();
		const std::filesystem::path directory = localAppData / L"EduGame3D" / L"AnimationEventEditor";
		std::error_code error;
		std::filesystem::create_directories(directory, error);
		if (!error)
		{
			// Import the known legacy preference without replacing an existing EduGame3D layout.
			// Missing or unreadable legacy settings leave normal default-layout creation in place.
			std::filesystem::copy_file(localAppData / L"OpenCampusAnimationEventEditor" / L"imgui.ini",
				directory / L"imgui.ini", std::filesystem::copy_options::skip_existing, error);
			return directory;
		}

		return std::filesystem::current_path();
	}

	std::string PathToUtf8String(const std::filesystem::path& path)
	{
		const std::u8string utf8Path = path.u8string();
		return std::string(utf8Path.begin(), utf8Path.end());
	}

	bool WriteDefaultImGuiLayoutIni(const std::string& path)
	{
		std::ofstream file(path, std::ios::binary | std::ios::trunc);
		if (!file)
		{
			return false;
		}

		file.write(DefaultImGuiLayoutIni.data(), static_cast<std::streamsize>(DefaultImGuiLayoutIni.size()));
		return file.good();
	}

	void EnsureDefaultImGuiLayoutIniExists(const std::string& path)
	{
		std::error_code error;
		if (std::filesystem::exists(path, error))
		{
			return;
		}

		WriteDefaultImGuiLayoutIni(path);
	}
}
