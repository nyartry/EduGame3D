#include "Framework/Rendering/Buffers/VertexBuffer.h"

#include "Framework/Rendering/Core/Dx12BufferHelper.h"
#include "Framework/Rendering/Core/RenderResourceAccess.h"

#include <Windows.h>
#include <wrl/client.h>

#include <d3d12.h>
#include <memory>
#include <utility>

struct VertexBuffer::Impl
{
	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	D3D12_VERTEX_BUFFER_VIEW view{};
	std::uint32_t vertexCount{};
};

VertexBuffer::VertexBuffer()
	: m_impl(std::make_unique<Impl>())
{
}

VertexBuffer::~VertexBuffer() = default;
VertexBuffer::VertexBuffer(VertexBuffer&&) noexcept = default;
VertexBuffer& VertexBuffer::operator=(VertexBuffer&&) noexcept = default;

std::uint32_t VertexBuffer::GetVertexCount() const
{
	return m_impl == nullptr ? 0 : m_impl->vertexCount;
}

void RenderResourceAccess::Initialize(VertexBuffer& buffer, ID3D12Device* device, const std::vector<Vertex>& vertices)
{
	if (buffer.m_impl == nullptr)
	{
		buffer.m_impl = std::make_unique<VertexBuffer::Impl>();
	}

	const UINT bufferSize = static_cast<UINT>(vertices.size() * sizeof(Vertex));
	buffer.m_impl->resource = Dx12BufferHelper::CreateUploadBufferWithData(device, vertices.data(), bufferSize);
	buffer.m_impl->vertexCount = static_cast<std::uint32_t>(vertices.size());
	buffer.m_impl->view.BufferLocation = buffer.m_impl->resource->GetGPUVirtualAddress();
	buffer.m_impl->view.StrideInBytes = sizeof(Vertex);
	buffer.m_impl->view.SizeInBytes = bufferSize;
}

void RenderResourceAccess::Bind(const VertexBuffer& buffer, ID3D12GraphicsCommandList* commandList)
{
	if (buffer.m_impl != nullptr)
	{
		commandList->IASetVertexBuffers(0, 1, &buffer.m_impl->view);
	}
}
