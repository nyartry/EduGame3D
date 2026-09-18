#include "Framework/Rendering/Buffers/SkinnedVertexBuffer.h"

#include "Framework/Rendering/Core/Dx12BufferHelper.h"
#include "Framework/Rendering/Core/RenderResourceAccess.h"

#include <Windows.h>
#include <wrl/client.h>

#include <d3d12.h>
#include <memory>
#include <utility>

struct SkinnedVertexBuffer::Impl
{
	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	D3D12_VERTEX_BUFFER_VIEW view{};
	std::uint32_t vertexCount{};
};

SkinnedVertexBuffer::SkinnedVertexBuffer()
	: m_impl(std::make_unique<Impl>())
{
}

SkinnedVertexBuffer::~SkinnedVertexBuffer() = default;
SkinnedVertexBuffer::SkinnedVertexBuffer(SkinnedVertexBuffer&&) noexcept = default;
SkinnedVertexBuffer& SkinnedVertexBuffer::operator=(SkinnedVertexBuffer&&) noexcept = default;

std::uint32_t SkinnedVertexBuffer::GetVertexCount() const
{
	return m_impl == nullptr ? 0 : m_impl->vertexCount;
}

void RenderResourceAccess::Initialize(
	SkinnedVertexBuffer& buffer,
	ID3D12Device* device,
	const std::vector<SkinnedVertex>& vertices)
{
	if (buffer.m_impl == nullptr)
	{
		buffer.m_impl = std::make_unique<SkinnedVertexBuffer::Impl>();
	}

	const UINT bufferSize = static_cast<UINT>(vertices.size() * sizeof(SkinnedVertex));
	buffer.m_impl->resource = Dx12BufferHelper::CreateUploadBufferWithData(device, vertices.data(), bufferSize);
	buffer.m_impl->vertexCount = static_cast<std::uint32_t>(vertices.size());
	buffer.m_impl->view.BufferLocation = buffer.m_impl->resource->GetGPUVirtualAddress();
	buffer.m_impl->view.StrideInBytes = sizeof(SkinnedVertex);
	buffer.m_impl->view.SizeInBytes = bufferSize;
}

void RenderResourceAccess::Bind(const SkinnedVertexBuffer& buffer, ID3D12GraphicsCommandList* commandList)
{
	if (buffer.m_impl != nullptr)
	{
		commandList->IASetVertexBuffers(0, 1, &buffer.m_impl->view);
	}
}
