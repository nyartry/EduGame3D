#pragma once

#include "Framework/Rendering/Sprites/Sprite.h"
#include "Framework/Rendering/Sprites/Triangle.h"

#include <Windows.h>

#include <DirectXMath.h>

struct ID3D12Device;

class SpriteShapeFactory
{
public:
	static Sprite CreateRect(
		ID3D12Device* device,
		float x,
		float y,
		float width,
		float height,
		const DirectX::XMFLOAT4& color);

	static Sprite CreateTriangle(
		ID3D12Device* device,
		float x,
		float y,
		float width,
		float height,
		const DirectX::XMFLOAT4& color,
		TriangleDirection direction = TriangleDirection::Up);
};
