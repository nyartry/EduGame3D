#pragma once

#include "Framework/Rendering/Sprites/Sprite.h"
#include "Framework/Rendering/Sprites/Triangle.h"

#include <DirectXMath.h>

class IRenderDevice;

class SpriteShapeFactory
{
public:
	static Sprite CreateRect(
		IRenderDevice& device,
		float x,
		float y,
		float width,
		float height,
		const DirectX::XMFLOAT4& color);

	static Sprite CreateTriangle(
		IRenderDevice& device,
		float x,
		float y,
		float width,
		float height,
		const DirectX::XMFLOAT4& color,
		TriangleDirection direction = TriangleDirection::Up);
};
