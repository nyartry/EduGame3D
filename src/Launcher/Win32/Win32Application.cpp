#include "Launcher/Win32/Win32Application.h"

#include "Framework/Audio/AudioSystem.h"
#include "Framework/Common/Common.h"
#include "Framework/Core/Time/FixedStepClock.h"
#include "Framework/Core/Diagnostics/Diagnostics.h"
#include "Framework/Effects/Effekseer/EffekseerEffectSystem.h"
#include "Framework/Platform/Win32/Win32InputBackend.h"
#include "Framework/Rendering/Core/Dx12Renderer.h"
#include "Framework/Scene/Core/SceneManager.h"
#include "Framework/Scene/Input/Input.h"
#include "Framework/Scene/Input/SimulationInputBuffer.h"
#include "Framework/UI/RmlUi/RmlUiService.h"
#include "Game/App/Game.h"

#include <chrono>
#include <algorithm>
#include <cwchar>
#include <string>

namespace
{
	constexpr UINT WindowWidth = 1280;
	constexpr UINT WindowHeight = 720;

	struct WindowState
	{
		bool sizeChanged = true;
		bool resetTiming = false;
	};

	// Keep the HWND and its callback data alive through renderer shutdown,
	// including when Run unwinds before the outer error dialog is displayed.
	struct WindowOwner
	{
		HWND handle{};
		~WindowOwner()
		{
			if (!IsWindow(handle)) return;
			// Teardown must not leave WM_QUIT for the outer exception dialog.
			SetWindowLongPtr(handle, GWLP_USERDATA, 0);
			DestroyWindow(handle);
		}
	};

	LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
	{
		if (message == WM_NCCREATE)
		{
			const auto* create = reinterpret_cast<const CREATESTRUCT*>(lParam);
			SetWindowLongPtr(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
		}
		auto* state = reinterpret_cast<WindowState*>(GetWindowLongPtr(window, GWLP_USERDATA));
		switch (message)
		{
		case WM_SIZE:
			if (state != nullptr)
			{
				// GPU/UI work stays outside this Windows callback. Coalesce size
				// messages and read the actual client rectangle at the next frame.
				state->sizeChanged = true;
				state->resetTiming = true;
			}
			return 0;
		case WM_ENTERSIZEMOVE:
		case WM_EXITSIZEMOVE:
			if (state != nullptr) state->resetTiming = true;
			return 0;
		case WM_NCDESTROY:
			SetWindowLongPtr(window, GWLP_USERDATA, 0);
			return DefWindowProc(window, message, wParam, lParam);
		case WM_DESTROY:
			if (state != nullptr) PostQuitMessage(0);
			return 0;
		case WM_KEYDOWN:
			if (wParam == VK_ESCAPE)
			{
				PostMessage(window, WM_CLOSE, 0, 0);
			}
			return 0;
		default:
			return DefWindowProc(window, message, wParam, lParam);
		}
	}

	void UpdateDebugTitle(HWND window, float elapsedSeconds, UINT& frameCount)
	{
		if (elapsedSeconds < 0.5f)
		{
			return;
		}
		const float fps = static_cast<float>(frameCount) / elapsedSeconds;
		wchar_t title[128]{};
		swprintf_s(title, L"DirectX12 Open Campus Game - FPS: %.1f", fps);
		SetWindowText(window, title);
		frameCount = 0;
	}
}

int Win32Application::Run(HINSTANCE instance, int showCommand)
{
	Diagnostics::SetSink([](std::string_view message)
	{
		const std::string line = std::string(message) + "\n";
		OutputDebugStringA(line.c_str());
	});
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
	WindowState windowState;
	WindowOwner ownedWindow;
	ownedWindow.handle = CreateWindowEx(
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
		&windowState);
	const HWND window = ownedWindow.handle;
	if (window == nullptr)
	{
		return 1;
	}

	AudioSystem audio;
	Dx12Renderer renderer;
	EffekseerEffectSystem effects;
	RmlUiService ui;
	Game game(renderer, audio, effects, effects, ui, WindowWidth, WindowHeight);
	SceneManager scenes;
	Input input;
	Win32InputBackend inputBackend;
	FixedStepClock simulationClock;
	SimulationInputBuffer simulationInput;
	RenderShutdownGuard shutdown(renderer);

	renderer.Initialize(window, WindowWidth, WindowHeight);
	effects.Initialize(renderer);
	game.RegisterContent();
	audio.Initialize();
	ui.Initialize();
	scenes.Initialize(renderer, renderer, WindowWidth, WindowHeight);
	game.RegisterScenes(scenes);
	if (!scenes.LoadScene(std::string(game.GetInitialSceneName()), SceneLoadType::Synchronous, SceneLoadMode::Single))
	{
		// SceneManager already logged the cause. At startup there is no old
		// scene to resume, so show the error and stop before entering the loop.
		const std::string error = "Initial scene load failed:\n" + scenes.GetLastLoadError();
		MessageBoxW(nullptr, ToWide(error.c_str()).c_str(), L"Startup error", MB_OK | MB_ICONERROR);
		return 1;
	}

	ShowWindow(window, showCommand);
	auto lastTickTime = std::chrono::steady_clock::now();
	auto fpsLastUpdate = lastTickTime;
	UINT fpsFrameCount = 0;
	bool wasFocused = GetForegroundWindow() == window;
	MSG message{};
	while (message.message != WM_QUIT)
	{
		if (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&message);
			DispatchMessage(&message);
			continue;
		}

		RECT clientRect{};
		if (!GetClientRect(window, &clientRect))
		{
			throw std::runtime_error("Cannot read the game window client dimensions.");
		}
		const UINT width = static_cast<UINT>(clientRect.right - clientRect.left);
		const UINT height = static_cast<UINT>(clientRect.bottom - clientRect.top);
		if (IsIconic(window) || width == 0 || height == 0)
		{
			// Release UI input on suspension, retaining the last positive
			// viewport. No frame or fixed update is submitted while minimized.
			inputBackend.Update(nullptr, input);
			scenes.UpdateFrame(0.0f, input);
			audio.Update();
			simulationClock.Reset();
			simulationInput.Reset();
			wasFocused = false;
			windowState.resetTiming = true;
			WaitMessage();
			continue;
		}
		if (windowState.sizeChanged)
		{
			renderer.Resize(width, height);
			scenes.Resize(width, height);
			windowState.sizeChanged = false;
		}

		inputBackend.Update(window, input);
		const auto now = std::chrono::steady_clock::now();
		const double elapsedSeconds = windowState.resetTiming ? 0.0 : std::chrono::duration<double>(now - lastTickTime).count();
		lastTickTime = now;
		const bool focused = GetForegroundWindow() == window && !IsIconic(window);
		if (!focused || focused != wasFocused || windowState.resetTiming)
		{
			simulationClock.Reset();
			simulationInput.Reset();
		}
		if (windowState.resetTiming)
		{
			fpsLastUpdate = now;
			fpsFrameCount = 0;
			windowState.resetTiming = false;
		}
		const unsigned stepCount = focused && wasFocused ? simulationClock.Advance(elapsedSeconds) : 0;
		wasFocused = focused;
		if (focused) simulationInput.Capture(input);
		for (unsigned step = 0; step < stepCount; ++step)
		{
			const Input stepInput = simulationInput.ConsumeStep();
			scenes.Update(simulationClock.GetStepSeconds(), stepInput);
			effects.Update(simulationClock.GetStepSeconds());
		}
		scenes.UpdateFrame(static_cast<float>(std::min(elapsedSeconds, 0.25)), input);
		audio.Update();
		const RenderView renderView = scenes.GetRenderView();
		renderer.BeginFrame(renderView.GetViewProjection());
		scenes.RenderWorld(renderer);
		if (renderView.effectsEnabled)
		{
			effects.Render(renderer, renderView.view, renderView.projection);
		}
		scenes.RenderOverlay(renderer);
		renderer.EndFrame();

		++fpsFrameCount;
		const float fpsElapsed = std::chrono::duration<float>(now - fpsLastUpdate).count();
		if (fpsElapsed >= 0.5f)
		{
			UpdateDebugTitle(window, fpsElapsed, fpsFrameCount);
			fpsLastUpdate = now;
		}
	}

	return static_cast<int>(message.wParam);
}
