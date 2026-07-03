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
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
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

	std::string MakeDefaultEventPath(const std::string& fbxPath)
	{
		std::filesystem::path path = std::filesystem::path(fbxPath);
		path.replace_extension(".anim_events.json");
		return path.string();
	}

	XMMATRIX BuildViewProjection(float yaw, float pitch, float distance, UINT width, UINT height)
	{
		const float aspect = height == 0 ? 1.0f : static_cast<float>(width) / static_cast<float>(height);
		const float horizontal = std::cos(pitch) * distance;
		const XMVECTOR eye = XMVectorSet(
			std::sin(yaw) * horizontal,
			1.1f + std::sin(pitch) * distance,
			std::cos(yaw) * horizontal,
			1.0f);
		const XMVECTOR target = XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);
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

			const XMMATRIX viewProjection = BuildViewProjection(
				m_cameraYaw,
				m_cameraPitch,
				m_cameraDistance,
				m_renderer.GetWidth(),
				m_renderer.GetHeight());
			m_renderer.BeginFrame(viewProjection);
			if (m_model != nullptr)
			{
				m_model->SetRotationY(m_modelRotation);
				m_model->Draw(m_renderer);
			}

			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();
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

		void SaveDefaultEvents()
		{
			SaveEvents(m_defaultSavePath);
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
			io.IniFilename = nullptr;
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
				m_status = stream.str();
			}
			catch (const std::exception& exception)
			{
				m_model.reset();
				m_boneNames.clear();
				m_status = std::string("Load failed: ") + exception.what();
			}
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
			ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
			DrawMainMenu();
			DrawAssetPanel();
			DrawViewportPanel();
			DrawTimelinePanel();
			DrawEventPanel();
			DrawStatusPanel();
		}

		void DrawMainMenu()
		{
			if (!ImGui::BeginMainMenuBar())
			{
				return;
			}

			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Open FBX..."))
				{
					OpenFbx();
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

			ImGui::EndMainMenuBar();
		}

		void DrawAssetPanel()
		{
			ImGui::Begin("Asset");
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
			ImGui::Begin("Viewport Controls");
			ImGui::SliderAngle("Camera Yaw", &m_cameraYaw, -180.0f, 180.0f);
			ImGui::SliderAngle("Camera Pitch", &m_cameraPitch, -15.0f, 60.0f);
			ImGui::SliderFloat("Camera Distance", &m_cameraDistance, 1.5f, 8.0f);
			ImGui::SliderAngle("Model Rotation", &m_modelRotation, -180.0f, 180.0f);
			ImGui::TextDisabled("The 3D preview is rendered behind the editor panels.");
			ImGui::End();
		}

		void DrawTimelinePanel()
		{
			ImGui::Begin("Timeline");
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
				AddEventAtTime(currentTime);
			}
			ImGui::SameLine();
			if (ImGui::Button("Delete Selected") && IsSelectedEventValid())
			{
				m_events.erase(m_events.begin() + m_selectedEvent);
				m_selectedEvent = -1;
			}

			DrawTimelineCanvas(duration);
			ImGui::End();
		}

		void DrawTimelineCanvas(float duration)
		{
			const float width = std::max(300.0f, ImGui::GetContentRegionAvail().x);
			const ImVec2 canvasSize(width, 128.0f);
			ImGui::InvisibleButton("timeline_canvas", canvasSize, ImGuiButtonFlags_MouseButtonLeft);
			const ImVec2 min = ImGui::GetItemRectMin();
			const ImVec2 max = ImGui::GetItemRectMax();
			const bool hovered = ImGui::IsItemHovered();
			ImDrawList* drawList = ImGui::GetWindowDrawList();

			drawList->AddRectFilled(min, max, IM_COL32(18, 24, 32, 240));
			drawList->AddRect(min, max, IM_COL32(80, 105, 130, 255));

			const float trackY = min.y + canvasSize.y * 0.55f;
			drawList->AddLine(ImVec2(min.x + 8.0f, trackY), ImVec2(max.x - 8.0f, trackY), IM_COL32(130, 150, 170, 255), 2.0f);

			const int tickCount = 10;
			for (int tick = 0; tick <= tickCount; ++tick)
			{
				const float ratio = static_cast<float>(tick) / static_cast<float>(tickCount);
				const float x = min.x + 8.0f + ratio * (canvasSize.x - 16.0f);
				drawList->AddLine(ImVec2(x, trackY - 28.0f), ImVec2(x, trackY + 28.0f), IM_COL32(60, 80, 100, 255));
				char label[32]{};
				std::snprintf(label, sizeof(label), "%.2f", duration * ratio);
				drawList->AddText(ImVec2(x + 3.0f, min.y + 8.0f), IM_COL32(170, 190, 210, 255), label);
			}

			const std::string currentAnimation = GetCurrentAnimationName();
			for (int eventIndex = 0; eventIndex < static_cast<int>(m_events.size()); ++eventIndex)
			{
				const AnimationEvent& event = m_events[static_cast<size_t>(eventIndex)];
				if (currentAnimation != event.animation)
				{
					continue;
				}

				const float ratio = std::clamp(event.time / duration, 0.0f, 1.0f);
				const float x = min.x + 8.0f + ratio * (canvasSize.x - 16.0f);
				const ImU32 color = eventIndex == m_selectedEvent
					? IM_COL32(255, 210, 90, 255)
					: IM_COL32(90, 210, 255, 255);
				drawList->AddTriangleFilled(
					ImVec2(x, trackY - 20.0f),
					ImVec2(x - 8.0f, trackY - 5.0f),
					ImVec2(x + 8.0f, trackY - 5.0f),
					color);
				drawList->AddLine(ImVec2(x, trackY - 5.0f), ImVec2(x, trackY + 28.0f), color, 2.0f);
				drawList->AddText(ImVec2(x + 8.0f, trackY + 10.0f), color, event.type);
			}

			const float currentRatio = std::clamp(m_model->GetAnimationTimeSeconds() / duration, 0.0f, 1.0f);
			const float currentX = min.x + 8.0f + currentRatio * (canvasSize.x - 16.0f);
			drawList->AddLine(ImVec2(currentX, min.y), ImVec2(currentX, max.y), IM_COL32(255, 110, 110, 255), 2.0f);

			if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			{
				const float mouseX = ImGui::GetIO().MousePos.x;
				int nearestEvent = -1;
				float nearestDistance = 10.0f;
				for (int eventIndex = 0; eventIndex < static_cast<int>(m_events.size()); ++eventIndex)
				{
					const AnimationEvent& event = m_events[static_cast<size_t>(eventIndex)];
					if (currentAnimation != event.animation)
					{
						continue;
					}

					const float x = min.x + 8.0f + std::clamp(event.time / duration, 0.0f, 1.0f) * (canvasSize.x - 16.0f);
					const float distance = std::fabs(mouseX - x);
					if (distance < nearestDistance)
					{
						nearestDistance = distance;
						nearestEvent = eventIndex;
					}
				}

				if (nearestEvent >= 0)
				{
					m_selectedEvent = nearestEvent;
				}
				else
				{
					const float ratio = std::clamp((mouseX - min.x - 8.0f) / (canvasSize.x - 16.0f), 0.0f, 1.0f);
					AddEventAtTime(ratio * duration);
				}
			}
		}

		void DrawEventPanel()
		{
			ImGui::Begin("Event Properties");
			if (!IsSelectedEventValid())
			{
				ImGui::TextDisabled("Select or create an event on the timeline.");
				ImGui::End();
				return;
			}

			AnimationEvent& event = m_events[static_cast<size_t>(m_selectedEvent)];
			ImGui::Text("Animation: %s", event.animation);
			ImGui::DragFloat("Time", &event.time, 0.005f, 0.0f, m_model ? m_model->GetCurrentAnimationDurationSeconds() : 999.0f, "%.3f s");
			ImGui::InputText("Name", event.name, sizeof(event.name));

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
			}

			if (ImGui::BeginCombo("Bone", event.bone[0] == '\0' ? "(none)" : event.bone))
			{
				if (ImGui::Selectable("(none)", event.bone[0] == '\0'))
				{
					event.bone[0] = '\0';
				}
				for (const std::string& boneName : m_boneNames)
				{
					const bool selected = boneName == event.bone;
					if (ImGui::Selectable(boneName.c_str(), selected))
					{
						CopyText(event.bone, boneName);
					}
				}
				ImGui::EndCombo();
			}

			ImGui::InputText("Cue", event.cue, sizeof(event.cue));
			ImGui::End();
		}

		void DrawStatusPanel()
		{
			ImGui::Begin("Status");
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
			m_status = "Saved events: " + path;
		}

		HWND m_hwnd{};
		Dx12Renderer m_renderer;
		ComPtr<ID3D12DescriptorHeap> m_imguiSrvHeap;
		std::array<bool, ImGuiSrvDescriptorCount> m_imguiSrvDescriptorAllocated{};
		UINT m_imguiSrvDescriptorSize{};
		std::unique_ptr<SkinnedModel> m_model;
		std::vector<std::string> m_boneNames;
		std::vector<AnimationEvent> m_events;
		std::string m_modelPath;
		std::string m_defaultSavePath;
		std::string m_status;
		std::chrono::steady_clock::time_point m_lastTick{};
		int m_selectedEvent{ -1 };
		bool m_isPlaying{};
		bool m_hasPendingResize{};
		UINT m_pendingResizeWidth{ WindowWidth };
		UINT m_pendingResizeHeight{ WindowHeight };
		float m_playbackSpeed{ 1.0f };
		float m_cameraYaw{ XMConvertToRadians(25.0f) };
		float m_cameraPitch{ XMConvertToRadians(15.0f) };
		float m_cameraDistance{ 4.0f };
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
