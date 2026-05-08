#include "Game.h"

void Game::Initialize(HWND hwnd, UINT width, UINT height)
{
	m_renderer.Initialize(hwnd, width, height);
}

void Game::Tick()
{
	m_renderer.Update();
	m_renderer.Render();
}

void Game::WaitForGpu()
{
	m_renderer.WaitForGpu();
}

