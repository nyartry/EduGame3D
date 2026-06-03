#include "Rendering/Sprites/Sprite.h"

#include "Rendering/Core/Dx12Renderer.h"

using namespace DirectX;

void Sprite::Initialize(
	ID3D12Device* device,
	const std::vector<UINT8>& rgbaPixels,
	UINT textureWidth,
	UINT textureHeight,
	float x,
	float y,
	float width,
	float height)
{
	m_x = x;
	m_y = y;
	m_width = width;
	m_height = height;
	m_vertexBuffer.Initialize(device, 6);
	m_material.InitializePixels(device, rgbaPixels, textureWidth, textureHeight);
	m_initialized = true;
	RebuildVertices();
}

void Sprite::InitializeTexture(
	ID3D12Device* device,
	const std::string& texturePath,
	float x,
	float y,
	float width,
	float height,
	bool useSrgb)
{
	m_x = x;
	m_y = y;
	m_width = width;
	m_height = height;
	m_vertexBuffer.Initialize(device, 6);
	m_material.InitializeTexture(device, texturePath, useSrgb);
	m_initialized = true;
	RebuildVertices();
}

void Sprite::SetPosition(float x, float y)
{
	m_x = x;
	m_y = y;
	RebuildVertices();
}

void Sprite::SetSize(float width, float height)
{
	m_width = width;
	m_height = height;
	RebuildVertices();
}

void Sprite::SetTint(const XMFLOAT4& tint)
{
	m_tint = tint;
	RebuildVertices();
}

void Sprite::Render(Dx12Renderer& renderer) const
{
	if (!m_initialized)
	{
		return;
	}

	renderer.DrawSprites(m_vertexBuffer, m_material);
}

void Sprite::RebuildVertices()
{
	if (!m_initialized)
	{
		return;
	}

	const float right = m_x + m_width;
	const float bottom = m_y + m_height;
	const SpriteVertex topLeft{ { m_x, m_y }, { 0.0f, 0.0f }, m_tint };
	const SpriteVertex topRight{ { right, m_y }, { 1.0f, 0.0f }, m_tint };
	const SpriteVertex bottomLeft{ { m_x, bottom }, { 0.0f, 1.0f }, m_tint };
	const SpriteVertex bottomRight{ { right, bottom }, { 1.0f, 1.0f }, m_tint };

	const std::vector<SpriteVertex> vertices =
	{
		topLeft,
		bottomLeft,
		topRight,
		topRight,
		bottomLeft,
		bottomRight
	};
	m_vertexBuffer.Update(vertices);
}
