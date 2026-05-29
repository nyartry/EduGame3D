#pragma once

#include "Rendering/SpriteImage.h"
#include "Rendering/Triangle.h"

#include <Windows.h>

#include <DirectXMath.h>

struct ID3D12Device;

class PrimitiveSpriteFactory
{
public:
	static SpriteImage CreateRect(
		ID3D12Device* device,
		float x,
		float y,
		float width,
		float height,
		const DirectX::XMFLOAT4& color);

	static SpriteImage CreateTriangle(
		ID3D12Device* device,
		float x,
		float y,
		float width,
		float height,
		const DirectX::XMFLOAT4& color,
		TriangleDirection direction = TriangleDirection::Up);
};
