#include "Rendering/PrimitiveSpriteFactory.h"

#include "Rendering/Rect.h"

namespace
{
	constexpr UINT DefaultPrimitiveTextureSize = 32;
}

SpriteImage PrimitiveSpriteFactory::CreateRect(
	ID3D12Device* device,
	float x,
	float y,
	float width,
	float height,
	const DirectX::XMFLOAT4& color)
{
	const Rect rect(DefaultPrimitiveTextureSize, DefaultPrimitiveTextureSize, color);
	return rect.CreateImage(device, x, y, width, height);
}

SpriteImage PrimitiveSpriteFactory::CreateTriangle(
	ID3D12Device* device,
	float x,
	float y,
	float width,
	float height,
	const DirectX::XMFLOAT4& color,
	TriangleDirection direction)
{
	const Triangle triangle(DefaultPrimitiveTextureSize, DefaultPrimitiveTextureSize, color, direction);
	return triangle.CreateImage(device, x, y, width, height);
}
