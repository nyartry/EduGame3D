#pragma once

#include "Framework/Rendering/Sprites/SpriteShape.h"

enum class TriangleDirection
{
	Up,
	Down,
	Left,
	Right
};

class Triangle : public SpriteShape
{
public:
	Triangle(
		std::uint32_t textureWidth,
		std::uint32_t textureHeight,
		const DirectX::XMFLOAT4& color,
		TriangleDirection direction = TriangleDirection::Up);

protected:
	bool ContainsPixel(std::uint32_t x, std::uint32_t y) const override;

private:
	bool ContainsUpTriangle(float normalizedX, float normalizedY) const;

	TriangleDirection m_direction{};
};
