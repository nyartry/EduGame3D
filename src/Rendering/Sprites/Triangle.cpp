#include "Rendering/Sprites/Triangle.h"

#include <algorithm>

Triangle::Triangle(
	UINT textureWidth,
	UINT textureHeight,
	const DirectX::XMFLOAT4& color,
	TriangleDirection direction)
	: SpriteShape(textureWidth, textureHeight, color)
	, m_direction(direction)
{
}

bool Triangle::ContainsPixel(UINT x, UINT y) const
{
	const float denominatorX = static_cast<float>(std::max<UINT>(GetTextureWidth() - 1, 1));
	const float denominatorY = static_cast<float>(std::max<UINT>(GetTextureHeight() - 1, 1));
	float normalizedX = static_cast<float>(x) / denominatorX;
	float normalizedY = static_cast<float>(y) / denominatorY;

	switch (m_direction)
	{
	case TriangleDirection::Down:
		normalizedY = 1.0f - normalizedY;
		break;
	case TriangleDirection::Left:
		std::swap(normalizedX, normalizedY);
		break;
	case TriangleDirection::Right:
		std::swap(normalizedX, normalizedY);
		normalizedY = 1.0f - normalizedY;
		break;
	case TriangleDirection::Up:
	default:
		break;
	}

	return ContainsUpTriangle(normalizedX, normalizedY);
}

bool Triangle::ContainsUpTriangle(float normalizedX, float normalizedY) const
{
	const float halfWidthAtY = normalizedY * 0.5f;
	return normalizedX >= 0.5f - halfWidthAtY && normalizedX <= 0.5f + halfWidthAtY;
}
