#include "Launcher/Win32/Win32Application.h"

#include "Framework/Audio/AudioSystem.h"
#include "Framework/Effects/Effekseer/EffekseerEffectSystem.h"
#include "Framework/Platform/Win32/Win32InputBackend.h"
#include "Framework/Rendering/Core/Dx12Renderer.h"
#include "Framework/Scene/Core/SceneManager.h"
#include "Framework/Scene/Input/Input.h"
#include "Framework/UI/RmlUi/RmlUiService.h"
#include "Game/App/Game.h"

#include <chrono>
#include <cwchar>
#include <string>

namespace
{
	constexpr UINT WindowWidth = 1280;
	constexpr UINT WindowHeight = 720;

	LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_DESTROY:
			PostQuitMessage(0);
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
	HWND window = CreateWindowEx(
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
		nullptr);
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

	renderer.Initialize(window, WindowWidth, WindowHeight);
	effects.Initialize(renderer);
	game.RegisterContent();
	audio.Initialize();
	ui.Initialize();
	scenes.Initialize(renderer, renderer, WindowWidth, WindowHeight);
	game.RegisterScenes(scenes);
	scenes.LoadScene(std::string(game.GetInitialSceneName()), SceneLoadType::Synchronous, SceneLoadMode::Single);

	ShowWindow(window, showCommand);
	auto lastTickTime = std::chrono::steady_clock::now();
	auto fpsLastUpdate = lastTickTime;
	UINT fpsFrameCount = 0;
	MSG message{};
	while (message.message != WM_QUIT)
	{
		if (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&message);
			DispatchMessage(&message);
			continue;
		}

		inputBackend.Update(window, input);
		const auto now = std::chrono::steady_clock::now();
		const float deltaTime = std::chrono::duration<float>(now - lastTickTime).count();
		lastTickTime = now;
		scenes.Update(deltaTime, input);
		audio.Update();
		effects.Update(deltaTime);
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

	renderer.WaitForGpu();
	return static_cast<int>(message.wParam);
}
