#include "TexturedVertexBuffer.h"

#include "Dx12BufferHelper.h"

void TexturedVertexBuffer::Initialize(ID3D12Device* device, const std::vector<TexturedVertex>& vertices)
{
	const UINT bufferSize = static_cast<UINT>(vertices.size() * sizeof(TexturedVertex));

	m_resource = Dx12BufferHelper::CreateUploadBufferWithData(device, vertices.data(), bufferSize);
	m_vertexCount = static_cast<UINT>(vertices.size());

	m_view.BufferLocation = m_resource->GetGPUVirtualAddress();
	m_view.StrideInBytes = sizeof(TexturedVertex);
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
