#pragma once

#include "Framework/Rendering/Sprites/Sprite.h"

#include <DirectXMath.h>
#include <cstdint>
#include <vector>

class IRenderDevice;

class SpriteShape
{
public:
	SpriteShape(std::uint32_t textureWidth, std::uint32_t textureHeight, const DirectX::XMFLOAT4& color);
	virtual ~SpriteShape() = default;

	Sprite CreateSprite(
		IRenderDevice& device,
		float x,
		float y,
		float width,
		float height) const;

protected:
	static constexpr std::uint32_t BytesPerPixel = 4;

	virtual bool ContainsPixel(std::uint32_t x, std::uint32_t y) const = 0;

	std::uint32_t GetTextureWidth() const;
	std::uint32_t GetTextureHeight() const;
	const DirectX::XMFLOAT4& GetColor() const;

private:
	std::vector<std::uint8_t> BuildPixels() const;

	std::uint32_t m_textureWidth{};
	std::uint32_t m_textureHeight{};
	DirectX::XMFLOAT4 m_color{};
};
