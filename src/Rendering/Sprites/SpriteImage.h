#pragma once

#include "Rendering/Materials/SpriteMaterial.h"
#include "Rendering/Buffers/SpriteVertexBuffer.h"

#include <Windows.h>

#include <DirectXMath.h>
#include <vector>

class Dx12Renderer;
struct ID3D12Device;

class SpriteImage
{
public:
	void Initialize(
		ID3D12Device* device,
		const std::vector<UINT8>& rgbaPixels,
		UINT textureWidth,
		UINT textureHeight,
		float x,
		float y,
		float width,
		float height);
	void SetPosition(float x, float y);
	void SetSize(float width, float height);
	void SetTint(const DirectX::XMFLOAT4& tint);
	void Render(Dx12Renderer& renderer) const;

private:
	void RebuildVertices();

	SpriteVertexBuffer m_vertexBuffer;
	SpriteMaterial m_material;
	DirectX::XMFLOAT4 m_tint{ 1.0f, 1.0f, 1.0f, 1.0f };
	float m_x{};
	float m_y{};
	float m_width{};
	float m_height{};
	bool m_initialized{};
};
