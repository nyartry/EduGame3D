#pragma once

#include "Framework/Models/SkinnedModelData.h"

#include <cstdint>
#include <memory>
#include <vector>

class RenderResourceAccess;

class SkinnedVertexBuffer
{
public:
	SkinnedVertexBuffer();
	~SkinnedVertexBuffer();
	SkinnedVertexBuffer(SkinnedVertexBuffer&&) noexcept;
	SkinnedVertexBuffer& operator=(SkinnedVertexBuffer&&) noexcept;
	SkinnedVertexBuffer(const SkinnedVertexBuffer&) = delete;
	SkinnedVertexBuffer& operator=(const SkinnedVertexBuffer&) = delete;

	std::uint32_t GetVertexCount() const;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;

	friend class RenderResourceAccess;
};
