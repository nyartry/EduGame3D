#pragma once

#include "Framework/Rendering/Materials/SpriteMaterial.h"
#include "Framework/Rendering/Geometry/SpriteVertex.h"
#include "Framework/Rendering/Buffers/SpriteVertexBuffer.h"

#include <DirectXMath.h>
#include <cstdint>
#include <string_view>
#include <vector>

class IRenderDevice;
class IRenderer;

struct SpriteRect
{
	float x{};
	float y{};
	float width{};
	float height{};
};

class SpriteBatch
{
public:
	void Initialize(IRenderDevice& device, std::uint32_t maxQuadCount = 1024);
	void Clear();
	void Upload();
	void Render(IRenderer& renderer) const;

	void DrawRectangle(float x, float y, float width, float height, const DirectX::XMFLOAT4& color);
	void DrawRectangle(const SpriteRect& rect, const DirectX::XMFLOAT4& color);
	void DrawTriangle(
		const DirectX::XMFLOAT2& a,
		const DirectX::XMFLOAT2& b,
		const DirectX::XMFLOAT2& c,
		const DirectX::XMFLOAT4& color);
	void DrawText(std::string_view text, float x, float y, float pixelSize, const DirectX::XMFLOAT4& color);

	DirectX::XMFLOAT2 MeasureText(std::string_view text, float pixelSize) const;

private:
	void AddQuad(
		float left,
		float top,
		float right,
		float bottom,
		const DirectX::XMFLOAT4& color,
		const DirectX::XMFLOAT2& uvTopLeft = { 0.0f, 0.0f },
		const DirectX::XMFLOAT2& uvBottomRight = { 1.0f, 1.0f });
	void DrawGlyph(char glyph, float x, float y, float pixelSize, const DirectX::XMFLOAT4& color);
	bool HasRoomForQuad() const;
	bool HasRoomForTriangle() const;

	std::vector<SpriteVertex> m_vertices;
	SpriteVertexBuffer m_vertexBuffer;
	SpriteMaterial m_whiteMaterial;
	std::uint32_t m_maxVertexCount{};
};
