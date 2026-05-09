#include "Game.h"

void Game::Initialize(HWND hwnd, UINT width, UINT height)
{
	m_renderer.Initialize(hwnd, width, height);
	m_scene.Initialize(m_renderer.GetDevice(), width, height);
}

void Game::Tick()
{
	m_scene.Update();
	m_renderer.BeginFrame(m_scene.GetViewProjectionMatrix());
	m_scene.Render(m_renderer);
	m_renderer.EndFrame();
}

void Game::WaitForGpu()
{
	m_renderer.WaitForGpu();
}

