#pragma once

#include <DirectXMath.h>

struct SpriteVertex
{
	DirectX::XMFLOAT2 position{};
	DirectX::XMFLOAT2 uv{};
	DirectX::XMFLOAT4 color{ 1.0f, 1.0f, 1.0f, 1.0f };
};
