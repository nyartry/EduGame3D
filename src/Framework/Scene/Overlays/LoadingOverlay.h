#pragma once

#include "Framework/Rendering/Sprites/SpriteBatch.h"

#include <cstdint>

class IRenderDevice;
class IRenderer;

class LoadingOverlay
{
public:
	void Initialize(IRenderDevice& device, std::uint32_t width, std::uint32_t height);
	void Resize(std::uint32_t width, std::uint32_t height);
	void Update(float deltaTime);
	void Render(IRenderer& renderer) const;

private:
	void RebuildBatch();

	SpriteBatch m_batch;
	std::uint32_t m_width{};
	std::uint32_t m_height{};
	float m_elapsedTime{};
};
