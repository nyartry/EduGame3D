#pragma once

#include "Rendering/Sprites/Sprite.h"

#include <Windows.h>

#include <DirectXMath.h>
#include <vector>

struct ID3D12Device;

class SpriteShape
{
public:
	SpriteShape(UINT textureWidth, UINT textureHeight, const DirectX::XMFLOAT4& color);
	virtual ~SpriteShape() = default;

	Sprite CreateSprite(
		ID3D12Device* device,
		float x,
		float y,
		float width,
		float height) const;

protected:
	static constexpr UINT BytesPerPixel = 4;

	virtual bool ContainsPixel(UINT x, UINT y) const = 0;

	UINT GetTextureWidth() const;
	UINT GetTextureHeight() const;
	const DirectX::XMFLOAT4& GetColor() const;

private:
	std::vector<UINT8> BuildPixels() const;

	UINT m_textureWidth{};
	UINT m_textureHeight{};
	DirectX::XMFLOAT4 m_color{};
};
