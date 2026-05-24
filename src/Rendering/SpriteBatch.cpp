#include "Rendering/SpriteBatch.h"

#include "Rendering/Dx12Renderer.h"

#include <array>
#include <cctype>
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
		{ ':', { "00000", "01100", "01100", "00000", "01100", "01100", "00000" } },
		{ '-', { "00000", "00000", "00000", "11111", "00000", "00000", "00000" } },
		{ '/', { "00001", "00010", "00100", "00100", "01000", "10000", "00000" } },
		{ '0', { "01110", "10001", "10011", "10101", "11001", "10001", "01110" } },
		{ '1', { "00100", "01100", "00100", "00100", "00100", "00100", "01110" } },
		{ '2', { "01110", "10001", "00001", "00010", "00100", "01000", "11111" } },
		{ '3', { "11110", "00001", "00001", "01110", "00001", "00001", "11110" } },
		{ '4', { "00010", "00110", "01010", "10010", "11111", "00010", "00010" } },
		{ '5', { "11111", "10000", "10000", "11110", "00001", "00001", "11110" } },
		{ '6', { "01110", "10000", "10000", "11110", "10001", "10001", "01110" } },
		{ '7', { "11111", "00001", "00010", "00100", "01000", "01000", "01000" } },
		{ '8', { "01110", "10001", "10001", "01110", "10001", "10001", "01110" } },
		{ '9', { "01110", "10001", "10001", "01111", "00001", "00001", "01110" } },
		{ 'A', { "01110", "10001", "10001", "11111", "10001", "10001", "10001" } },
		{ 'B', { "11110", "10001", "10001", "11110", "10001", "10001", "11110" } },
		{ 'C', { "01110", "10001", "10000", "10000", "10000", "10001", "01110" } },
		{ 'D', { "11110", "10001", "10001", "10001", "10001", "10001", "11110" } },
		{ 'E', { "11111", "10000", "10000", "11110", "10000", "10000", "11111" } },
		{ 'F', { "11111", "10000", "10000", "11110", "10000", "10000", "10000" } },
		{ 'G', { "01110", "10001", "10000", "10111", "10001", "10001", "01110" } },
		{ 'H', { "10001", "10001", "10001", "11111", "10001", "10001", "10001" } },
		{ 'I', { "11111", "00100", "00100", "00100", "00100", "00100", "11111" } },
		{ 'J', { "00111", "00010", "00010", "00010", "00010", "10010", "01100" } },
		{ 'K', { "10001", "10010", "10100", "11000", "10100", "10010", "10001" } },
		{ 'L', { "10000", "10000", "10000", "10000", "10000", "10000", "11111" } },
		{ 'M', { "10001", "11011", "10101", "10101", "10001", "10001", "10001" } },
		{ 'N', { "10001", "11001", "10101", "10011", "10001", "10001", "10001" } },
		{ 'O', { "01110", "10001", "10001", "10001", "10001", "10001", "01110" } },
		{ 'P', { "11110", "10001", "10001", "11110", "10000", "10000", "10000" } },
		{ 'Q', { "01110", "10001", "10001", "10001", "10101", "10010", "01101" } },
		{ 'R', { "11110", "10001", "10001", "11110", "10100", "10010", "10001" } },
		{ 'S', { "01111", "10000", "10000", "01110", "00001", "00001", "11110" } },
		{ 'T', { "11111", "00100", "00100", "00100", "00100", "00100", "00100" } },
		{ 'U', { "10001", "10001", "10001", "10001", "10001", "10001", "01110" } },
		{ 'V', { "10001", "10001", "10001", "10001", "10001", "01010", "00100" } },
		{ 'W', { "10001", "10001", "10001", "10101", "10101", "10101", "01010" } },
		{ 'X', { "10001", "10001", "01010", "00100", "01010", "10001", "10001" } },
		{ 'Y', { "10001", "10001", "01010", "00100", "00100", "00100", "00100" } },
		{ 'Z', { "11111", "00001", "00010", "00100", "01000", "10000", "11111" } }
	};

	constexpr float GlyphWidth = 5.0f;
	constexpr float GlyphHeight = 7.0f;
	constexpr float GlyphAdvance = 6.0f;

	char NormalizeGlyph(char glyph)
	{
		return static_cast<char>(std::toupper(static_cast<unsigned char>(glyph)));
	}
}

void SpriteBatch::Initialize(ID3D12Device* device, UINT maxQuadCount)
{
	m_maxVertexCount = maxQuadCount * 6;
	m_vertices.clear();
	m_vertices.reserve(m_maxVertexCount);
	m_vertexBuffer.Initialize(device, m_maxVertexCount);
	m_whiteMaterial.InitializeSolidColor(device, 255, 255, 255, 255);
}

void SpriteBatch::Clear()
{
	m_vertices.clear();
}

void SpriteBatch::Upload()
{
	m_vertexBuffer.Update(m_vertices);
}

void SpriteBatch::Render(Dx12Renderer& renderer) const
{
	if (m_vertexBuffer.GetVertexCount() == 0)
	{
		return;
	}

	renderer.DrawSprites(m_vertexBuffer, m_whiteMaterial);
}

void SpriteBatch::DrawRectangle(float x, float y, float width, float height, const XMFLOAT4& color)
{
	DrawRectangle({ x, y, width, height }, color);
}

void SpriteBatch::DrawRectangle(const SpriteRect& rect, const XMFLOAT4& color)
{
	AddQuad(rect.x, rect.y, rect.x + rect.width, rect.y + rect.height, color);
}

void SpriteBatch::DrawText(std::string_view text, float x, float y, float pixelSize, const XMFLOAT4& color)
{
	float cursorX = x;
	float cursorY = y;
	const float startX = x;
	const float advance = GlyphAdvance * pixelSize;
	for (char glyph : text)
	{
		if (glyph == '\n')
		{
			cursorX = startX;
			cursorY += (GlyphHeight + 1.0f) * pixelSize;
			continue;
		}

		DrawGlyph(NormalizeGlyph(glyph), cursorX, cursorY, pixelSize, color);
		cursorX += advance;
	}
}

XMFLOAT2 SpriteBatch::MeasureText(std::string_view text, float pixelSize) const
{
	float lineWidth = 0.0f;
	float maxWidth = 0.0f;
	float lineCount = 1.0f;
	const float advance = GlyphAdvance * pixelSize;

	for (char glyph : text)
	{
		if (glyph == '\n')
		{
			maxWidth = maxWidth > lineWidth ? maxWidth : lineWidth;
			lineWidth = 0.0f;
			lineCount += 1.0f;
			continue;
		}

		lineWidth += advance;
	}

	maxWidth = maxWidth > lineWidth ? maxWidth : lineWidth;
	if (maxWidth > 0.0f)
	{
		maxWidth -= pixelSize;
	}

	const float height = lineCount * GlyphHeight * pixelSize + (lineCount - 1.0f) * pixelSize;
	return { maxWidth, height };
}

void SpriteBatch::AddQuad(
	float left,
	float top,
	float right,
	float bottom,
	const XMFLOAT4& color,
	const XMFLOAT2& uvTopLeft,
	const XMFLOAT2& uvBottomRight)
{
	if (!HasRoomForQuad())
	{
		return;
	}

	const SpriteVertex topLeft{ { left, top }, { uvTopLeft.x, uvTopLeft.y }, color };
	const SpriteVertex topRight{ { right, top }, { uvBottomRight.x, uvTopLeft.y }, color };
	const SpriteVertex bottomLeft{ { left, bottom }, { uvTopLeft.x, uvBottomRight.y }, color };
	const SpriteVertex bottomRight{ { right, bottom }, { uvBottomRight.x, uvBottomRight.y }, color };

	m_vertices.push_back(topLeft);
	m_vertices.push_back(bottomLeft);
	m_vertices.push_back(topRight);
	m_vertices.push_back(topRight);
	m_vertices.push_back(bottomLeft);
	m_vertices.push_back(bottomRight);
}

void SpriteBatch::DrawGlyph(char glyph, float x, float y, float pixelSize, const XMFLOAT4& color)
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
			const float top = y + static_cast<float>(row) * pixelSize;
			AddQuad(left, top, left + pixelSize * 0.86f, top + pixelSize * 0.86f, color);
		}
	}
}

bool SpriteBatch::HasRoomForQuad() const
{
	return m_vertices.size() + 6 <= m_maxVertexCount;
}
