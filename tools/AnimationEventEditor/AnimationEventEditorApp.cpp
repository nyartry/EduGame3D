#include "AnimationEventEditorApp.h"

#include "AnimationEventData.h"
#include "AnimationEventEditorDialogs.h"
#include "AnimationEventEditorLayout.h"
#include "AnimationEventJson.h"
#include "Framework/Common/ModelScaleSettings.h"
#include "Framework/Core/Math/Transform.h"
#include "Framework/Models/SkinnedModel.h"
#include "Framework/Rendering/Core/Dx12Renderer.h"

#include <backends/imgui_impl_dx12.h>
#include <backends/imgui_impl_win32.h>
#include <imgui.h>

#include <Windows.h>
#include <d3d12.h>
#include <DirectXMath.h>
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

namespace AnimationEventEditorTool
{
namespace
{
	constexpr UINT WindowWidth = AnimationEventEditorApp::DefaultWindowWidth;
	constexpr UINT WindowHeight = AnimationEventEditorApp::DefaultWindowHeight;
	constexpr DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	constexpr UINT ImGuiSrvDescriptorCount = 64;
	constexpr size_t MaxUndoStates = 100;

	void AddGridStrip(
		std::vector<Vertex>& vertices,
		float minX,
		float maxX,
		float minZ,
		float maxZ,
		const std::array<float, 4>& color)
	{
		constexpr float GridY = -0.01f;
		const Vertex v0{ { minX, GridY, minZ }, { color[0], color[1], color[2], color[3] } };
		const Vertex v1{ { maxX, GridY, minZ }, { color[0], color[1], color[2], color[3] } };
		const Vertex v2{ { maxX, GridY, maxZ }, { color[0], color[1], color[2], color[3] } };
		const Vertex v3{ { minX, GridY, maxZ }, { color[0], color[1], color[2], color[3] } };

		vertices.push_back(v0);
		vertices.push_back(v1);
		vertices.push_back(v2);
		vertices.push_back(v0);
		vertices.push_back(v2);
		vertices.push_back(v3);
	}

	std::vector<Vertex> CreateViewportGridVertices()
	{
		constexpr float GridExtent = 4.0f;
		constexpr float GridStep = 0.5f;
		constexpr float MinorThickness = 0.004f;
		constexpr float MajorThickness = 0.008f;
		constexpr int GridLineCount = static_cast<int>((GridExtent * 2.0f) / GridStep);
		constexpr std::array<float, 4> MinorColor{ 0.115f, 0.17f, 0.22f, 1.0f };
		constexpr std::array<float, 4> MajorColor{ 0.17f, 0.25f, 0.32f, 1.0f };
		constexpr std::array<float, 4> CenterColor{ 0.22f, 0.32f, 0.40f, 1.0f };

		std::vector<Vertex> vertices;
		vertices.reserve(static_cast<size_t>(GridLineCount + 1) * 12);

		for (int line = 0; line <= GridLineCount; ++line)
		{
			const float offset = -GridExtent + static_cast<float>(line) * GridStep;
			const bool isCenter = std::fabs(offset) < 0.001f;
			const bool isMajor = line % 2 == 0;
			const float halfThickness = (isCenter || isMajor ? MajorThickness : MinorThickness) * 0.5f;
			const std::array<float, 4>& color = isCenter ? CenterColor : (isMajor ? MajorColor : MinorColor);

			AddGridStrip(vertices, offset - halfThickness, offset + halfThickness, -GridExtent, GridExtent, color);
			AddGridStrip(vertices, -GridExtent, GridExtent, offset - halfThickness, offset + halfThickness, color);
		}

		return vertices;
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

}

	struct AnimationEventEditorApp::Impl
	{
	public:
		void Initialize(HWND hwnd)
		{
			m_hwnd = hwnd;
			m_renderer.Initialize(hwnd, WindowWidth, WindowHeight);
			m_renderer.CreateVertexBuffer(m_viewportGrid, CreateViewportGridVertices());
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

			ProcessPendingModelLoad();
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
			m_renderer.Draw(m_viewportGrid, XMMatrixIdentity());
			if (m_model != nullptr)
			{
				Transform modelTransform;
				modelTransform.rotationRadians.y = m_modelRotation;
				m_model->Draw(m_renderer, modelTransform.ToMatrix());
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

			// The UI may run after model draw commands have already been recorded.
			// Both menu and keyboard requests are applied at the next frame boundary.
			m_pendingModelPath = *selectedPath;
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

			const std::string_view defaultLayout = GetDefaultImGuiLayoutIni();
			ImGui::LoadIniSettingsFromMemory(defaultLayout.data(), defaultLayout.size());
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
			initInfo.SrvDescriptorAllocFn = &Impl::AllocateImGuiSrvDescriptor;
			initInfo.SrvDescriptorFreeFn = &Impl::FreeImGuiSrvDescriptor;
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
			auto* app = static_cast<Impl*>(info->UserData);
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
			auto* app = static_cast<Impl*>(info->UserData);
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

		void ProcessPendingModelLoad()
		{
			if (!m_pendingModelPath)
			{
				return;
			}

			const std::string path = std::move(*m_pendingModelPath);
			m_pendingModelPath.reset();
			LoadModel(path);
			m_lastTick = std::chrono::steady_clock::now();
		}

		void LoadModel(const std::string& path)
		{
			try
			{
				auto model = std::make_unique<SkinnedModel>();
				model->Initialize(
					m_renderer,
					path,
					ModelScaleSettings::NormalizeToHeight(2.0f),
					SkinningMode::Cpu);
				model->SetAnimationTimeSeconds(0.0f);

				// Prepare all replacement state before touching the current document.
				std::string modelPath = path;
				std::string defaultSavePath = MakeDefaultEventPath(path);
				std::vector<std::string> boneNames;
				for (const BoneData& bone : model->GetModelData().bones)
				{
					boneNames.push_back(bone.name);
				}
				std::vector<AnimationEvent> events;

				std::ostringstream stream;
				stream << "Loaded " << path << " with " << model->GetModelData().animations.size() << " animation(s).";
				std::error_code existsError;
				if (std::filesystem::exists(defaultSavePath, existsError))
				{
					std::string loadError;
					const std::optional<AnimationEventFileData> eventData = LoadAnimationEventFile(defaultSavePath, loadError);
					if (eventData)
					{
						events = eventData->events;
						stream << " Loaded " << events.size() << " event(s) from " << defaultSavePath << ".";
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
				std::vector<AnimationEvent> savedEvents = events;
				std::string status = stream.str();

				// Called before BeginFrame, so every use of the old model has been
				// submitted. Wait before releasing its vertex buffers and materials.
				m_renderer.WaitForGpu();
				m_model = std::move(model);
				m_modelPath = std::move(modelPath);
				m_defaultSavePath = std::move(defaultSavePath);
				m_boneNames = std::move(boneNames);
				m_events = std::move(events);
				m_lastSavedEvents = std::move(savedEvents);
				m_selectedEvent = -1;
				m_draggedTimelineEvent = -1;
				m_timelineDragMoved = false;
				m_undoStack.clear();
				m_redoStack.clear();
				UpdateDirtyFlag();
				m_status = std::move(status);
			}
			catch (const std::exception& exception)
			{
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
			ImGui::TextUnformatted("Speed");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(96.0f);
			if (ImGui::InputFloat("##SpeedInput", &m_playbackSpeed, 0.1f, 0.5f, "%.3f"))
			{
				m_playbackSpeed = std::clamp(m_playbackSpeed, 0.1f, 2.0f);
			}
			ImGui::SameLine();
			ImGui::SetNextItemWidth(160.0f);
			if (ImGui::SliderFloat("##SpeedSlider", &m_playbackSpeed, 0.1f, 2.0f, "%.3f"))
			{
				m_playbackSpeed = std::clamp(m_playbackSpeed, 0.1f, 2.0f);
			}

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
			constexpr float DragStartThresholdPixels = 4.0f;
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
					m_timelineDragStartMouseX = io.MousePos.x;
					m_timelineDragStartTime = m_events[static_cast<size_t>(hoveredEvent)].time;
					m_timelineDragMoved = false;
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
					if (m_timelineDragMoved)
					{
						CommitEventEdit(m_timelineDragStartState);
					}
					m_draggedTimelineEvent = -1;
					m_timelineDragMoved = false;
				}
				else
				{
					const float mouseDeltaX = io.MousePos.x - m_timelineDragStartMouseX;
					if (!m_timelineDragMoved && std::fabs(mouseDeltaX) >= DragStartThresholdPixels)
					{
						m_timelineDragMoved = true;
						m_isPlaying = false;
					}

					if (m_timelineDragMoved)
					{
						AnimationEvent& draggedEvent = m_events[static_cast<size_t>(m_draggedTimelineEvent)];
						const float timeDelta = (mouseDeltaX / timelineWidth) * duration;
						draggedEvent.time = std::clamp(m_timelineDragStartTime + timeDelta, 0.0f, duration);
						m_selectedEvent = m_draggedTimelineEvent;
						m_model->SetAnimationTimeSeconds(draggedEvent.time);
					}
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
		VertexBuffer m_viewportGrid;
		ComPtr<ID3D12DescriptorHeap> m_imguiSrvHeap;
		std::array<bool, ImGuiSrvDescriptorCount> m_imguiSrvDescriptorAllocated{};
		UINT m_imguiSrvDescriptorSize{};
		std::unique_ptr<SkinnedModel> m_model;
		std::optional<std::string> m_pendingModelPath;
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
		bool m_timelineDragMoved{};
		UINT m_pendingResizeWidth{ WindowWidth };
		UINT m_pendingResizeHeight{ WindowHeight };
		CameraDragMode m_cameraDragMode{ CameraDragMode::None };
		float m_playbackSpeed{ 1.0f };
		float m_cameraYaw{ XMConvertToRadians(25.0f) };
		float m_cameraPitch{ XMConvertToRadians(15.0f) };
		float m_cameraDistance{ 4.0f };
		XMFLOAT3 m_cameraTarget{ 0.0f, 1.0f, 0.0f };
		float m_timelineContextTime{};
		float m_timelineDragStartMouseX{};
		float m_timelineDragStartTime{};
		float m_modelRotation{};
	};


AnimationEventEditorApp::AnimationEventEditorApp()
	: m_impl(std::make_unique<Impl>())
{
}

AnimationEventEditorApp::~AnimationEventEditorApp() = default;

void AnimationEventEditorApp::Initialize(HWND hwnd)
{
	m_impl->Initialize(hwnd);
}

void AnimationEventEditorApp::Shutdown()
{
	m_impl->Shutdown();
}

void AnimationEventEditorApp::Tick()
{
	m_impl->Tick();
}

void AnimationEventEditorApp::OpenFbx()
{
	m_impl->OpenFbx();
}

void AnimationEventEditorApp::OpenEvents()
{
	m_impl->OpenEvents();
}

void AnimationEventEditorApp::SaveDefaultEvents()
{
	m_impl->SaveDefaultEvents();
}

void AnimationEventEditorApp::Undo()
{
	m_impl->Undo();
}

void AnimationEventEditorApp::Redo()
{
	m_impl->Redo();
}

bool AnimationEventEditorApp::CanUndo() const
{
	return m_impl->CanUndo();
}

bool AnimationEventEditorApp::CanRedo() const
{
	return m_impl->CanRedo();
}

void AnimationEventEditorApp::ResetLayout()
{
	m_impl->ResetLayout();
}

void AnimationEventEditorApp::RequestResize(UINT width, UINT height)
{
	m_impl->RequestResize(width, height);
}

void AnimationEventEditorApp::RequestClientResize()
{
	m_impl->RequestClientResize();
}
}
