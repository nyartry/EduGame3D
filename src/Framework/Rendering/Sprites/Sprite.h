#pragma once

#include "Framework/Rendering/Materials/SpriteMaterial.h"
#include "Framework/Rendering/Buffers/SpriteVertexBuffer.h"

#include <Windows.h>

#include <DirectXMath.h>
#include <string>
#include <vector>

class Dx12Renderer;
struct ID3D12Device;

class Sprite
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
	void InitializeTexture(
		ID3D12Device* device,
		const std::string& texturePath,
		float x,
		float y,
		float width,
		float height,
		bool useSrgb = true);
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
