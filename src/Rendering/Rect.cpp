#include "Rendering/Rect.h"

Rect::Rect(UINT textureWidth, UINT textureHeight, const DirectX::XMFLOAT4& color)
	: PrimitiveSprite(textureWidth, textureHeight, color)
{
}

bool Rect::ContainsPixel(UINT, UINT) const
{
	return true;
}
