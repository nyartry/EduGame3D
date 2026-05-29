#include "Rendering/Buffers/TexturedVertexBuffer.h"

#include "Common/Common.h"
#include "Rendering/Core/Dx12BufferHelper.h"

#include <cstring>

void TexturedVertexBuffer::Initialize(ID3D12Device* device, const std::vector<TexturedVertex>& vertices)
{
	const UINT bufferSize = static_cast<UINT>(vertices.size() * sizeof(TexturedVertex));

	m_device = device;
	m_resource = Dx12BufferHelper::CreateUploadBufferWithData(device, vertices.data(), bufferSize);
	m_vertexCount = static_cast<UINT>(vertices.size());
	m_capacity = m_vertexCount;
	m_dynamicSlotSize = bufferSize;

	m_view.BufferLocation = m_resource->GetGPUVirtualAddress();
	m_view.StrideInBytes = sizeof(TexturedVertex);
	m_view.SizeInBytes = bufferSize;
}

void TexturedVertexBuffer::Update(const std::vector<TexturedVertex>& vertices)
{
	const UINT vertexCount = static_cast<UINT>(vertices.size());
	if (vertexCount > m_capacity)
	{
		return;
	}

	EnsureDynamicResource();
	const UINT bufferSize = static_cast<UINT>(vertices.size() * sizeof(TexturedVertex));
	const UINT bufferOffset = m_nextDynamicSlot * m_dynamicSlotSize;
	m_nextDynamicSlot = (m_nextDynamicSlot + 1) % DynamicBufferCopies;
	WriteVertices(vertices, bufferOffset);

	m_vertexCount = vertexCount;
	m_view.BufferLocation = m_resource->GetGPUVirtualAddress() + bufferOffset;
	m_view.SizeInBytes = bufferSize;
}

void TexturedVertexBuffer::Bind(ID3D12GraphicsCommandList* commandList) const
{
	commandList->IASetVertexBuffers(0, 1, &m_view);
}

UINT TexturedVertexBuffer::GetVertexCount() const
{
	return m_vertexCount;
}

void TexturedVertexBuffer::EnsureDynamicResource()
{
	if (m_usesDynamicCopies)
	{
		return;
	}

	m_dynamicSlotSize = static_cast<UINT>(m_capacity * sizeof(TexturedVertex));
	m_resource = Dx12BufferHelper::CreateUploadBuffer(
		m_device,
		static_cast<UINT64>(m_dynamicSlotSize) * DynamicBufferCopies);
	m_nextDynamicSlot = 0;
	m_usesDynamicCopies = true;
}

void TexturedVertexBuffer::WriteVertices(const std::vector<TexturedVertex>& vertices, UINT bufferOffset)
{
	UINT8* mappedData = nullptr;
	D3D12_RANGE readRange{};
	ThrowIfFailed(m_resource->Map(0, &readRange, reinterpret_cast<void**>(&mappedData)));
	memcpy(mappedData + bufferOffset, vertices.data(), vertices.size() * sizeof(TexturedVertex));
	m_resource->Unmap(0, nullptr);
}
