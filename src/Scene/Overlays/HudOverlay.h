#pragma once

#include "Rendering/Sprites/SpriteBatch.h"

#include <Windows.h>

class Dx12Renderer;
struct ID3D12Device;

class HudOverlay
{
public:
	void Initialize(ID3D12Device* device, UINT width, UINT height);
	void Update(float deltaTime);
	void Render(Dx12Renderer& renderer) const;

private:
	void RebuildBatch();

	SpriteBatch m_batch;
	UINT m_width{};
	UINT m_height{};
	float m_elapsedTime{};
};
