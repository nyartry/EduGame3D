#include "Framework/Rendering/Buffers/VertexBuffer.h"

#include "Framework/Rendering/Core/Dx12BufferHelper.h"

void VertexBuffer::Initialize(ID3D12Device* device, const std::vector<Vertex>& vertices)
{
	const UINT bufferSize = static_cast<UINT>(vertices.size() * sizeof(Vertex));

	m_resource = Dx12BufferHelper::CreateUploadBufferWithData(device, vertices.data(), bufferSize);
	m_vertexCount = static_cast<UINT>(vertices.size());

	m_view.BufferLocation = m_resource->GetGPUVirtualAddress();
	m_view.StrideInBytes = sizeof(Vertex);
	m_view.SizeInBytes = bufferSize;
}

void VertexBuffer::Bind(ID3D12GraphicsCommandList* commandList) const
{
	commandList->IASetVertexBuffers(0, 1, &m_view);
}

UINT VertexBuffer::GetVertexCount() const
{
	return m_vertexCount;
}
