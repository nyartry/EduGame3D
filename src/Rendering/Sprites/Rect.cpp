#include "Rendering/Sprites/Rect.h"

Rect::Rect(UINT textureWidth, UINT textureHeight, const DirectX::XMFLOAT4& color)
	: SpriteShape(textureWidth, textureHeight, color)
{
}

bool Rect::ContainsPixel(UINT, UINT) const
{
	return true;
}
