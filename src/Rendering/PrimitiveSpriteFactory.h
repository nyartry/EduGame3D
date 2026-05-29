#pragma once

#include "Rendering/SpriteImage.h"

#include <Windows.h>

struct ID3D12Device;

class PrimitiveSpriteFactory
{
public:
	static SpriteImage CreateDiamondSprite(
		ID3D12Device* device,
		float x,
		float y,
		float width,
		float height);

private:
	static constexpr UINT DiamondTextureWidth = 24;
	static constexpr UINT DiamondTextureHeight = 24;
};
