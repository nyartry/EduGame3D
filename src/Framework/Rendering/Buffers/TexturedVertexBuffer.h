#pragma once

#include "Framework/Rendering/Geometry/TexturedVertex.h"

#include <cstdint>
#include <memory>
#include <vector>

class RenderResourceAccess;

class TexturedVertexBuffer
{
public:
	TexturedVertexBuffer();
	~TexturedVertexBuffer();
	TexturedVertexBuffer(TexturedVertexBuffer&&) noexcept;
	TexturedVertexBuffer& operator=(TexturedVertexBuffer&&) noexcept;
	TexturedVertexBuffer(const TexturedVertexBuffer&) = delete;
	TexturedVertexBuffer& operator=(const TexturedVertexBuffer&) = delete;

	void Update(const std::vector<TexturedVertex>& vertices);

	std::uint32_t GetVertexCount() const;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;

	friend class RenderResourceAccess;
};
