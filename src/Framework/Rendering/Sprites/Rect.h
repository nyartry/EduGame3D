#pragma once

#include "Framework/Rendering/Sprites/SpriteShape.h"

class Rect : public SpriteShape
{
public:
	Rect(std::uint32_t textureWidth, std::uint32_t textureHeight, const DirectX::XMFLOAT4& color);

protected:
	bool ContainsPixel(std::uint32_t x, std::uint32_t y) const override;
};
