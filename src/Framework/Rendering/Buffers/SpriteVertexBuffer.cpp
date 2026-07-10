#include "Framework/Rendering/Buffers/SpriteVertexBuffer.h"

#include "Framework/Common/Common.h"
#include "Framework/Rendering/Core/Dx12BufferHelper.h"

#include <algorithm>
#include <cstring>

void SpriteVertexBuffer::Initialize(ID3D12Device* device, UINT vertexCapacity)
{
	m_device = device;
	m_capacity = std::max<UINT>(vertexCapacity, 1);
	m_slotSize = static_cast<UINT>(m_capacity * sizeof(SpriteVertex));
	m_resource = Dx12BufferHelper::CreateUploadBuffer(
		device,
		static_cast<UINT64>(m_slotSize) * DynamicBufferCopies);

	m_nextSlot = 0;
	m_vertexCount = 0;
	m_view.BufferLocation = m_resource->GetGPUVirtualAddress();
	m_view.StrideInBytes = sizeof(SpriteVertex);
	m_view.SizeInBytes = 0;
}

void SpriteVertexBuffer::Update(const std::vector<SpriteVertex>& vertices)
{
	const UINT vertexCount = static_cast<UINT>(vertices.size());
	if (vertexCount > m_capacity || m_resource == nullptr)
	{
		return;
	}

	const UINT bufferOffset = m_nextSlot * m_slotSize;
	m_nextSlot = (m_nextSlot + 1) % DynamicBufferCopies;

	if (!vertices.empty())
	{
		UINT8* mappedData = nullptr;
		D3D12_RANGE readRange{};
		ThrowIfFailed(m_resource->Map(0, &readRange, reinterpret_cast<void**>(&mappedData)));
		memcpy(mappedData + bufferOffset, vertices.data(), vertices.size() * sizeof(SpriteVertex));
		m_resource->Unmap(0, nullptr);
	}

	m_vertexCount = vertexCount;
	m_view.BufferLocation = m_resource->GetGPUVirtualAddress() + bufferOffset;
	m_view.SizeInBytes = static_cast<UINT>(vertices.size() * sizeof(SpriteVertex));
}

void SpriteVertexBuffer::Bind(ID3D12GraphicsCommandList* commandList) const
{
	commandList->IASetVertexBuffers(0, 1, &m_view);
}

UINT SpriteVertexBuffer::GetVertexCount() const
{
	return m_vertexCount;
}

UINT SpriteVertexBuffer::GetVertexCapacity() const
{
	return m_capacity;
}
