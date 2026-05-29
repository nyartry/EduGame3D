#pragma once

#include "Rendering/Materials/SpriteMaterial.h"
#include "Rendering/Geometry/SpriteVertex.h"
#include "Rendering/Buffers/SpriteVertexBuffer.h"

#include <Windows.h>

#include <DirectXMath.h>
#include <string_view>
#include <vector>

class Dx12Renderer;
struct ID3D12Device;

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
	void Initialize(ID3D12Device* device, UINT maxQuadCount = 1024);
	void Clear();
	void Upload();
	void Render(Dx12Renderer& renderer) const;

	void DrawRectangle(float x, float y, float width, float height, const DirectX::XMFLOAT4& color);
	void DrawRectangle(const SpriteRect& rect, const DirectX::XMFLOAT4& color);
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

	std::vector<SpriteVertex> m_vertices;
	SpriteVertexBuffer m_vertexBuffer;
	SpriteMaterial m_whiteMaterial;
	UINT m_maxVertexCount{};
};
