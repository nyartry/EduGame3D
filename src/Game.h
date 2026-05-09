#pragma once

#include "Dx12Renderer.h"
#include "GameScene.h"

#include <Windows.h>

class Game
{
public:
	void Initialize(HWND hwnd, UINT width, UINT height);
	void Tick();
	void WaitForGpu();

private:
	Dx12Renderer m_renderer;
	GameScene m_scene;
};

