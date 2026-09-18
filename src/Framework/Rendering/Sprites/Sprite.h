#pragma once

#include "Framework/Rendering/Materials/SpriteMaterial.h"
#include "Framework/Rendering/Buffers/SpriteVertexBuffer.h"

#include <DirectXMath.h>
#include <cstdint>
#include <string>
#include <vector>

class IRenderDevice;
class IRenderer;

class Sprite
{
public:
	void Initialize(
		IRenderDevice& device,
		const std::vector<std::uint8_t>& rgbaPixels,
		std::uint32_t textureWidth,
		std::uint32_t textureHeight,
		float x,
		float y,
		float width,
		float height);
	void InitializeTexture(
		IRenderDevice& device,
		const std::string& texturePath,
		float x,
		float y,
		float width,
		float height,
		bool useSrgb = true);
	void SetPosition(float x, float y);
	void SetSize(float width, float height);
	void SetTint(const DirectX::XMFLOAT4& tint);
	void Render(IRenderer& renderer) const;

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
