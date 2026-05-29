#pragma once

#include "Rendering/Sprites/PrimitiveSprite.h"

enum class TriangleDirection
{
	Up,
	Down,
	Left,
	Right
};

class Triangle : public PrimitiveSprite
{
public:
	Triangle(
		UINT textureWidth,
		UINT textureHeight,
		const DirectX::XMFLOAT4& color,
		TriangleDirection direction = TriangleDirection::Up);

protected:
	bool ContainsPixel(UINT x, UINT y) const override;

private:
	bool ContainsUpTriangle(float normalizedX, float normalizedY) const;

	TriangleDirection m_direction{};
};
