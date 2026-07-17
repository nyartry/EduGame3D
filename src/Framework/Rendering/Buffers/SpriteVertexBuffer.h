#pragma once

#include "Framework/Rendering/Geometry/SpriteVertex.h"

#include <cstdint>
#include <memory>
#include <vector>

class RenderResourceAccess;

class SpriteVertexBuffer
{
public:
	SpriteVertexBuffer();
	~SpriteVertexBuffer();
	SpriteVertexBuffer(SpriteVertexBuffer&&) noexcept;
	SpriteVertexBuffer& operator=(SpriteVertexBuffer&&) noexcept;
	SpriteVertexBuffer(const SpriteVertexBuffer&) = delete;
	SpriteVertexBuffer& operator=(const SpriteVertexBuffer&) = delete;

	void Update(const std::vector<SpriteVertex>& vertices);

	std::uint32_t GetVertexCount() const;
	std::uint32_t GetVertexCapacity() const;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;

	friend class RenderResourceAccess;
};
