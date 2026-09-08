#include "AnimationEventEditorApp.h"
#include "Framework/Common/Common.h"
#include "Framework/Core/Diagnostics/Diagnostics.h"
#include "Framework/Core/Diagnostics/ExceptionUtils.h"

#include <backends/imgui_impl_win32.h>
#include <imgui.h>

#include <Windows.h>

using namespace AnimationEventEditorTool;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

namespace
{
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

int APIENTRY wWinMain(
	_In_ HINSTANCE instance,
	_In_opt_ HINSTANCE,
	_In_ PWSTR,
	_In_ int showCommand)
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

		RECT windowRect =
		{
			0,
			0,
			static_cast<LONG>(AnimationEventEditorApp::DefaultWindowWidth),
			static_cast<LONG>(AnimationEventEditorApp::DefaultWindowHeight)
		};
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
	catch (...)
	{
		const auto message = DescribeException();
		Diagnostics::Write(message);
		MessageBoxW(nullptr, ToWide(message.c_str()).c_str(), L"Animation Event Editor Error", MB_OK | MB_ICONERROR);
		return 1;
	}
}
