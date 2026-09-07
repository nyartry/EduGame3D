#include "Framework/Rendering/Buffers/SpriteVertexBuffer.h"

#include "Framework/Rendering/Core/FrameUploadBuffer.h"
#include "Framework/Rendering/Core/RenderResourceAccess.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <utility>

struct SpriteVertexBuffer::Impl
{
	std::vector<SpriteVertex> vertices;
	std::uint32_t capacity{};
};

SpriteVertexBuffer::SpriteVertexBuffer()
	: m_impl(std::make_unique<Impl>())
{
}

SpriteVertexBuffer::~SpriteVertexBuffer() = default;
SpriteVertexBuffer::SpriteVertexBuffer(SpriteVertexBuffer&&) noexcept = default;
SpriteVertexBuffer& SpriteVertexBuffer::operator=(SpriteVertexBuffer&&) noexcept = default;

void SpriteVertexBuffer::Update(const std::vector<SpriteVertex>& vertices)
{
	if (m_impl == nullptr)
	{
		return;
	}

	// Updates may run zero or many times before a draw. Keep the latest CPU data;
	// the renderer snapshots it into fence-protected storage when drawing.
	const auto vertexCount = std::min<std::size_t>(vertices.size(), m_impl->capacity);
	m_impl->vertices.assign(vertices.begin(), vertices.begin() + vertexCount);
}

std::uint32_t SpriteVertexBuffer::GetVertexCount() const
{
	return m_impl == nullptr ? 0 : static_cast<std::uint32_t>(m_impl->vertices.size());
}

std::uint32_t SpriteVertexBuffer::GetVertexCapacity() const
{
	return m_impl == nullptr ? 0 : m_impl->capacity;
}

void RenderResourceAccess::Initialize(SpriteVertexBuffer& buffer, ID3D12Device*, std::uint32_t vertexCapacity)
{
	if (vertexCapacity > (std::numeric_limits<UINT>::max)() / sizeof(SpriteVertex))
	{
		throw std::length_error("Sprite vertex data exceeds the D3D12 vertex view size.");
	}
	if (buffer.m_impl == nullptr)
	{
		buffer.m_impl = std::make_unique<SpriteVertexBuffer::Impl>();
	}
	buffer.m_impl->capacity = vertexCapacity;
	buffer.m_impl->vertices.clear();
	buffer.m_impl->vertices.reserve(vertexCapacity);
}

void RenderResourceAccess::Bind(
	const SpriteVertexBuffer& buffer, ID3D12GraphicsCommandList* commandList, FrameUploadBuffer& upload)
{
	if (buffer.m_impl != nullptr)
	{
		D3D12_VERTEX_BUFFER_VIEW view{};
		view.StrideInBytes = sizeof(SpriteVertex);
		view.SizeInBytes = static_cast<UINT>(buffer.m_impl->vertices.size() * sizeof(SpriteVertex));
		if (view.SizeInBytes != 0)
		{
			view.BufferLocation = upload.Write(buffer.m_impl->vertices.data(), view.SizeInBytes);
		}
		commandList->IASetVertexBuffers(0, 1, &view);
	}
}
