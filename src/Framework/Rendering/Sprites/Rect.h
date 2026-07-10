#pragma once

#include "Framework/Rendering/Sprites/SpriteShape.h"

class Rect : public SpriteShape
{
public:
	Rect(UINT textureWidth, UINT textureHeight, const DirectX::XMFLOAT4& color);

protected:
	bool ContainsPixel(UINT x, UINT y) const override;
};
