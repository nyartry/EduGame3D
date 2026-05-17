#include "Scene/LoadingOverlay.h"

#include "Rendering/Dx12Renderer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <string_view>
#include <unordered_map>

using namespace DirectX;

namespace
{
	using GlyphRows = std::array<const char*, 7>;

	const std::unordered_map<char, GlyphRows> Glyphs =
	{
		{ ' ', { "00000", "00000", "00000", "00000", "00000", "00000", "00000" } },
		{ '.', { "00000", "00000", "00000", "00000", "00000", "01100", "01100" } },
		{ 'A', { "01110", "10001", "10001", "11111", "10001", "10001", "10001" } },
		{ 'D', { "11110", "10001", "10001", "10001", "10001", "10001", "11110" } },
		{ 'G', { "01110", "10001", "10000", "10111", "10001", "10001", "01110" } },
		{ 'I', { "11111", "00100", "00100", "00100", "00100", "00100", "11111" } },
		{ 'L', { "10000", "10000", "10000", "10000", "10000", "10000", "11111" } },
		{ 'N', { "10001", "11001", "10101", "10011", "10001", "10001", "10001" } },
		{ 'O', { "01110", "10001", "10001", "10001", "10001", "10001", "01110" } },
		{ 'W', { "10001", "10001", "10001", "10101", "10101", "10101", "01010" } }
	};

	constexpr XMFLOAT4 TextColor{ 0.92f, 0.96f, 1.0f, 1.0f };
	constexpr XMFLOAT4 BarBaseColor{ 0.18f, 0.28f, 0.36f, 1.0f };
	constexpr XMFLOAT4 BarActiveColor{ 0.58f, 0.86f, 1.0f, 1.0f };
	constexpr int FrameCount = 12;

	void AddQuad(std::vector<Vertex>& vertices, float left, float top, float right, float bottom, const XMFLOAT4& color)
	{
		const Vertex topLeft{ { left, top, 0.0f }, { color.x, color.y, color.z, color.w } };
		const Vertex topRight{ { right, top, 0.0f }, { color.x, color.y, color.z, color.w } };
		const Vertex bottomLeft{ { left, bottom, 0.0f }, { color.x, color.y, color.z, color.w } };
		const Vertex bottomRight{ { right, bottom, 0.0f }, { color.x, color.y, color.z, color.w } };

		vertices.push_back(topLeft);
		vertices.push_back(bottomLeft);
		vertices.push_back(topRight);
		vertices.push_back(topRight);
		vertices.push_back(bottomLeft);
		vertices.push_back(bottomRight);
	}

	void AddGlyph(std::vector<Vertex>& vertices, char glyph, float x, float y, float pixelSize)
	{
		const auto rows = Glyphs.find(glyph);
		if (rows == Glyphs.end())
		{
			return;
		}

		for (int row = 0; row < 7; ++row)
		{
			const std::string_view rowBits = rows->second[static_cast<size_t>(row)];
			for (int column = 0; column < 5; ++column)
			{
				if (rowBits[static_cast<size_t>(column)] != '1')
				{
					continue;
				}

				const float left = x + static_cast<float>(column) * pixelSize;
				const float top = y - static_cast<float>(row) * pixelSize;
				AddQuad(vertices, left, top, left + pixelSize * 0.82f, top - pixelSize * 0.82f, TextColor);
			}
		}
	}

	std::vector<Vertex> BuildTextVertices(int dotCount)
	{
		std::string text = "NOW LOADING";
		text.append(static_cast<size_t>(dotCount), '.');

		constexpr float pixelSize = 0.023f;
		constexpr float glyphAdvance = pixelSize * 6.0f;
		const float textWidth = static_cast<float>(text.size()) * glyphAdvance - pixelSize;
		float x = -textWidth * 0.5f;
		constexpr float y = 0.08f;

		std::vector<Vertex> vertices;
		vertices.reserve(text.size() * 7 * 5 * 6);
		for (char glyph : text)
		{
			AddGlyph(vertices, glyph, x, y, pixelSize);
			x += glyphAdvance;
		}
		return vertices;
	}

	std::vector<Vertex> BuildBarVertices(int activeIndex)
	{
		constexpr int SegmentCount = 12;
		constexpr float segmentWidth = 0.065f;
		constexpr float segmentHeight = 0.035f;
		constexpr float segmentGap = 0.012f;
		const float totalWidth = static_cast<float>(SegmentCount) * segmentWidth + static_cast<float>(SegmentCount - 1) * segmentGap;
		float x = -totalWidth * 0.5f;
		constexpr float top = -0.16f;

		std::vector<Vertex> vertices;
		vertices.reserve(SegmentCount * 6);
		for (int index = 0; index < SegmentCount; ++index)
		{
			const int distance = std::min((index - activeIndex + SegmentCount) % SegmentCount, (activeIndex - index + SegmentCount) % SegmentCount);
			const XMFLOAT4& color = distance <= 1 ? BarActiveColor : BarBaseColor;
			AddQuad(vertices, x, top, x + segmentWidth, top - segmentHeight, color);
			x += segmentWidth + segmentGap;
		}
		return vertices;
	}
}

void LoadingOverlay::Initialize(ID3D12Device* device)
{
	BuildFrames(device);
}

void LoadingOverlay::Update(float deltaTime)
{
	m_elapsedTime += deltaTime;
}

void LoadingOverlay::Render(Dx12Renderer& renderer) const
{
	if (m_frames.empty())
	{
		return;
	}

	const int frameIndex = static_cast<int>(m_elapsedTime * 8.0f) % static_cast<int>(m_frames.size());
	const LoadingFrame& frame = m_frames[static_cast<size_t>(frameIndex)];
	const float pulse = (std::sin(m_elapsedTime * 5.0f) + 1.0f) * 0.5f;
	const float yOffset = 0.015f * pulse;
	const XMMATRIX world = XMMatrixTranslation(0.0f, yOffset, 0.0f);

	renderer.DrawScreen(frame.text, world);
	renderer.DrawScreen(frame.bar, XMMatrixIdentity());
}

void LoadingOverlay::BuildFrames(ID3D12Device* device)
{
	m_frames.clear();
	m_frames.resize(FrameCount);

	for (int index = 0; index < FrameCount; ++index)
	{
		const int dotCount = (index / 3) % 4;
		m_frames[static_cast<size_t>(index)].text.Initialize(device, BuildTextVertices(dotCount));
		m_frames[static_cast<size_t>(index)].bar.Initialize(device, BuildBarVertices(index));
	}
}
