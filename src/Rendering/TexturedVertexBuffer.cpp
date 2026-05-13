#include "Rendering/TexturedVertexBuffer.h"

#include "Common/Common.h"
#include "Rendering/Dx12BufferHelper.h"

#include <cstring>

void TexturedVertexBuffer::Initialize(ID3D12Device* device, const std::vector<TexturedVertex>& vertices)
{
	const UINT bufferSize = static_cast<UINT>(vertices.size() * sizeof(TexturedVertex));

	m_resource = Dx12BufferHelper::CreateUploadBufferWithData(device, vertices.data(), bufferSize);
	m_vertexCount = static_cast<UINT>(vertices.size());
	m_capacity = m_vertexCount;

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

	UINT8* mappedData = nullptr;
	D3D12_RANGE readRange{};
	ThrowIfFailed(m_resource->Map(0, &readRange, reinterpret_cast<void**>(&mappedData)));
	memcpy(mappedData, vertices.data(), vertices.size() * sizeof(TexturedVertex));
	m_resource->Unmap(0, nullptr);

	m_vertexCount = vertexCount;
	m_view.SizeInBytes = static_cast<UINT>(vertices.size() * sizeof(TexturedVertex));
}

void TexturedVertexBuffer::Bind(ID3D12GraphicsCommandList* commandList) const
{
	commandList->IASetVertexBuffers(0, 1, &m_view);
}

UINT TexturedVertexBuffer::GetVertexCount() const
{
	return m_vertexCount;
}
