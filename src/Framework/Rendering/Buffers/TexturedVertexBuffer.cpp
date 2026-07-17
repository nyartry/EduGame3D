#include "Framework/Rendering/Buffers/TexturedVertexBuffer.h"

#include "Framework/Common/Common.h"
#include "Framework/Rendering/Core/Dx12BufferHelper.h"
#include "Framework/Rendering/Core/RenderResourceAccess.h"

#include <Windows.h>
#include <wrl/client.h>

#include <cstring>
#include <d3d12.h>
#include <memory>
#include <utility>

struct TexturedVertexBuffer::Impl
{
	static constexpr std::uint32_t DynamicBufferCopies = 3;

	void EnsureDynamicResource()
	{
		if (usesDynamicCopies)
		{
			return;
		}

		dynamicSlotSize = static_cast<std::uint32_t>(capacity * sizeof(TexturedVertex));
		resource = Dx12BufferHelper::CreateUploadBuffer(
			device,
			static_cast<UINT64>(dynamicSlotSize) * DynamicBufferCopies);
		nextDynamicSlot = 0;
		usesDynamicCopies = true;
	}

	void WriteVertices(const std::vector<TexturedVertex>& vertices, std::uint32_t bufferOffset)
	{
		void* mappedData = nullptr;
		const D3D12_RANGE readRange{ 0, 0 };
		ThrowIfFailed(resource->Map(0, &readRange, &mappedData));
		std::memcpy(
			static_cast<std::uint8_t*>(mappedData) + bufferOffset,
			vertices.data(),
			vertices.size() * sizeof(TexturedVertex));
		resource->Unmap(0, nullptr);
	}

	ID3D12Device* device{};
	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	D3D12_VERTEX_BUFFER_VIEW view{};
	std::uint32_t vertexCount{};
	std::uint32_t capacity{};
	std::uint32_t dynamicSlotSize{};
	std::uint32_t nextDynamicSlot{};
	bool usesDynamicCopies{};
};

TexturedVertexBuffer::TexturedVertexBuffer()
	: m_impl(std::make_unique<Impl>())
{
}

TexturedVertexBuffer::~TexturedVertexBuffer() = default;
TexturedVertexBuffer::TexturedVertexBuffer(TexturedVertexBuffer&&) noexcept = default;
TexturedVertexBuffer& TexturedVertexBuffer::operator=(TexturedVertexBuffer&&) noexcept = default;

void TexturedVertexBuffer::Update(const std::vector<TexturedVertex>& vertices)
{
	if (m_impl == nullptr || vertices.size() > m_impl->capacity)
	{
		return;
	}

	m_impl->EnsureDynamicResource();
	const std::uint32_t bufferSize = static_cast<std::uint32_t>(vertices.size() * sizeof(TexturedVertex));
	const std::uint32_t bufferOffset = m_impl->nextDynamicSlot * m_impl->dynamicSlotSize;
	m_impl->nextDynamicSlot = (m_impl->nextDynamicSlot + 1) % Impl::DynamicBufferCopies;
	m_impl->WriteVertices(vertices, bufferOffset);

	m_impl->vertexCount = static_cast<std::uint32_t>(vertices.size());
	m_impl->view.BufferLocation = m_impl->resource->GetGPUVirtualAddress() + bufferOffset;
	m_impl->view.SizeInBytes = bufferSize;
}

std::uint32_t TexturedVertexBuffer::GetVertexCount() const
{
	return m_impl == nullptr ? 0 : m_impl->vertexCount;
}

void RenderResourceAccess::Initialize(
	TexturedVertexBuffer& buffer,
	ID3D12Device* device,
	const std::vector<TexturedVertex>& vertices)
{
	if (buffer.m_impl == nullptr)
	{
		buffer.m_impl = std::make_unique<TexturedVertexBuffer::Impl>();
	}

	const std::uint32_t bufferSize = static_cast<std::uint32_t>(vertices.size() * sizeof(TexturedVertex));
	buffer.m_impl->device = device;
	buffer.m_impl->resource = Dx12BufferHelper::CreateUploadBufferWithData(device, vertices.data(), bufferSize);
	buffer.m_impl->vertexCount = static_cast<std::uint32_t>(vertices.size());
	buffer.m_impl->capacity = buffer.m_impl->vertexCount;
	buffer.m_impl->dynamicSlotSize = bufferSize;
	buffer.m_impl->view.BufferLocation = buffer.m_impl->resource->GetGPUVirtualAddress();
	buffer.m_impl->view.StrideInBytes = sizeof(TexturedVertex);
	buffer.m_impl->view.SizeInBytes = bufferSize;
}

void RenderResourceAccess::Bind(const TexturedVertexBuffer& buffer, ID3D12GraphicsCommandList* commandList)
{
	if (buffer.m_impl != nullptr)
	{
		commandList->IASetVertexBuffers(0, 1, &buffer.m_impl->view);
	}
}
