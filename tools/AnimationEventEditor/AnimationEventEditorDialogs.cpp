#include "AnimationEventEditorDialogs.h"

#include <commdlg.h>

#include <array>
#include <cwchar>

namespace AnimationEventEditorTool
{
	namespace
	{
		std::string WideToUtf8(const std::wstring& text)
		{
			if (text.empty())
			{
				return {};
			}

			const int length = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
			if (length <= 0)
			{
				return {};
			}

			std::string result(static_cast<size_t>(length), '\0');
			WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, result.data(), length, nullptr, nullptr);
			result.pop_back();
			return result;
		}

		std::wstring Utf8ToWide(const std::string& text)
		{
			if (text.empty())
			{
				return {};
			}

			const int length = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
			if (length <= 0)
			{
				return {};
			}

			std::wstring result(static_cast<size_t>(length), L'\0');
			MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, result.data(), length);
			result.pop_back();
			return result;
		}
	}

	std::optional<std::string> ShowOpenFbxDialog(HWND hwnd)
	{
		std::array<wchar_t, MAX_PATH> fileName{};
		OPENFILENAMEW openFileName{};
		openFileName.lStructSize = sizeof(openFileName);
		openFileName.hwndOwner = hwnd;
		openFileName.lpstrFilter = L"FBX files (*.fbx)\0*.fbx\0All files (*.*)\0*.*\0";
		openFileName.lpstrFile = fileName.data();
		openFileName.nMaxFile = static_cast<DWORD>(fileName.size());
		openFileName.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

		if (!GetOpenFileNameW(&openFileName))
		{
			return std::nullopt;
		}

		return WideToUtf8(fileName.data());
	}

	std::optional<std::string> ShowSaveJsonDialog(HWND hwnd, const std::string& initialPath)
	{
		std::array<wchar_t, MAX_PATH> fileName{};
		const std::wstring wideInitialPath = Utf8ToWide(initialPath);
		if (!wideInitialPath.empty())
		{
			wcsncpy_s(fileName.data(), fileName.size(), wideInitialPath.c_str(), _TRUNCATE);
		}

		OPENFILENAMEW saveFileName{};
		saveFileName.lStructSize = sizeof(saveFileName);
		saveFileName.hwndOwner = hwnd;
		saveFileName.lpstrFilter = L"Animation event files (*.anim_events.json)\0*.anim_events.json\0JSON files (*.json)\0*.json\0All files (*.*)\0*.*\0";
		saveFileName.lpstrFile = fileName.data();
		saveFileName.nMaxFile = static_cast<DWORD>(fileName.size());
		saveFileName.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
		saveFileName.lpstrDefExt = L"json";

		if (!GetSaveFileNameW(&saveFileName))
		{
			return std::nullopt;
		}

		return WideToUtf8(fileName.data());
	}

	std::optional<std::string> ShowOpenEventDialog(HWND hwnd, const std::string& initialPath)
	{
		std::array<wchar_t, MAX_PATH> fileName{};
		const std::wstring wideInitialPath = Utf8ToWide(initialPath);
		if (!wideInitialPath.empty())
		{
			wcsncpy_s(fileName.data(), fileName.size(), wideInitialPath.c_str(), _TRUNCATE);
		}

		OPENFILENAMEW openFileName{};
		openFileName.lStructSize = sizeof(openFileName);
		openFileName.hwndOwner = hwnd;
		openFileName.lpstrFilter = L"Animation event files (*.anim_events.json)\0*.anim_events.json\0JSON files (*.json)\0*.json\0All files (*.*)\0*.*\0";
		openFileName.lpstrFile = fileName.data();
		openFileName.nMaxFile = static_cast<DWORD>(fileName.size());
		openFileName.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

		if (!GetOpenFileNameW(&openFileName))
		{
			return std::nullopt;
		}

		return WideToUtf8(fileName.data());
	}
}
