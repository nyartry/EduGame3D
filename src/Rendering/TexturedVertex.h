#pragma once

#include <DirectXMath.h>

struct TexturedVertex
{
	DirectX::XMFLOAT3 position{};
	DirectX::XMFLOAT3 normal{};
	DirectX::XMFLOAT3 tangent{};
	DirectX::XMFLOAT2 uv{};
	DirectX::XMFLOAT4 color{ 1.0f, 1.0f, 1.0f, 1.0f };
};
