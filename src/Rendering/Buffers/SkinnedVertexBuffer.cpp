#include "Rendering/Buffers/SkinnedVertexBuffer.h"

#include "Rendering/Core/Dx12BufferHelper.h"

void SkinnedVertexBuffer::Initialize(ID3D12Device* device, const std::vector<SkinnedVertex>& vertices)
{
	const UINT bufferSize = static_cast<UINT>(vertices.size() * sizeof(SkinnedVertex));

	m_resource = Dx12BufferHelper::CreateUploadBufferWithData(device, vertices.data(), bufferSize);
	m_vertexCount = static_cast<UINT>(vertices.size());

	m_view.BufferLocation = m_resource->GetGPUVirtualAddress();
	m_view.StrideInBytes = sizeof(SkinnedVertex);
	m_view.SizeInBytes = bufferSize;
}

void SkinnedVertexBuffer::Bind(ID3D12GraphicsCommandList* commandList) const
{
	commandList->IASetVertexBuffers(0, 1, &m_view);
}

UINT SkinnedVertexBuffer::GetVertexCount() const
{
	return m_vertexCount;
}
