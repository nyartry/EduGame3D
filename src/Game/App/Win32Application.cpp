#include "Game/App/Win32Application.h"

#include "Game/App/Game.h"

namespace
{
	constexpr UINT WindowWidth = 1280;
	constexpr UINT WindowHeight = 720;

	LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_CREATE:
		{
			const auto createStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
			SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));
			return 0;
		}
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		case WM_KEYDOWN:
			if (wParam == VK_ESCAPE)
			{
				PostMessage(hwnd, WM_CLOSE, 0, 0);
			}
			return 0;
		default:
			return DefWindowProc(hwnd, message, wParam, lParam);
		}
	}
}

int Win32Application::Run(HINSTANCE instance, int showCommand)
{
	Game game;

	const wchar_t className[] = L"DirectX12OpenCampusWindow";
	WNDCLASSEX windowClass{};
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = WindowProc;
	windowClass.hInstance = instance;
	windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	windowClass.lpszClassName = className;
	RegisterClassEx(&windowClass);

	RECT windowRect = { 0, 0, static_cast<LONG>(WindowWidth), static_cast<LONG>(WindowHeight) };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	HWND hwnd = CreateWindowEx(
		0,
		className,
		L"DirectX12 Open Campus Game",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		nullptr,
		nullptr,
		instance,
		&game);

	if (hwnd == nullptr)
	{
		return 1;
	}

	game.Initialize(hwnd, WindowWidth, WindowHeight);
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
			game.Tick();
		}
	}

	game.WaitForGpu();
	return static_cast<int>(message.wParam);
}

