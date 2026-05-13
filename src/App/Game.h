#pragma once

#include "Rendering/Dx12Renderer.h"
#include "Scene/GameScene.h"
#include "Scene/Input.h"

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
	GameScene m_scene;
	std::chrono::steady_clock::time_point m_lastTickTime{};
	std::chrono::steady_clock::time_point m_fpsLastUpdate{};
	UINT m_fpsFrameCount{};
};

