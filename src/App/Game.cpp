#include "App/Game.h"

#include "Scene/Scenes/GameScene.h"
#include "Scene/Scenes/TitleScene.h"

#include <cwchar>

void Game::Initialize(HWND hwnd, UINT width, UINT height)
{
	m_hwnd = hwnd;
	m_renderer.Initialize(hwnd, width, height);
	m_sceneManager.Initialize(m_renderer.GetDevice(), m_renderer.GetCommandQueue(), width, height);
	m_sceneManager.AddScene<GameScene>("Game");
	m_sceneManager.AddScene<TitleScene>("Title");
	m_sceneManager.LoadScene("Title", SceneLoadType::Synchronous, SceneLoadMode::Single);
	m_lastTickTime = std::chrono::steady_clock::now();
	m_fpsLastUpdate = std::chrono::steady_clock::now();
}

void Game::Tick()
{
	m_input.Update(m_hwnd);
	const float deltaTime = CalculateDeltaTime();
	m_sceneManager.Update(deltaTime, m_input);
	m_renderer.BeginFrame(m_sceneManager.GetViewProjectionMatrix());
	m_sceneManager.Render(m_renderer);
	m_renderer.EndFrame();
	UpdateDebugTitle();
}

void Game::WaitForGpu()
{
	m_renderer.WaitForGpu();
}

void Game::UpdateDebugTitle()
{
	++m_fpsFrameCount;

	const auto now = std::chrono::steady_clock::now();
	const float elapsedSeconds = std::chrono::duration<float>(now - m_fpsLastUpdate).count();
	if (elapsedSeconds < 0.5f)
	{
		return;
	}

	const float fps = static_cast<float>(m_fpsFrameCount) / elapsedSeconds;
	wchar_t title[128]{};
	swprintf_s(title, L"DirectX12 Open Campus Game - FPS: %.1f", fps);
	SetWindowText(m_hwnd, title);

	m_fpsFrameCount = 0;
	m_fpsLastUpdate = now;
}

float Game::CalculateDeltaTime()
{
	const auto now = std::chrono::steady_clock::now();
	const float deltaTime = std::chrono::duration<float>(now - m_lastTickTime).count();
	m_lastTickTime = now;
	return deltaTime;
}

