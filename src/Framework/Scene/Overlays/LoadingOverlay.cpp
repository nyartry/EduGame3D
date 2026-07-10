#include "Framework/Scene/Overlays/LoadingOverlay.h"

#include "Framework/Rendering/Core/Dx12Renderer.h"

#include <cmath>
#include <string>
#include <string_view>

using namespace DirectX;

namespace
{
	constexpr XMFLOAT4 TextColor{ 0.92f, 0.96f, 1.0f, 1.0f };
	constexpr XMFLOAT4 BarBaseColor{ 0.18f, 0.28f, 0.36f, 1.0f };
	constexpr XMFLOAT4 BarActiveColor{ 0.58f, 0.86f, 1.0f, 1.0f };
	constexpr XMFLOAT4 DimColor{ 0.02f, 0.03f, 0.05f, 0.48f };
	constexpr int SegmentCount = 12;
}

void LoadingOverlay::Initialize(ID3D12Device* device, UINT width, UINT height)
{
	m_width = width;
	m_height = height;
	m_batch.Initialize(device, 1024);
	RebuildBatch();
}

void LoadingOverlay::Update(float deltaTime)
{
	m_elapsedTime += deltaTime;
	RebuildBatch();
}

void LoadingOverlay::Render(Dx12Renderer& renderer) const
{
	m_batch.Render(renderer);
}

void LoadingOverlay::RebuildBatch()
{
	const int frameIndex = static_cast<int>(m_elapsedTime * 8.0f) % SegmentCount;

	m_batch.Clear();
	m_batch.DrawRectangle(0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height), DimColor);

	constexpr float textPixelSize = 5.0f;
	constexpr std::string_view text = "NOW LOADING...";
	const XMFLOAT2 textSize = m_batch.MeasureText(text, textPixelSize);
	float textX = (static_cast<float>(m_width) - textSize.x) * 0.5f;
	const float textY = static_cast<float>(m_height) * 0.5f - 72.0f;
	for (size_t index = 0; index < text.size(); ++index)
	{
		const float wave = std::sin(m_elapsedTime * 7.0f + static_cast<float>(index) * 0.58f);
		const float letterPulse = (wave + 1.0f) * 0.5f;
		const XMFLOAT4 letterColor
		{
			TextColor.x,
			TextColor.y,
			TextColor.z,
			0.58f + 0.42f * letterPulse
		};
		const char letter = text[index];
		m_batch.DrawText(std::string_view(&letter, 1), textX, textY + wave * 9.0f, textPixelSize, letterColor);
		textX += 6.0f * textPixelSize;
	}

	constexpr float segmentWidth = 48.0f;
	constexpr float segmentHeight = 14.0f;
	constexpr float segmentGap = 8.0f;
	const float totalWidth = static_cast<float>(SegmentCount) * segmentWidth + static_cast<float>(SegmentCount - 1) * segmentGap;
	float x = (static_cast<float>(m_width) - totalWidth) * 0.5f;
	const float y = static_cast<float>(m_height) * 0.5f + 26.0f;

	for (int index = 0; index < SegmentCount; ++index)
	{
		const int clockwiseDistance = (index - frameIndex + SegmentCount) % SegmentCount;
		const int counterClockwiseDistance = (frameIndex - index + SegmentCount) % SegmentCount;
		const int distance = clockwiseDistance < counterClockwiseDistance ? clockwiseDistance : counterClockwiseDistance;
		const XMFLOAT4& color = distance <= 1 ? BarActiveColor : BarBaseColor;
		m_batch.DrawRectangle(x, y, segmentWidth, segmentHeight, color);
		x += segmentWidth + segmentGap;
	}

	m_batch.Upload();
}
