#include "Game/UI/HudOverlay.h"

#include "Framework/Rendering/Core/IRenderDevice.h"
#include "Framework/Rendering/Core/IRenderer.h"

#include <cmath>

using namespace DirectX;

namespace
{
	constexpr XMFLOAT4 PanelColor{ 0.02f, 0.04f, 0.06f, 0.58f };
	constexpr XMFLOAT4 PanelAccentColor{ 0.55f, 0.86f, 1.0f, 0.95f };
	constexpr XMFLOAT4 HealthColor{ 0.30f, 0.92f, 0.50f, 0.95f };
	constexpr XMFLOAT4 StaminaColor{ 1.0f, 0.76f, 0.24f, 0.95f };
	constexpr XMFLOAT4 BarBackColor{ 0.08f, 0.11f, 0.14f, 0.88f };
	constexpr XMFLOAT4 TextColor{ 0.92f, 0.97f, 1.0f, 0.95f };
	constexpr XMFLOAT4 ReticleColor{ 0.92f, 0.98f, 1.0f, 0.82f };
}

void HudOverlay::Initialize(IRenderDevice& device, std::uint32_t width, std::uint32_t height)
{
	m_width = width;
	m_height = height;
	m_batch.Initialize(device, 384);
	m_initialized = true;
	RebuildBatch();
}

void HudOverlay::Resize(std::uint32_t width, std::uint32_t height)
{
	if (width == 0 || height == 0)
	{
		return;
	}
	m_width = width;
	m_height = height;
	if (m_initialized)
	{
		RebuildBatch();
	}
}

void HudOverlay::Update(float deltaTime)
{
	m_elapsedTime += deltaTime;
	RebuildBatch();
}

void HudOverlay::Render(IRenderer& renderer) const
{
	m_batch.Render(renderer);
}

void HudOverlay::RebuildBatch()
{
	const float pulse = (std::sin(m_elapsedTime * 2.4f) + 1.0f) * 0.5f;
	const float staminaFill = 0.72f + 0.18f * pulse;

	m_batch.Clear();

	m_batch.DrawRectangle(24.0f, 24.0f, 254.0f, 78.0f, PanelColor);
	m_batch.DrawRectangle(24.0f, 24.0f, 4.0f, 78.0f, PanelAccentColor);
	m_batch.DrawText("HP", 42.0f, 40.0f, 3.0f, TextColor);
	m_batch.DrawRectangle(82.0f, 40.0f, 168.0f, 12.0f, BarBackColor);
	m_batch.DrawRectangle(82.0f, 40.0f, 146.0f, 12.0f, HealthColor);

	m_batch.DrawText("SP", 42.0f, 68.0f, 3.0f, TextColor);
	m_batch.DrawRectangle(82.0f, 68.0f, 168.0f, 12.0f, BarBackColor);
	m_batch.DrawRectangle(82.0f, 68.0f, 168.0f * staminaFill, 12.0f, StaminaColor);

	const float centerX = static_cast<float>(m_width) * 0.5f;
	const float centerY = static_cast<float>(m_height) * 0.5f;
	m_batch.DrawRectangle(centerX - 18.0f, centerY - 1.0f, 12.0f, 2.0f, ReticleColor);
	m_batch.DrawRectangle(centerX + 6.0f, centerY - 1.0f, 12.0f, 2.0f, ReticleColor);
	m_batch.DrawRectangle(centerX - 1.0f, centerY - 18.0f, 2.0f, 12.0f, ReticleColor);
	m_batch.DrawRectangle(centerX - 1.0f, centerY + 6.0f, 2.0f, 12.0f, ReticleColor);

	constexpr float labelSize = 3.0f;
	const XMFLOAT2 labelSizePx = m_batch.MeasureText("EDUGAME3D", labelSize);
	m_batch.DrawText(
		"EDUGAME3D",
		static_cast<float>(m_width) - labelSizePx.x - 28.0f,
		30.0f,
		labelSize,
		TextColor);

	m_batch.Upload();
}
