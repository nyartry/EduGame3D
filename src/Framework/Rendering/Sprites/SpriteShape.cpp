#include "Framework/Rendering/Sprites/SpriteShape.h"

#include "Framework/Rendering/Core/IRenderDevice.h"

#include <algorithm>
#include <cmath>

using namespace DirectX;

namespace
{
	std::uint8_t ToByte(float value)
	{
		const float clampedValue = std::clamp(value, 0.0f, 1.0f);
		return static_cast<std::uint8_t>(std::round(clampedValue * 255.0f));
	}
}

SpriteShape::SpriteShape(std::uint32_t textureWidth, std::uint32_t textureHeight, const XMFLOAT4& color)
	: m_textureWidth(std::max<std::uint32_t>(textureWidth, 1))
	, m_textureHeight(std::max<std::uint32_t>(textureHeight, 1))
	, m_color(color)
{
}

Sprite SpriteShape::CreateSprite(
	IRenderDevice& device,
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

std::uint32_t SpriteShape::GetTextureWidth() const
{
	return m_textureWidth;
}

std::uint32_t SpriteShape::GetTextureHeight() const
{
	return m_textureHeight;
}

const XMFLOAT4& SpriteShape::GetColor() const
{
	return m_color;
}

std::vector<std::uint8_t> SpriteShape::BuildPixels() const
{
	std::vector<std::uint8_t> pixels(static_cast<size_t>(m_textureWidth) * m_textureHeight * BytesPerPixel, 0);
	const std::uint8_t red = ToByte(m_color.x);
	const std::uint8_t green = ToByte(m_color.y);
	const std::uint8_t blue = ToByte(m_color.z);
	const std::uint8_t alpha = ToByte(m_color.w);

	for (std::uint32_t y = 0; y < m_textureHeight; ++y)
	{
		for (std::uint32_t x = 0; x < m_textureWidth; ++x)
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
