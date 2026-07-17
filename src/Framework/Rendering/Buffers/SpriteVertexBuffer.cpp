#include "Framework/Rendering/Buffers/SpriteVertexBuffer.h"

#include "Framework/Common/Common.h"
#include "Framework/Rendering/Core/Dx12BufferHelper.h"
#include "Framework/Rendering/Core/RenderResourceAccess.h"

#include <Windows.h>
#include <wrl/client.h>

#include <algorithm>
#include <cstring>
#include <d3d12.h>
#include <memory>
#include <utility>

struct SpriteVertexBuffer::Impl
{
	static constexpr std::uint32_t DynamicBufferCopies = 3;

	ID3D12Device* device{};
	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	D3D12_VERTEX_BUFFER_VIEW view{};
	std::uint32_t vertexCount{};
	std::uint32_t capacity{};
	std::uint32_t slotSize{};
	std::uint32_t nextSlot{};
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

	const std::uint32_t vertexCount = std::min<std::uint32_t>(static_cast<std::uint32_t>(vertices.size()), m_impl->capacity);
	const std::uint32_t bufferOffset = m_impl->nextSlot * m_impl->slotSize;
	m_impl->nextSlot = (m_impl->nextSlot + 1) % Impl::DynamicBufferCopies;

	void* mappedData = nullptr;
	const D3D12_RANGE readRange{ 0, 0 };
	ThrowIfFailed(m_impl->resource->Map(0, &readRange, &mappedData));
	std::memcpy(
		static_cast<std::uint8_t*>(mappedData) + bufferOffset,
		vertices.data(),
		static_cast<std::size_t>(vertexCount) * sizeof(SpriteVertex));
	m_impl->resource->Unmap(0, nullptr);

	m_impl->vertexCount = vertexCount;
	m_impl->view.BufferLocation = m_impl->resource->GetGPUVirtualAddress() + bufferOffset;
	m_impl->view.SizeInBytes = vertexCount * sizeof(SpriteVertex);
}

std::uint32_t SpriteVertexBuffer::GetVertexCount() const
{
	return m_impl == nullptr ? 0 : m_impl->vertexCount;
}

std::uint32_t SpriteVertexBuffer::GetVertexCapacity() const
{
	return m_impl == nullptr ? 0 : m_impl->capacity;
}

void RenderResourceAccess::Initialize(SpriteVertexBuffer& buffer, ID3D12Device* device, std::uint32_t vertexCapacity)
{
	if (buffer.m_impl == nullptr)
	{
		buffer.m_impl = std::make_unique<SpriteVertexBuffer::Impl>();
	}

	buffer.m_impl->device = device;
	buffer.m_impl->capacity = vertexCapacity;
	buffer.m_impl->slotSize = vertexCapacity * sizeof(SpriteVertex);
	buffer.m_impl->resource = Dx12BufferHelper::CreateUploadBuffer(
		device,
		static_cast<UINT64>(buffer.m_impl->slotSize) * SpriteVertexBuffer::Impl::DynamicBufferCopies);
	buffer.m_impl->view.StrideInBytes = sizeof(SpriteVertex);
	buffer.m_impl->view.SizeInBytes = 0;
	buffer.m_impl->view.BufferLocation = buffer.m_impl->resource->GetGPUVirtualAddress();
	buffer.m_impl->nextSlot = 0;
}

void RenderResourceAccess::Bind(const SpriteVertexBuffer& buffer, ID3D12GraphicsCommandList* commandList)
{
	if (buffer.m_impl != nullptr)
	{
		commandList->IASetVertexBuffers(0, 1, &buffer.m_impl->view);
	}
}
