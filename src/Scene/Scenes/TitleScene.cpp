#include "Scene/Scenes/TitleScene.h"

#include "Rendering/Core/Dx12Renderer.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>

using namespace DirectX;

namespace
{
	constexpr const char* NextSceneName = "Game";
	constexpr XMFLOAT4 BackgroundColor{ 0.015f, 0.025f, 0.045f, 0.88f };
	constexpr XMFLOAT4 AccentColor{ 0.26f, 0.78f, 1.0f, 0.88f };
	constexpr XMFLOAT4 AccentDarkColor{ 0.10f, 0.22f, 0.32f, 0.92f };
	constexpr XMFLOAT4 TitleColor{ 0.92f, 0.98f, 1.0f, 1.0f };
	constexpr XMFLOAT4 PromptColor{ 1.0f, 0.78f, 0.32f, 0.96f };
	constexpr XMFLOAT4 SubtleTextColor{ 0.62f, 0.74f, 0.84f, 0.92f };
}

TitleScene::TitleScene() = default;

TitleScene::~TitleScene()
{
	Unload();
}

void TitleScene::Load(const SceneLoadContext& context)
{
	m_width = context.width;
	m_height = context.height;
	m_batch.Initialize(context.device, 4096);
	RebuildBatch();
}

void TitleScene::Unload()
{
}

void TitleScene::Update(float deltaTime, const Input& input)
{
	m_elapsedTime += deltaTime;
	if (input.WasAnyPressed() || input.WasLeftMousePressed())
	{
		m_startRequested = true;
	}

	RebuildBatch();
}

void TitleScene::Render(Dx12Renderer& renderer) const
{
	m_batch.Render(renderer);
}

XMMATRIX TitleScene::GetViewProjectionMatrix() const
{
	return XMMatrixIdentity();
}

std::string TitleScene::GetRequestedSceneName() const
{
	return m_startRequested ? NextSceneName : std::string{};
}

bool TitleScene::ShouldLoadRequestedSceneAsync() const
{
	return m_startRequested;
}

void TitleScene::RebuildBatch()
{
	const float width = static_cast<float>(m_width);
	const float height = static_cast<float>(m_height);
	const float pulse = (std::sin(m_elapsedTime * 3.2f) + 1.0f) * 0.5f;
	const float titleSize = std::clamp(width / 118.0f, 4.0f, 8.0f);
	const float subtitleSize = std::clamp(width / 260.0f, 2.0f, 4.0f);
	const float promptSize = std::clamp(width / 260.0f, 2.6f, 4.4f);
	const float centerX = width * 0.5f;
	const float centerY = height * 0.5f;

	m_batch.Clear();
	m_batch.DrawRectangle(0.0f, 0.0f, width, height, BackgroundColor);
	m_batch.DrawRectangle(0.0f, height * 0.22f, width, 2.0f, AccentDarkColor);
	m_batch.DrawRectangle(0.0f, height * 0.78f, width, 2.0f, AccentDarkColor);

	const float panelWidth = std::min(width - 80.0f, 760.0f);
	const float panelHeight = std::min(height * 0.42f, 310.0f);
	const float panelX = centerX - panelWidth * 0.5f;
	const float panelY = centerY - panelHeight * 0.5f;
	m_batch.DrawRectangle(panelX, panelY, panelWidth, panelHeight, { 0.025f, 0.04f, 0.065f, 0.72f });
	m_batch.DrawRectangle(panelX, panelY, 5.0f, panelHeight, AccentColor);
	m_batch.DrawRectangle(panelX + panelWidth - 5.0f, panelY, 5.0f, panelHeight, AccentColor);

	const float markerWidth = 88.0f + 18.0f * pulse;
	m_batch.DrawRectangle(centerX - markerWidth * 0.5f, panelY + 34.0f, markerWidth, 5.0f, PromptColor);
	m_batch.DrawRectangle(centerX - markerWidth * 0.5f, panelY + panelHeight - 39.0f, markerWidth, 5.0f, PromptColor);

	DrawCenteredText("TITLE", centerY - 70.0f, titleSize, TitleColor);
	DrawCenteredText("OPEN CAMPUS GAME", centerY + 8.0f, subtitleSize, SubtleTextColor);
	DrawCenteredText("PRESS ANY KEY", centerY + 92.0f + pulse * 6.0f, promptSize, PromptColor);

	m_batch.Upload();
}

void TitleScene::DrawCenteredText(std::string_view text, float centerY, float pixelSize, const XMFLOAT4& color)
{
	const XMFLOAT2 textSize = m_batch.MeasureText(text, pixelSize);
	const float x = (static_cast<float>(m_width) - textSize.x) * 0.5f;
	const float y = centerY - textSize.y * 0.5f;
	m_batch.DrawText(text, x, y, pixelSize, color);
}
