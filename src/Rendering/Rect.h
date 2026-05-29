#pragma once

#include "Rendering/PrimitiveSprite.h"

class Rect : public PrimitiveSprite
{
public:
	Rect(UINT textureWidth, UINT textureHeight, const DirectX::XMFLOAT4& color);

protected:
	bool ContainsPixel(UINT x, UINT y) const override;
};
