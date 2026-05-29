#pragma once

#include "Rendering/Core/Dx12Renderer.h"
#include "Scene/Input/Input.h"
#include "Scene/Core/SceneManager.h"

#include <Windows.h>

#include <chrono>

class Game
{
public:
	void Initialize(HWND hwnd, UINT width, UINT height);
	void Tick();
	void WaitForGpu();

private:
	void UpdateDebugTitle();
	float CalculateDeltaTime();

	HWND m_hwnd{};
	Input m_input;
	Dx12Renderer m_renderer;
	SceneManager m_sceneManager;
	std::chrono::steady_clock::time_point m_lastTickTime{};
	std::chrono::steady_clock::time_point m_fpsLastUpdate{};
	UINT m_fpsFrameCount{};
};

