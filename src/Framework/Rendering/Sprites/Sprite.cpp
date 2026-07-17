#include "Framework/Rendering/Sprites/Sprite.h"

#include "Framework/Rendering/Core/IRenderDevice.h"
#include "Framework/Rendering/Core/IRenderer.h"

using namespace DirectX;

void Sprite::Initialize(
	IRenderDevice& device,
	const std::vector<std::uint8_t>& rgbaPixels,
	std::uint32_t textureWidth,
	std::uint32_t textureHeight,
	float x,
	float y,
	float width,
	float height)
{
	m_x = x;
	m_y = y;
	m_width = width;
	m_height = height;
	device.CreateSpriteVertexBuffer(m_vertexBuffer, 6);
	device.CreatePixelSpriteMaterial(m_material, rgbaPixels, textureWidth, textureHeight);
	m_initialized = true;
	RebuildVertices();
}

void Sprite::InitializeTexture(
	IRenderDevice& device,
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
	device.CreateSpriteVertexBuffer(m_vertexBuffer, 6);
	device.CreateTextureSpriteMaterial(m_material, texturePath, useSrgb);
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

void Sprite::Render(IRenderer& renderer) const
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
