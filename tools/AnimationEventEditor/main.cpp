#include "Common/ModelScaleSettings.h"
#include "Models/SkinnedModel.h"
#include "Rendering/Core/Dx12Renderer.h"

#include <backends/imgui_impl_dx12.h>
#include <backends/imgui_impl_win32.h>
#include <imgui.h>

#include <Windows.h>
#include <commdlg.h>
#include <d3d12.h>
#include <DirectXMath.h>
#include <shlobj.h>
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

namespace
{
	constexpr UINT WindowWidth = 1600;
	constexpr UINT WindowHeight = 900;
	constexpr DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	constexpr UINT ImGuiSrvDescriptorCount = 64;
	constexpr size_t MaxUndoStates = 100;
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

	template <size_t Count>
	void CopyText(char (&destination)[Count], std::string_view text)
	{
		const size_t length = std::min(Count - 1, text.size());
		std::copy_n(text.data(), length, destination);
		destination[length] = '\0';
	}

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

	std::string JsonEscape(std::string_view text)
	{
		std::string result;
		for (const char character : text)
		{
			switch (character)
			{
			case '\\':
				result += "\\\\";
				break;
			case '"':
				result += "\\\"";
				break;
			case '\n':
				result += "\\n";
				break;
			case '\r':
				result += "\\r";
				break;
			case '\t':
				result += "\\t";
				break;
			default:
				result.push_back(character);
				break;
			}
		}
		return result;
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

	std::string MakeDefaultEventPath(const std::string& fbxPath)
	{
		std::filesystem::path path = std::filesystem::path(fbxPath);
		path.replace_extension(".anim_events.json");
		return path.string();
	}

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

	std::filesystem::path MakeEditorSettingsDirectory()
	{
		std::filesystem::path directory = GetLocalAppDataPath() / L"OpenCampusAnimationEventEditor";
		std::error_code error;
		std::filesystem::create_directories(directory, error);
		if (!error)
		{
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

	XMMATRIX BuildViewProjection(float yaw, float pitch, float distance, const XMFLOAT3& targetPosition, UINT width, UINT height)
	{
		const float aspect = height == 0 ? 1.0f : static_cast<float>(width) / static_cast<float>(height);
		const float horizontal = std::cos(pitch) * distance;
		const XMVECTOR target = XMLoadFloat3(&targetPosition);
		const XMVECTOR eye = XMVectorSet(
			targetPosition.x + std::sin(yaw) * horizontal,
			targetPosition.y + std::sin(pitch) * distance,
			targetPosition.z + std::cos(yaw) * horizontal,
			1.0f);
		const XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		const XMMATRIX view = XMMatrixLookAtLH(eye, target, up);
		const XMMATRIX projection = XMMatrixPerspectiveFovLH(XMConvertToRadians(55.0f), aspect, 0.05f, 100.0f);
		return view * projection;
	}

	struct AnimationEvent
	{
		float time{};
		char animation[96]{};
		char type[48]{};
		char name[96]{};
		char bone[96]{};
		char cue[128]{};
	};

	struct AnimationEventFileData
	{
		std::string sourceFbx;
		std::vector<AnimationEvent> events;
	};

	struct EventHistoryState
	{
		std::vector<AnimationEvent> events;
		int selectedEvent{ -1 };
	};

	enum class CameraDragMode
	{
		None,
		Orbit,
		Pan,
		Dolly
	};

	bool AnimationEventEquals(const AnimationEvent& left, const AnimationEvent& right)
	{
		return left.time == right.time
			&& std::strcmp(left.animation, right.animation) == 0
			&& std::strcmp(left.type, right.type) == 0
			&& std::strcmp(left.name, right.name) == 0
			&& std::strcmp(left.bone, right.bone) == 0
			&& std::strcmp(left.cue, right.cue) == 0;
	}

	bool AnimationEventListsEqual(const std::vector<AnimationEvent>& left, const std::vector<AnimationEvent>& right)
	{
		if (left.size() != right.size())
		{
			return false;
		}

		for (size_t index = 0; index < left.size(); ++index)
		{
			if (!AnimationEventEquals(left[index], right[index]))
			{
				return false;
			}
		}

		return true;
	}

	class AnimationEventJsonParser
	{
	public:
		explicit AnimationEventJsonParser(std::string_view text)
			: m_text(text)
		{
		}

		bool Parse(AnimationEventFileData& output, std::string& error)
		{
			SkipWhitespace();
			if (!Consume('{'))
			{
				return Fail("Expected root object.", error);
			}

			while (true)
			{
				SkipWhitespace();
				if (Consume('}'))
				{
					return true;
				}

				std::string key;
				if (!ParseString(key, error))
				{
					return false;
				}
				if (!Consume(':'))
				{
					return Fail("Expected ':' after object key.", error);
				}

				if (key == "sourceFbx")
				{
					if (!ParseString(output.sourceFbx, error))
					{
						return false;
					}
				}
				else if (key == "events")
				{
					if (!ParseEventsArray(output.events, error))
					{
						return false;
					}
				}
				else if (!SkipValue(error))
				{
					return false;
				}

				SkipWhitespace();
				if (Consume(','))
				{
					continue;
				}
				if (Peek() != '}')
				{
					return Fail("Expected ',' or '}' in root object.", error);
				}
			}
		}

	private:
		void SkipWhitespace()
		{
			while (m_position < m_text.size() && std::isspace(static_cast<unsigned char>(m_text[m_position])) != 0)
			{
				++m_position;
			}
		}

		char Peek()
		{
			SkipWhitespace();
			return m_position < m_text.size() ? m_text[m_position] : '\0';
		}

		bool Consume(char expected)
		{
			SkipWhitespace();
			if (m_position >= m_text.size() || m_text[m_position] != expected)
			{
				return false;
			}
			++m_position;
			return true;
		}

		bool ParseString(std::string& output, std::string& error)
		{
			SkipWhitespace();
			if (m_position >= m_text.size() || m_text[m_position] != '"')
			{
				return Fail("Expected string.", error);
			}
			++m_position;
			output.clear();

			while (m_position < m_text.size())
			{
				const char character = m_text[m_position++];
				if (character == '"')
				{
					return true;
				}

				if (character != '\\')
				{
					output.push_back(character);
					continue;
				}

				if (m_position >= m_text.size())
				{
					return Fail("Invalid escape sequence.", error);
				}

				const char escaped = m_text[m_position++];
				switch (escaped)
				{
				case '"':
				case '\\':
				case '/':
					output.push_back(escaped);
					break;
				case 'n':
					output.push_back('\n');
					break;
				case 'r':
					output.push_back('\r');
					break;
				case 't':
					output.push_back('\t');
					break;
				default:
					return Fail("Unsupported string escape sequence.", error);
				}
			}

			return Fail("Unterminated string.", error);
		}

		bool ParseNumber(float& output, std::string& error)
		{
			SkipWhitespace();
			const size_t begin = m_position;
			if (m_position < m_text.size() && (m_text[m_position] == '-' || m_text[m_position] == '+'))
			{
				++m_position;
			}
			while (m_position < m_text.size() && std::isdigit(static_cast<unsigned char>(m_text[m_position])) != 0)
			{
				++m_position;
			}
			if (m_position < m_text.size() && m_text[m_position] == '.')
			{
				++m_position;
				while (m_position < m_text.size() && std::isdigit(static_cast<unsigned char>(m_text[m_position])) != 0)
				{
					++m_position;
				}
			}
			if (m_position < m_text.size() && (m_text[m_position] == 'e' || m_text[m_position] == 'E'))
			{
				++m_position;
				if (m_position < m_text.size() && (m_text[m_position] == '-' || m_text[m_position] == '+'))
				{
					++m_position;
				}
				while (m_position < m_text.size() && std::isdigit(static_cast<unsigned char>(m_text[m_position])) != 0)
				{
					++m_position;
				}
			}

			if (begin == m_position)
			{
				return Fail("Expected number.", error);
			}

			try
			{
				output = std::stof(std::string(m_text.substr(begin, m_position - begin)));
			}
			catch (const std::exception&)
			{
				return Fail("Invalid number.", error);
			}
			return true;
		}

		bool ParseEventsArray(std::vector<AnimationEvent>& events, std::string& error)
		{
			if (!Consume('['))
			{
				return Fail("Expected events array.", error);
			}

			events.clear();
			while (true)
			{
				SkipWhitespace();
				if (Consume(']'))
				{
					return true;
				}

				AnimationEvent event{};
				if (!ParseEventObject(event, error))
				{
					return false;
				}
				events.push_back(event);

				SkipWhitespace();
				if (Consume(','))
				{
					continue;
				}
				if (Peek() != ']')
				{
					return Fail("Expected ',' or ']' in events array.", error);
				}
			}
		}

		bool ParseEventObject(AnimationEvent& event, std::string& error)
		{
			if (!Consume('{'))
			{
				return Fail("Expected event object.", error);
			}

			while (true)
			{
				SkipWhitespace();
				if (Consume('}'))
				{
					return true;
				}

				std::string key;
				if (!ParseString(key, error))
				{
					return false;
				}
				if (!Consume(':'))
				{
					return Fail("Expected ':' after event key.", error);
				}

				if (key == "animation")
				{
					std::string value;
					if (!ParseString(value, error))
					{
						return false;
					}
					CopyText(event.animation, value);
				}
				else if (key == "time")
				{
					if (!ParseNumber(event.time, error))
					{
						return false;
					}
					event.time = std::max(0.0f, event.time);
				}
				else if (key == "type")
				{
					std::string value;
					if (!ParseString(value, error))
					{
						return false;
					}
					CopyText(event.type, value);
				}
				else if (key == "name")
				{
					std::string value;
					if (!ParseString(value, error))
					{
						return false;
					}
					CopyText(event.name, value);
				}
				else if (key == "bone")
				{
					std::string value;
					if (!ParseString(value, error))
					{
						return false;
					}
					CopyText(event.bone, value);
				}
				else if (key == "cue")
				{
					std::string value;
					if (!ParseString(value, error))
					{
						return false;
					}
					CopyText(event.cue, value);
				}
				else if (!SkipValue(error))
				{
					return false;
				}

				SkipWhitespace();
				if (Consume(','))
				{
					continue;
				}
				if (Peek() != '}')
				{
					return Fail("Expected ',' or '}' in event object.", error);
				}
			}
		}

		bool SkipValue(std::string& error)
		{
			SkipWhitespace();
			const char valueStart = Peek();
			if (valueStart == '"')
			{
				std::string ignored;
				return ParseString(ignored, error);
			}
			if (valueStart == '{')
			{
				return SkipObject(error);
			}
			if (valueStart == '[')
			{
				return SkipArray(error);
			}
			if (std::isdigit(static_cast<unsigned char>(valueStart)) != 0 || valueStart == '-' || valueStart == '+')
			{
				float ignored{};
				return ParseNumber(ignored, error);
			}
			if (ConsumeLiteral("true") || ConsumeLiteral("false") || ConsumeLiteral("null"))
			{
				return true;
			}

			return Fail("Unsupported JSON value.", error);
		}

		bool SkipObject(std::string& error)
		{
			if (!Consume('{'))
			{
				return false;
			}

			while (true)
			{
				SkipWhitespace();
				if (Consume('}'))
				{
					return true;
				}

				std::string key;
				if (!ParseString(key, error))
				{
					return false;
				}
				if (!Consume(':') || !SkipValue(error))
				{
					return false;
				}
				SkipWhitespace();
				if (Consume(','))
				{
					continue;
				}
				if (Peek() != '}')
				{
					return Fail("Expected ',' or '}' while skipping object.", error);
				}
			}
		}

		bool SkipArray(std::string& error)
		{
			if (!Consume('['))
			{
				return false;
			}

			while (true)
			{
				SkipWhitespace();
				if (Consume(']'))
				{
					return true;
				}
				if (!SkipValue(error))
				{
					return false;
				}
				SkipWhitespace();
				if (Consume(','))
				{
					continue;
				}
				if (Peek() != ']')
				{
					return Fail("Expected ',' or ']' while skipping array.", error);
				}
			}
		}

		bool ConsumeLiteral(std::string_view literal)
		{
			SkipWhitespace();
			if (m_text.substr(m_position, literal.size()) != literal)
			{
				return false;
			}
			m_position += literal.size();
			return true;
		}

		bool Fail(std::string_view message, std::string& error) const
		{
			std::ostringstream stream;
			stream << message << " Offset " << m_position << ".";
			error = stream.str();
			return false;
		}

		std::string_view m_text;
		size_t m_position{};
	};

	std::optional<AnimationEventFileData> LoadAnimationEventFile(const std::string& path, std::string& error)
	{
		std::ifstream file(path, std::ios::binary);
		if (!file)
		{
			error = "Could not open file.";
			return std::nullopt;
		}

		const std::string text{
			std::istreambuf_iterator<char>(file),
			std::istreambuf_iterator<char>() };
		AnimationEventFileData data;
		AnimationEventJsonParser parser(text);
		if (!parser.Parse(data, error))
		{
			return std::nullopt;
		}
		return data;
	}

	class AnimationEventEditorApp
	{
	public:
		void Initialize(HWND hwnd)
		{
			m_hwnd = hwnd;
			m_renderer.Initialize(hwnd, WindowWidth, WindowHeight);
			CreateImGuiContext();
			m_lastTick = std::chrono::steady_clock::now();
			m_status = "Open an FBX file to begin.";
		}

		void Shutdown()
		{
			m_renderer.WaitForGpu();
			ImGui_ImplDX12_Shutdown();
			ImGui_ImplWin32_Shutdown();
			ImGui::DestroyContext();
		}

		void Tick()
		{
			if (ApplyPendingResize())
			{
				m_lastTick = std::chrono::steady_clock::now();
				return;
			}

			const float deltaTime = CalculateDeltaTime();
			UpdatePlayback(deltaTime);

			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ApplyPendingLayoutReset();
			ImGui::NewFrame();
			UpdateCameraInput();

			const XMMATRIX viewProjection = BuildViewProjection(
				m_cameraYaw,
				m_cameraPitch,
				m_cameraDistance,
				m_cameraTarget,
				m_renderer.GetWidth(),
				m_renderer.GetHeight());
			m_renderer.BeginFrame(viewProjection);
			if (m_model != nullptr)
			{
				m_model->SetRotationY(m_modelRotation);
				m_model->Draw(m_renderer);
			}

			DrawUi();
			ImGui::Render();

			ID3D12DescriptorHeap* heaps[] = { m_imguiSrvHeap.Get() };
			m_renderer.GetCommandList()->SetDescriptorHeaps(1, heaps);
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_renderer.GetCommandList());
			m_renderer.EndFrame();
		}

		void OpenFbx()
		{
			const std::optional<std::string> selectedPath = ShowOpenFbxDialog(m_hwnd);
			if (!selectedPath)
			{
				return;
			}

			LoadModel(*selectedPath);
		}

		void OpenEvents()
		{
			if (m_model == nullptr)
			{
				return;
			}

			const std::optional<std::string> selectedPath = ShowOpenEventDialog(m_hwnd, m_defaultSavePath);
			if (!selectedPath)
			{
				return;
			}

			LoadEvents(*selectedPath);
		}

		void SaveDefaultEvents()
		{
			SaveEvents(m_defaultSavePath);
		}

		void Undo()
		{
			if (m_undoStack.empty())
			{
				return;
			}

			PushRedoState(CaptureEventState());
			const EventHistoryState previousState = m_undoStack.back();
			m_undoStack.pop_back();
			RestoreEventState(previousState);
			UpdateDirtyFlag();
			m_status = "Undo.";
		}

		void Redo()
		{
			if (m_redoStack.empty())
			{
				return;
			}

			PushUndoState(CaptureEventState());
			const EventHistoryState nextState = m_redoStack.back();
			m_redoStack.pop_back();
			RestoreEventState(nextState);
			UpdateDirtyFlag();
			m_status = "Redo.";
		}

		bool CanUndo() const
		{
			return !m_undoStack.empty();
		}

		bool CanRedo() const
		{
			return !m_redoStack.empty();
		}

		void ResetLayout()
		{
			if (!WriteDefaultImGuiLayoutIni(m_imguiIniPath))
			{
				m_status = "Layout reset failed: could not write imgui.ini.";
				return;
			}

			m_pendingLayoutReset = true;
			m_status = "Layout reset to the saved default editor arrangement.";
		}

		void RequestResize(UINT width, UINT height)
		{
			if (width == 0 || height == 0)
			{
				return;
			}

			m_pendingResizeWidth = width;
			m_pendingResizeHeight = height;
			m_hasPendingResize = true;
		}

		void RequestClientResize()
		{
			RECT clientRect{};
			if (!GetClientRect(m_hwnd, &clientRect))
			{
				return;
			}

			const UINT width = static_cast<UINT>(std::max<LONG>(0, clientRect.right - clientRect.left));
			const UINT height = static_cast<UINT>(std::max<LONG>(0, clientRect.bottom - clientRect.top));
			RequestResize(width, height);
		}

	private:
		bool ApplyPendingResize()
		{
			if (!m_hasPendingResize)
			{
				return false;
			}

			m_hasPendingResize = false;
			if (m_pendingResizeWidth == m_renderer.GetWidth() && m_pendingResizeHeight == m_renderer.GetHeight())
			{
				return false;
			}

			m_renderer.Resize(m_pendingResizeWidth, m_pendingResizeHeight);
			return true;
		}

		void ApplyPendingLayoutReset()
		{
			if (!m_pendingLayoutReset)
			{
				return;
			}

			ImGui::LoadIniSettingsFromMemory(DefaultImGuiLayoutIni.data(), DefaultImGuiLayoutIni.size());
			ImGui::SaveIniSettingsToDisk(m_imguiIniPath.c_str());
			m_pendingLayoutReset = false;
		}

		void UpdateCameraInput()
		{
			const ImGuiIO& io = ImGui::GetIO();
			const bool inputBlocked = IsCameraInputBlocked();

			if (m_cameraDragMode == CameraDragMode::None && !inputBlocked)
			{
				if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
				{
					m_cameraDragMode = CameraDragMode::Orbit;
				}
				else if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
				{
					m_cameraDragMode = CameraDragMode::Pan;
				}
				else if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
				{
					m_cameraDragMode = CameraDragMode::Dolly;
				}
			}

			if (m_cameraDragMode != CameraDragMode::None)
			{
				const ImGuiMouseButton activeButton = GetCameraDragMouseButton();
				if (!ImGui::IsMouseDown(activeButton))
				{
					m_cameraDragMode = CameraDragMode::None;
				}
				else
				{
					const ImVec2 delta = io.MouseDelta;
					if (m_cameraDragMode == CameraDragMode::Orbit)
					{
						OrbitCamera(delta);
					}
					else if (m_cameraDragMode == CameraDragMode::Pan)
					{
						PanCamera(delta);
					}
					else if (m_cameraDragMode == CameraDragMode::Dolly)
					{
						ZoomCamera(delta.y * 0.015f);
					}

					ImGui::SetMouseCursor(m_cameraDragMode == CameraDragMode::Dolly
						? ImGuiMouseCursor_ResizeNS
						: ImGuiMouseCursor_ResizeAll);
				}
			}

			if (m_cameraDragMode == CameraDragMode::None && !inputBlocked && io.MouseWheel != 0.0f)
			{
				ZoomCamera(-io.MouseWheel * 0.18f);
			}
		}

		bool IsCameraInputBlocked() const
		{
			return m_mouseOverEditorPanel
				|| ImGui::IsAnyItemActive()
				|| ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup);
		}

		ImGuiMouseButton GetCameraDragMouseButton() const
		{
			switch (m_cameraDragMode)
			{
			case CameraDragMode::Orbit:
				return ImGuiMouseButton_Left;
			case CameraDragMode::Pan:
				return ImGuiMouseButton_Middle;
			case CameraDragMode::Dolly:
				return ImGuiMouseButton_Right;
			default:
				return ImGuiMouseButton_Left;
			}
		}

		void OrbitCamera(const ImVec2& mouseDelta)
		{
			constexpr float OrbitSensitivity = 0.006f;
			m_cameraYaw += mouseDelta.x * OrbitSensitivity;
			m_cameraPitch += mouseDelta.y * OrbitSensitivity;
			m_cameraPitch = std::clamp(
				m_cameraPitch,
				XMConvertToRadians(-80.0f),
				XMConvertToRadians(80.0f));
		}

		void PanCamera(const ImVec2& mouseDelta)
		{
			constexpr float PanSensitivity = 0.0015f;
			const XMVECTOR target = XMLoadFloat3(&m_cameraTarget);
			const XMVECTOR eye = CalculateCameraEye();
			const XMVECTOR forward = XMVector3Normalize(target - eye);
			const XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
			const XMVECTOR right = XMVector3Normalize(XMVector3Cross(forward, worldUp));
			const XMVECTOR up = XMVector3Normalize(XMVector3Cross(right, forward));
			const float scale = std::max(0.1f, m_cameraDistance) * PanSensitivity;
			const XMVECTOR movement = right * (mouseDelta.x * scale) + up * (mouseDelta.y * scale);
			XMStoreFloat3(&m_cameraTarget, target + movement);
		}

		void ZoomCamera(float amount)
		{
			m_cameraDistance *= std::exp(amount);
			m_cameraDistance = std::clamp(m_cameraDistance, 0.35f, 30.0f);
		}

		void ResetCamera()
		{
			m_cameraYaw = XMConvertToRadians(25.0f);
			m_cameraPitch = XMConvertToRadians(15.0f);
			m_cameraDistance = 4.0f;
			m_cameraTarget = XMFLOAT3(0.0f, 1.0f, 0.0f);
		}

		XMVECTOR CalculateCameraEye() const
		{
			const float horizontal = std::cos(m_cameraPitch) * m_cameraDistance;
			return XMVectorSet(
				m_cameraTarget.x + std::sin(m_cameraYaw) * horizontal,
				m_cameraTarget.y + std::sin(m_cameraPitch) * m_cameraDistance,
				m_cameraTarget.z + std::cos(m_cameraYaw) * horizontal,
				1.0f);
		}

		void CreateImGuiContext()
		{
			D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
			heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			heapDesc.NumDescriptors = ImGuiSrvDescriptorCount;
			heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			heapDesc.NodeMask = 0;
			if (FAILED(m_renderer.GetDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_imguiSrvHeap))))
			{
				throw std::runtime_error("Failed to create ImGui descriptor heap.");
			}
			m_imguiSrvDescriptorSize = m_renderer.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			m_imguiSrvDescriptorAllocated.fill(false);

			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO();
			io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
			m_imguiIniPath = PathToUtf8String(MakeEditorSettingsDirectory() / L"imgui.ini");
			EnsureDefaultImGuiLayoutIniExists(m_imguiIniPath);
			io.IniFilename = m_imguiIniPath.c_str();
			ImGui::StyleColorsDark();

			ImGui_ImplWin32_Init(m_hwnd);

			ImGui_ImplDX12_InitInfo initInfo{};
			initInfo.Device = m_renderer.GetDevice();
			initInfo.CommandQueue = m_renderer.GetCommandQueue();
			initInfo.NumFramesInFlight = Dx12Renderer::FrameCount;
			initInfo.RTVFormat = BackBufferFormat;
			initInfo.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			initInfo.SrvDescriptorHeap = m_imguiSrvHeap.Get();
			initInfo.UserData = this;
			initInfo.SrvDescriptorAllocFn = &AnimationEventEditorApp::AllocateImGuiSrvDescriptor;
			initInfo.SrvDescriptorFreeFn = &AnimationEventEditorApp::FreeImGuiSrvDescriptor;
			if (!ImGui_ImplDX12_Init(&initInfo))
			{
				throw std::runtime_error("Failed to initialize ImGui DX12 backend.");
			}
		}

		static void AllocateImGuiSrvDescriptor(
			ImGui_ImplDX12_InitInfo* info,
			D3D12_CPU_DESCRIPTOR_HANDLE* outCpuHandle,
			D3D12_GPU_DESCRIPTOR_HANDLE* outGpuHandle)
		{
			auto* app = static_cast<AnimationEventEditorApp*>(info->UserData);
			if (app == nullptr)
			{
				throw std::runtime_error("ImGui descriptor allocator is missing app state.");
			}

			for (UINT descriptorIndex = 0; descriptorIndex < ImGuiSrvDescriptorCount; ++descriptorIndex)
			{
				if (app->m_imguiSrvDescriptorAllocated[descriptorIndex])
				{
					continue;
				}

				app->m_imguiSrvDescriptorAllocated[descriptorIndex] = true;
				*outCpuHandle = app->m_imguiSrvHeap->GetCPUDescriptorHandleForHeapStart();
				outCpuHandle->ptr += static_cast<SIZE_T>(descriptorIndex) * app->m_imguiSrvDescriptorSize;
				*outGpuHandle = app->m_imguiSrvHeap->GetGPUDescriptorHandleForHeapStart();
				outGpuHandle->ptr += static_cast<UINT64>(descriptorIndex) * app->m_imguiSrvDescriptorSize;
				return;
			}

			throw std::runtime_error("ImGui SRV descriptor heap is full.");
		}

		static void FreeImGuiSrvDescriptor(
			ImGui_ImplDX12_InitInfo* info,
			D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
			D3D12_GPU_DESCRIPTOR_HANDLE)
		{
			auto* app = static_cast<AnimationEventEditorApp*>(info->UserData);
			if (app == nullptr || app->m_imguiSrvDescriptorSize == 0)
			{
				return;
			}

			const D3D12_CPU_DESCRIPTOR_HANDLE startHandle = app->m_imguiSrvHeap->GetCPUDescriptorHandleForHeapStart();
			if (cpuHandle.ptr < startHandle.ptr)
			{
				return;
			}

			const SIZE_T offset = cpuHandle.ptr - startHandle.ptr;
			if (offset % app->m_imguiSrvDescriptorSize != 0)
			{
				return;
			}

			const UINT descriptorIndex = static_cast<UINT>(offset / app->m_imguiSrvDescriptorSize);
			if (descriptorIndex < ImGuiSrvDescriptorCount)
			{
				app->m_imguiSrvDescriptorAllocated[descriptorIndex] = false;
			}
		}

		void LoadModel(const std::string& path)
		{
			try
			{
				auto model = std::make_unique<SkinnedModel>();
				model->Initialize(
					m_renderer.GetDevice(),
					path,
					ModelScaleSettings::NormalizeToHeight(2.0f),
					SkinningMode::Cpu);
				model->SetAnimationTimeSeconds(0.0f);

				m_model = std::move(model);
				m_modelPath = path;
				m_defaultSavePath = MakeDefaultEventPath(path);
				m_events.clear();
				m_selectedEvent = -1;
				RebuildBoneList();

				std::ostringstream stream;
				stream << "Loaded " << path << " with " << m_model->GetModelData().animations.size() << " animation(s).";
				std::error_code existsError;
				if (std::filesystem::exists(m_defaultSavePath, existsError))
				{
					std::string loadError;
					const std::optional<AnimationEventFileData> eventData = LoadAnimationEventFile(m_defaultSavePath, loadError);
					if (eventData)
					{
						m_events = eventData->events;
						stream << " Loaded " << m_events.size() << " event(s) from " << m_defaultSavePath << ".";
					}
					else
					{
						stream << " Event load failed: " << loadError;
					}
				}
				else
				{
					stream << " No existing event file found.";
				}
				ResetHistoryToSavedState();
				m_status = stream.str();
			}
			catch (const std::exception& exception)
			{
				m_model.reset();
				m_boneNames.clear();
				m_events.clear();
				m_selectedEvent = -1;
				ResetHistoryToSavedState();
				m_status = std::string("Load failed: ") + exception.what();
			}
		}

		void LoadEvents(const std::string& path)
		{
			std::string error;
			const std::optional<AnimationEventFileData> eventData = LoadAnimationEventFile(path, error);
			if (!eventData)
			{
				m_status = "Event load failed: " + error + " (" + path + ")";
				return;
			}

			m_events = eventData->events;
			m_selectedEvent = -1;
			m_defaultSavePath = path;
			ResetHistoryToSavedState();

			std::ostringstream stream;
			stream << "Loaded " << m_events.size() << " event(s): " << path;
			m_status = stream.str();
		}

		void RebuildBoneList()
		{
			m_boneNames.clear();
			if (m_model == nullptr)
			{
				return;
			}

			for (const BoneData& bone : m_model->GetModelData().bones)
			{
				m_boneNames.push_back(bone.name);
			}
		}

		float CalculateDeltaTime()
		{
			const auto now = std::chrono::steady_clock::now();
			const float deltaTime = std::chrono::duration<float>(now - m_lastTick).count();
			m_lastTick = now;
			return deltaTime;
		}

		void UpdatePlayback(float deltaTime)
		{
			if (!m_isPlaying || m_model == nullptr)
			{
				return;
			}

			const float duration = m_model->GetCurrentAnimationDurationSeconds();
			float nextTime = m_model->GetAnimationTimeSeconds() + deltaTime * m_playbackSpeed;
			if (duration > 0.0f)
			{
				nextTime = std::fmod(nextTime, duration);
				if (nextTime < 0.0f)
				{
					nextTime += duration;
				}
			}
			m_model->SetAnimationTimeSeconds(nextTime);
		}

		std::string GetCurrentAnimationName() const
		{
			if (m_model == nullptr)
			{
				return {};
			}

			const SkinnedModelData& modelData = m_model->GetModelData();
			const size_t animationIndex = m_model->GetCurrentAnimationIndex();
			if (animationIndex >= modelData.animations.size())
			{
				return {};
			}

			return modelData.animations[animationIndex].name;
		}

		void DrawUi()
		{
			HandleShortcuts();
			m_mouseOverEditorPanelThisFrame = false;
			ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
			DrawMainMenu();
			DrawAssetPanel();
			DrawViewportPanel();
			DrawTimelinePanel();
			DrawEventPanel();
			DrawStatusPanel();
			m_mouseOverEditorPanel = m_mouseOverEditorPanelThisFrame;
		}

		void RecordEditorPanelHover()
		{
			if (ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows))
			{
				m_mouseOverEditorPanelThisFrame = true;
			}
		}

		void HandleShortcuts()
		{
			const ImGuiIO& io = ImGui::GetIO();
			if (!io.KeyCtrl || ImGui::IsAnyItemActive())
			{
				return;
			}

			if (ImGui::IsKeyPressed(ImGuiKey_Z, false))
			{
				Undo();
			}
			else if (ImGui::IsKeyPressed(ImGuiKey_Y, false))
			{
				Redo();
			}
		}

		void DrawMainMenu()
		{
			if (!ImGui::BeginMainMenuBar())
			{
				return;
			}
			RecordEditorPanelHover();

			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Open FBX..."))
				{
					OpenFbx();
				}
				if (ImGui::MenuItem("Open Events...", nullptr, false, m_model != nullptr))
				{
					OpenEvents();
				}
				if (ImGui::MenuItem("Save Events", "Ctrl+S", false, m_model != nullptr))
				{
					SaveEvents(m_defaultSavePath);
				}
				if (ImGui::MenuItem("Save Events As...", nullptr, false, m_model != nullptr))
				{
					const std::optional<std::string> path = ShowSaveJsonDialog(m_hwnd, m_defaultSavePath);
					if (path)
					{
						SaveEvents(*path);
					}
				}
				if (ImGui::MenuItem("Exit"))
				{
					PostMessage(m_hwnd, WM_CLOSE, 0, 0);
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Edit"))
			{
				if (ImGui::MenuItem("Undo", "Ctrl+Z", false, CanUndo()))
				{
					Undo();
				}
				if (ImGui::MenuItem("Redo", "Ctrl+Y", false, CanRedo()))
				{
					Redo();
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("View"))
			{
				if (ImGui::MenuItem("Reset Layout"))
				{
					ResetLayout();
				}
				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}

		void DrawAssetPanel()
		{
			SetInitialWindowRect(0.0f, 19.0f, 358.0f, 756.0f);
			ImGui::Begin("Asset");
			RecordEditorPanelHover();
			if (ImGui::Button("Open FBX"))
			{
				OpenFbx();
			}

			ImGui::TextWrapped("%s", m_modelPath.empty() ? "(no FBX loaded)" : m_modelPath.c_str());
			ImGui::Separator();

			if (m_model == nullptr)
			{
				ImGui::TextDisabled("No model loaded.");
				ImGui::End();
				return;
			}

			const SkinnedModelData& modelData = m_model->GetModelData();
			ImGui::Text("Animations");
			for (size_t animationIndex = 0; animationIndex < modelData.animations.size(); ++animationIndex)
			{
				const AnimationClip& clip = modelData.animations[animationIndex];
				const bool selected = animationIndex == m_model->GetCurrentAnimationIndex();
				const std::string label = clip.name.empty()
					? "Animation " + std::to_string(animationIndex)
					: clip.name;
				if (ImGui::Selectable(label.c_str(), selected))
				{
					m_model->PlayAnimationByIndex(animationIndex);
					m_isPlaying = false;
					m_selectedEvent = -1;
				}
			}

			ImGui::Separator();
			ImGui::Text("Bones: %zu", modelData.bones.size());
			ImGui::Text("Meshes: %zu", modelData.meshes.size());
			ImGui::End();
		}

		void DrawViewportPanel()
		{
			SetInitialWindowRect(0.0f, 19.0f, 358.0f, 756.0f);
			ImGui::Begin("Viewport Controls");
			RecordEditorPanelHover();
			ImGui::SliderAngle("Camera Yaw", &m_cameraYaw, -180.0f, 180.0f);
			ImGui::SliderAngle("Camera Pitch", &m_cameraPitch, -15.0f, 60.0f);
			ImGui::SliderFloat("Camera Distance", &m_cameraDistance, 1.5f, 8.0f);
			ImGui::DragFloat3("Camera Target", &m_cameraTarget.x, 0.01f);
			if (ImGui::Button("Reset Camera"))
			{
				ResetCamera();
			}
			ImGui::SliderAngle("Model Rotation", &m_modelRotation, -180.0f, 180.0f);
			ImGui::TextDisabled("The 3D preview is rendered behind the editor panels.");
			ImGui::End();
		}

		void DrawTimelinePanel()
		{
			SetInitialWindowRect(361.0f, 777.0f, 1559.0f, 232.0f);
			ImGui::Begin("Timeline");
			RecordEditorPanelHover();
			if (m_model == nullptr)
			{
				ImGui::TextDisabled("Open an FBX to edit animation events.");
				ImGui::End();
				return;
			}

			const float duration = std::max(0.001f, m_model->GetCurrentAnimationDurationSeconds());
			float currentTime = std::clamp(m_model->GetAnimationTimeSeconds(), 0.0f, duration);

			if (ImGui::Button(m_isPlaying ? "Pause" : "Play"))
			{
				m_isPlaying = !m_isPlaying;
			}
			ImGui::SameLine();
			if (ImGui::Button("Stop"))
			{
				m_isPlaying = false;
				m_model->SetAnimationTimeSeconds(0.0f);
			}
			ImGui::SameLine();
			ImGui::SetNextItemWidth(120.0f);
			ImGui::SliderFloat("Speed", &m_playbackSpeed, 0.1f, 2.0f);

			ImGui::SetNextItemWidth(-1.0f);
			if (ImGui::SliderFloat("Time", &currentTime, 0.0f, duration, "%.3f s"))
			{
				m_isPlaying = false;
				m_model->SetAnimationTimeSeconds(currentTime);
			}

			if (ImGui::Button("Add Event At Current Time"))
			{
				const EventHistoryState beforeEdit = CaptureEventState();
				AddEventAtTime(currentTime);
				CommitEventEdit(beforeEdit);
			}
			ImGui::SameLine();
			if (ImGui::Button("Delete Selected") && IsSelectedEventValid())
			{
				const EventHistoryState beforeEdit = CaptureEventState();
				m_events.erase(m_events.begin() + m_selectedEvent);
				m_selectedEvent = -1;
				CommitEventEdit(beforeEdit);
			}

			DrawTimelineCanvas(duration);
			ImGui::End();
		}

		void DrawTimelineCanvas(float duration)
		{
			const float width = std::max(300.0f, ImGui::GetContentRegionAvail().x);
			const ImVec2 canvasSize(width, 128.0f);
			ImGui::InvisibleButton("timeline_canvas", canvasSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
			const ImVec2 min = ImGui::GetItemRectMin();
			const ImVec2 max = ImGui::GetItemRectMax();
			const bool hovered = ImGui::IsItemHovered();
			const ImGuiIO& io = ImGui::GetIO();
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			const float timelineStartX = min.x + 8.0f;
			const float timelineWidth = canvasSize.x - 16.0f;
			const float trackY = min.y + canvasSize.y * 0.55f;
			const std::string currentAnimation = GetCurrentAnimationName();

			auto timeToX = [&](float time)
			{
				const float ratio = std::clamp(time / duration, 0.0f, 1.0f);
				return timelineStartX + ratio * timelineWidth;
			};

			auto mouseXToTime = [&](float mouseX)
			{
				const float ratio = std::clamp((mouseX - timelineStartX) / timelineWidth, 0.0f, 1.0f);
				return ratio * duration;
			};

			const int hoveredEvent = hovered
				? FindTimelineEventAtPosition(min, canvasSize, duration, currentAnimation, io.MousePos)
				: -1;
			if (hoveredEvent >= 0 || m_draggedTimelineEvent >= 0)
			{
				ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
			}

			if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			{
				if (hoveredEvent >= 0)
				{
					m_selectedEvent = hoveredEvent;
					m_draggedTimelineEvent = hoveredEvent;
					m_timelineDragStartState = CaptureEventState();
					m_isPlaying = false;
				}
				else
				{
					m_selectedEvent = -1;
				}
			}

			if (m_draggedTimelineEvent >= 0)
			{
				if (!ImGui::IsMouseDown(ImGuiMouseButton_Left) || m_draggedTimelineEvent >= static_cast<int>(m_events.size()))
				{
					CommitEventEdit(m_timelineDragStartState);
					m_draggedTimelineEvent = -1;
				}
				else
				{
					AnimationEvent& draggedEvent = m_events[static_cast<size_t>(m_draggedTimelineEvent)];
					draggedEvent.time = mouseXToTime(io.MousePos.x);
					m_selectedEvent = m_draggedTimelineEvent;
					m_model->SetAnimationTimeSeconds(draggedEvent.time);
				}
			}

			if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
			{
				m_timelineContextTime = mouseXToTime(io.MousePos.x);
				if (hoveredEvent >= 0)
				{
					m_selectedEvent = hoveredEvent;
				}
				ImGui::OpenPopup("timeline_context");
			}

			if (ImGui::BeginPopup("timeline_context"))
			{
				if (ImGui::MenuItem("Add Event Here"))
				{
					const EventHistoryState beforeEdit = CaptureEventState();
					AddEventAtTime(m_timelineContextTime);
					CommitEventEdit(beforeEdit);
				}
				if (IsSelectedEventValid() && ImGui::MenuItem("Delete Selected Event"))
				{
					const EventHistoryState beforeEdit = CaptureEventState();
					m_events.erase(m_events.begin() + m_selectedEvent);
					m_selectedEvent = -1;
					CommitEventEdit(beforeEdit);
				}
				ImGui::EndPopup();
			}

			drawList->AddRectFilled(min, max, IM_COL32(18, 24, 32, 240));
			drawList->AddRect(min, max, IM_COL32(80, 105, 130, 255));

			drawList->AddLine(ImVec2(timelineStartX, trackY), ImVec2(timelineStartX + timelineWidth, trackY), IM_COL32(130, 150, 170, 255), 2.0f);

			const int tickCount = 10;
			for (int tick = 0; tick <= tickCount; ++tick)
			{
				const float ratio = static_cast<float>(tick) / static_cast<float>(tickCount);
				const float x = timelineStartX + ratio * timelineWidth;
				drawList->AddLine(ImVec2(x, trackY - 28.0f), ImVec2(x, trackY + 28.0f), IM_COL32(60, 80, 100, 255));
				char label[32]{};
				std::snprintf(label, sizeof(label), "%.2f", duration * ratio);
				drawList->AddText(ImVec2(x + 3.0f, min.y + 8.0f), IM_COL32(170, 190, 210, 255), label);
			}

			for (int eventIndex = 0; eventIndex < static_cast<int>(m_events.size()); ++eventIndex)
			{
				const AnimationEvent& event = m_events[static_cast<size_t>(eventIndex)];
				if (currentAnimation != event.animation)
				{
					continue;
				}

				const float x = timeToX(event.time);
				const bool selected = eventIndex == m_selectedEvent;
				const bool hot = eventIndex == hoveredEvent || eventIndex == m_draggedTimelineEvent;
				const ImU32 color = selected
					? IM_COL32(255, 210, 90, 255)
					: (hot ? IM_COL32(140, 230, 255, 255) : IM_COL32(90, 210, 255, 255));
				drawList->AddTriangleFilled(
					ImVec2(x, trackY - 20.0f),
					ImVec2(x - 8.0f, trackY - 5.0f),
					ImVec2(x + 8.0f, trackY - 5.0f),
					color);
				drawList->AddLine(ImVec2(x, trackY - 5.0f), ImVec2(x, trackY + 28.0f), color, 2.0f);
				drawList->AddText(ImVec2(x + 8.0f, trackY + 10.0f), color, event.type);
			}

			const float currentRatio = std::clamp(m_model->GetAnimationTimeSeconds() / duration, 0.0f, 1.0f);
			const float currentX = timelineStartX + currentRatio * timelineWidth;
			drawList->AddLine(ImVec2(currentX, min.y), ImVec2(currentX, max.y), IM_COL32(255, 110, 110, 255), 2.0f);
		}

		int FindTimelineEventAtPosition(
			const ImVec2& canvasMin,
			const ImVec2& canvasSize,
			float duration,
			const std::string& currentAnimation,
			const ImVec2& position) const
		{
			const float timelineStartX = canvasMin.x + 8.0f;
			const float timelineWidth = canvasSize.x - 16.0f;
			const float trackY = canvasMin.y + canvasSize.y * 0.55f;
			if (position.y < trackY - 28.0f || position.y > trackY + 42.0f)
			{
				return -1;
			}

			int nearestEvent = -1;
			float nearestDistance = 12.0f;
			for (int eventIndex = 0; eventIndex < static_cast<int>(m_events.size()); ++eventIndex)
			{
				const AnimationEvent& event = m_events[static_cast<size_t>(eventIndex)];
				if (currentAnimation != event.animation)
				{
					continue;
				}

				const float ratio = std::clamp(event.time / duration, 0.0f, 1.0f);
				const float x = timelineStartX + ratio * timelineWidth;
				const float distance = std::fabs(position.x - x);
				if (distance < nearestDistance)
				{
					nearestDistance = distance;
					nearestEvent = eventIndex;
				}
			}

			return nearestEvent;
		}

		void DrawEventPanel()
		{
			SetInitialWindowRect(1548.0f, 19.0f, 372.0f, 756.0f);
			ImGui::Begin("Event Properties");
			RecordEditorPanelHover();
			if (!IsSelectedEventValid())
			{
				ImGui::TextDisabled("Select or create an event on the timeline.");
				ImGui::End();
				return;
			}

			const EventHistoryState beforeEdit = CaptureEventState();
			AnimationEvent& event = m_events[static_cast<size_t>(m_selectedEvent)];
			bool edited = false;
			ImGui::Text("Animation: %s", event.animation);
			edited |= ImGui::DragFloat("Time", &event.time, 0.005f, 0.0f, m_model ? m_model->GetCurrentAnimationDurationSeconds() : 999.0f, "%.3f s");
			edited |= ImGui::InputText("Name", event.name, sizeof(event.name));

			const char* eventTypes[] =
			{
				"Footstep",
				"PlaySE",
				"PlayEffect",
				"HitboxStart",
				"HitboxEnd",
				"Custom"
			};
			int selectedType = 0;
			for (int index = 0; index < static_cast<int>(std::size(eventTypes)); ++index)
			{
				if (std::string_view(event.type) == eventTypes[index])
				{
					selectedType = index;
					break;
				}
			}
			if (ImGui::Combo("Type", &selectedType, eventTypes, static_cast<int>(std::size(eventTypes))))
			{
				CopyText(event.type, eventTypes[selectedType]);
				edited = true;
			}

			if (ImGui::BeginCombo("Bone", event.bone[0] == '\0' ? "(none)" : event.bone))
			{
				if (ImGui::Selectable("(none)", event.bone[0] == '\0'))
				{
					event.bone[0] = '\0';
					edited = true;
				}
				for (const std::string& boneName : m_boneNames)
				{
					const bool selected = boneName == event.bone;
					if (ImGui::Selectable(boneName.c_str(), selected))
					{
						CopyText(event.bone, boneName);
						edited = true;
					}
				}
				ImGui::EndCombo();
			}

			edited |= ImGui::InputText("Cue", event.cue, sizeof(event.cue));
			if (edited)
			{
				CommitEventEdit(beforeEdit);
			}
			ImGui::End();
		}

		void DrawStatusPanel()
		{
			SetInitialWindowRect(0.0f, 777.0f, 359.0f, 232.0f);
			ImGui::Begin("Status");
			RecordEditorPanelHover();
			ImGui::Text("Events: %s", m_hasUnsavedChanges ? "Unsaved changes" : "Saved");
			ImGui::TextWrapped("%s", m_status.c_str());
			if (!m_defaultSavePath.empty())
			{
				ImGui::TextWrapped("Default save: %s", m_defaultSavePath.c_str());
			}
			ImGui::End();
		}

		void AddEventAtTime(float time)
		{
			if (m_model == nullptr)
			{
				return;
			}

			AnimationEvent event{};
			event.time = std::clamp(time, 0.0f, std::max(0.0f, m_model->GetCurrentAnimationDurationSeconds()));
			CopyText(event.animation, GetCurrentAnimationName());
			CopyText(event.type, "Footstep");
			CopyText(event.name, "Footstep");
			CopyText(event.cue, "footstep_default");
			m_events.push_back(event);
			m_selectedEvent = static_cast<int>(m_events.size()) - 1;
		}

		bool IsSelectedEventValid() const
		{
			return m_selectedEvent >= 0 && m_selectedEvent < static_cast<int>(m_events.size());
		}

		void SaveEvents(const std::string& path)
		{
			if (path.empty())
			{
				m_status = "Save failed: path is empty.";
				return;
			}

			std::ofstream file(path, std::ios::binary);
			if (!file)
			{
				m_status = "Save failed: " + path;
				return;
			}

			file << "{\n";
			file << "  \"schema\": \"open-campus-animation-events-v1\",\n";
			file << "  \"sourceFbx\": \"" << JsonEscape(m_modelPath) << "\",\n";
			file << "  \"events\": [\n";
			for (size_t index = 0; index < m_events.size(); ++index)
			{
				const AnimationEvent& event = m_events[index];
				file << "    {\n";
				file << "      \"animation\": \"" << JsonEscape(event.animation) << "\",\n";
				file << "      \"time\": " << event.time << ",\n";
				file << "      \"type\": \"" << JsonEscape(event.type) << "\",\n";
				file << "      \"name\": \"" << JsonEscape(event.name) << "\",\n";
				file << "      \"bone\": \"" << JsonEscape(event.bone) << "\",\n";
				file << "      \"cue\": \"" << JsonEscape(event.cue) << "\"\n";
				file << "    }" << (index + 1 == m_events.size() ? "\n" : ",\n");
			}
			file << "  ]\n";
			file << "}\n";

			m_defaultSavePath = path;
			m_lastSavedEvents = m_events;
			UpdateDirtyFlag();
			m_status = "Saved events: " + path;
		}

		EventHistoryState CaptureEventState() const
		{
			EventHistoryState state;
			state.events = m_events;
			state.selectedEvent = m_selectedEvent;
			return state;
		}

		void RestoreEventState(const EventHistoryState& state)
		{
			m_events = state.events;
			m_selectedEvent = state.selectedEvent;
			if (!IsSelectedEventValid())
			{
				m_selectedEvent = -1;
			}
		}

		void CommitEventEdit(const EventHistoryState& beforeEdit)
		{
			const EventHistoryState afterEdit = CaptureEventState();
			if (AnimationEventListsEqual(beforeEdit.events, afterEdit.events) && beforeEdit.selectedEvent == afterEdit.selectedEvent)
			{
				return;
			}

			PushUndoState(beforeEdit);
			m_redoStack.clear();
			UpdateDirtyFlag();
		}

		void PushUndoState(const EventHistoryState& state)
		{
			m_undoStack.push_back(state);
			if (m_undoStack.size() > MaxUndoStates)
			{
				m_undoStack.erase(m_undoStack.begin());
			}
		}

		void PushRedoState(const EventHistoryState& state)
		{
			m_redoStack.push_back(state);
			if (m_redoStack.size() > MaxUndoStates)
			{
				m_redoStack.erase(m_redoStack.begin());
			}
		}

		void ResetHistoryToSavedState()
		{
			m_undoStack.clear();
			m_redoStack.clear();
			m_lastSavedEvents = m_events;
			UpdateDirtyFlag();
		}

		void UpdateDirtyFlag()
		{
			m_hasUnsavedChanges = !AnimationEventListsEqual(m_events, m_lastSavedEvents);
		}

		void SetInitialWindowRect(float x, float y, float width, float height)
		{
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			const float safeWidth = std::max(180.0f, std::min(width, viewport->WorkSize.x - 16.0f));
			const float safeHeight = std::max(96.0f, std::min(height, viewport->WorkSize.y - 16.0f));
			const float maxX = std::max(8.0f, viewport->WorkSize.x - safeWidth - 8.0f);
			const float maxY = std::max(8.0f, viewport->WorkSize.y - safeHeight - 8.0f);
			const float safeX = std::clamp(x, 8.0f, maxX);
			const float safeY = std::clamp(y, 8.0f, maxY);

			ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + safeX, viewport->WorkPos.y + safeY), ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSize(ImVec2(safeWidth, safeHeight), ImGuiCond_FirstUseEver);
		}

		HWND m_hwnd{};
		Dx12Renderer m_renderer;
		ComPtr<ID3D12DescriptorHeap> m_imguiSrvHeap;
		std::array<bool, ImGuiSrvDescriptorCount> m_imguiSrvDescriptorAllocated{};
		UINT m_imguiSrvDescriptorSize{};
		std::unique_ptr<SkinnedModel> m_model;
		std::vector<std::string> m_boneNames;
		std::vector<AnimationEvent> m_events;
		std::vector<AnimationEvent> m_lastSavedEvents;
		std::vector<EventHistoryState> m_undoStack;
		std::vector<EventHistoryState> m_redoStack;
		EventHistoryState m_timelineDragStartState;
		std::string m_modelPath;
		std::string m_defaultSavePath;
		std::string m_status;
		std::string m_imguiIniPath;
		std::chrono::steady_clock::time_point m_lastTick{};
		int m_selectedEvent{ -1 };
		int m_draggedTimelineEvent{ -1 };
		bool m_isPlaying{};
		bool m_hasPendingResize{};
		bool m_hasUnsavedChanges{};
		bool m_pendingLayoutReset{};
		bool m_mouseOverEditorPanel{};
		bool m_mouseOverEditorPanelThisFrame{};
		UINT m_pendingResizeWidth{ WindowWidth };
		UINT m_pendingResizeHeight{ WindowHeight };
		CameraDragMode m_cameraDragMode{ CameraDragMode::None };
		float m_playbackSpeed{ 1.0f };
		float m_cameraYaw{ XMConvertToRadians(25.0f) };
		float m_cameraPitch{ XMConvertToRadians(15.0f) };
		float m_cameraDistance{ 4.0f };
		XMFLOAT3 m_cameraTarget{ 0.0f, 1.0f, 0.0f };
		float m_timelineContextTime{};
		float m_modelRotation{};
	};

	AnimationEventEditorApp* GApp{};

	LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		if (ImGui::GetCurrentContext() != nullptr && ImGui_ImplWin32_WndProcHandler(hwnd, message, wParam, lParam))
		{
			return true;
		}

		switch (message)
		{
		case WM_SIZE:
			if (GApp != nullptr && wParam != SIZE_MINIMIZED)
			{
				GApp->RequestClientResize();
				return 0;
			}
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		case WM_KEYDOWN:
			if (wParam == VK_ESCAPE)
			{
				PostMessage(hwnd, WM_CLOSE, 0, 0);
				return 0;
			}
			if ((GetKeyState(VK_CONTROL) & 0x8000) != 0 && wParam == 'O' && GApp != nullptr)
			{
				GApp->OpenFbx();
				return 0;
			}
			if ((GetKeyState(VK_CONTROL) & 0x8000) != 0 && wParam == 'Z' && GApp != nullptr)
			{
				GApp->Undo();
				return 0;
			}
			if ((GetKeyState(VK_CONTROL) & 0x8000) != 0 && wParam == 'Y' && GApp != nullptr)
			{
				GApp->Redo();
				return 0;
			}
			if ((GetKeyState(VK_CONTROL) & 0x8000) != 0 && wParam == 'S' && GApp != nullptr)
			{
				GApp->SaveDefaultEvents();
				return 0;
			}
			break;
		default:
			break;
		}

		return DefWindowProc(hwnd, message, wParam, lParam);
	}
}

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand)
{
	try
	{
		const wchar_t className[] = L"OpenCampusAnimationEventEditorWindow";
		WNDCLASSEXW windowClass{};
		windowClass.cbSize = sizeof(WNDCLASSEXW);
		windowClass.style = CS_HREDRAW | CS_VREDRAW;
		windowClass.lpfnWndProc = WindowProc;
		windowClass.hInstance = instance;
		windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
		windowClass.lpszClassName = className;
		RegisterClassExW(&windowClass);

		RECT windowRect = { 0, 0, static_cast<LONG>(WindowWidth), static_cast<LONG>(WindowHeight) };
		AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

		HWND hwnd = CreateWindowExW(
			0,
			className,
			L"Open Campus Animation Event Editor",
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			windowRect.right - windowRect.left,
			windowRect.bottom - windowRect.top,
			nullptr,
			nullptr,
			instance,
			nullptr);

		if (hwnd == nullptr)
		{
			return 1;
		}

		AnimationEventEditorApp app;
		GApp = &app;
		app.Initialize(hwnd);
		ShowWindow(hwnd, showCommand);

		MSG message{};
		while (message.message != WM_QUIT)
		{
			if (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&message);
				DispatchMessage(&message);
			}
			else
			{
				app.Tick();
			}
		}

		app.Shutdown();
		GApp = nullptr;
		return static_cast<int>(message.wParam);
	}
	catch (const std::exception& exception)
	{
		MessageBoxA(nullptr, exception.what(), "Animation Event Editor Error", MB_OK | MB_ICONERROR);
		return 1;
	}
}
