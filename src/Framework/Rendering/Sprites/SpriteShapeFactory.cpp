#include "Framework/Rendering/Sprites/SpriteShapeFactory.h"

#include "Framework/Rendering/Sprites/Rect.h"

namespace
{
	constexpr UINT DefaultPrimitiveTextureSize = 32;
}

Sprite SpriteShapeFactory::CreateRect(
	ID3D12Device* device,
	float x,
	float y,
	float width,
	float height,
	const DirectX::XMFLOAT4& color)
{
	const Rect rect(DefaultPrimitiveTextureSize, DefaultPrimitiveTextureSize, color);
	return rect.CreateSprite(device, x, y, width, height);
}

Sprite SpriteShapeFactory::CreateTriangle(
	ID3D12Device* device,
	float x,
	float y,
	float width,
	float height,
	const DirectX::XMFLOAT4& color,
	TriangleDirection direction)
{
	const Triangle triangle(DefaultPrimitiveTextureSize, DefaultPrimitiveTextureSize, color, direction);
	return triangle.CreateSprite(device, x, y, width, height);
}
