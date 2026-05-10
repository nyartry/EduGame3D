#pragma once

#include <DirectXMath.h>

enum class RootMotionMode
{
	Ignore,
	Apply,
	Blend,
};

struct RootMotionDelta
{
	DirectX::XMFLOAT3 translation{ 0.0f, 0.0f, 0.0f };
};
