#include "Rendering/Sprites/SpriteShape.h"

#include <algorithm>
#include <cmath>

using namespace DirectX;

namespace
{
	UINT8 ToByte(float value)
	{
		const float clampedValue = std::clamp(value, 0.0f, 1.0f);
		return static_cast<UINT8>(std::round(clampedValue * 255.0f));
	}
}

SpriteShape::SpriteShape(UINT textureWidth, UINT textureHeight, const XMFLOAT4& color)
	: m_textureWidth(std::max<UINT>(textureWidth, 1))
	, m_textureHeight(std::max<UINT>(textureHeight, 1))
	, m_color(color)
{
}

Sprite SpriteShape::CreateSprite(
	ID3D12Device* device,
	float x,
	float y,
	float width,
	float height) const
{
	Sprite sprite;
	sprite.Initialize(
		device,
		BuildPixels(),
		m_textureWidth,
		m_textureHeight,
		x,
		y,
		width,
		height);
	return sprite;
}

UINT SpriteShape::GetTextureWidth() const
{
	return m_textureWidth;
}

UINT SpriteShape::GetTextureHeight() const
{
	return m_textureHeight;
}

const XMFLOAT4& SpriteShape::GetColor() const
{
	return m_color;
}

std::vector<UINT8> SpriteShape::BuildPixels() const
{
	std::vector<UINT8> pixels(static_cast<size_t>(m_textureWidth) * m_textureHeight * BytesPerPixel, 0);
	const UINT8 red = ToByte(m_color.x);
	const UINT8 green = ToByte(m_color.y);
	const UINT8 blue = ToByte(m_color.z);
	const UINT8 alpha = ToByte(m_color.w);

	for (UINT y = 0; y < m_textureHeight; ++y)
	{
		for (UINT x = 0; x < m_textureWidth; ++x)
		{
			if (!ContainsPixel(x, y))
			{
				continue;
			}

			const size_t offset = (static_cast<size_t>(y) * m_textureWidth + x) * BytesPerPixel;
			pixels[offset + 0] = red;
			pixels[offset + 1] = green;
			pixels[offset + 2] = blue;
			pixels[offset + 3] = alpha;
		}
	}

	return pixels;
}
