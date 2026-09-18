#include "Framework/Rendering/Sprites/Rect.h"

Rect::Rect(std::uint32_t textureWidth, std::uint32_t textureHeight, const DirectX::XMFLOAT4& color)
	: SpriteShape(textureWidth, textureHeight, color)
{
}

bool Rect::ContainsPixel(std::uint32_t, std::uint32_t) const
{
	return true;
}
