#pragma once

#include "Framework/Rendering/Geometry/Vertex.h"

#include <cstdint>
#include <memory>
#include <vector>

class RenderResourceAccess;

class VertexBuffer
{
public:
	VertexBuffer();
	~VertexBuffer();
	VertexBuffer(VertexBuffer&&) noexcept;
	VertexBuffer& operator=(VertexBuffer&&) noexcept;
	VertexBuffer(const VertexBuffer&) = delete;
	VertexBuffer& operator=(const VertexBuffer&) = delete;

	std::uint32_t GetVertexCount() const;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;

	friend class RenderResourceAccess;
};
