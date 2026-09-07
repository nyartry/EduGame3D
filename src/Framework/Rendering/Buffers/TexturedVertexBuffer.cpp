#include "Framework/Rendering/Buffers/TexturedVertexBuffer.h"

#include "Framework/Rendering/Core/Dx12BufferHelper.h"
#include "Framework/Rendering/Core/FrameUploadBuffer.h"
#include "Framework/Rendering/Core/RenderResourceAccess.h"

#include <limits>
#include <memory>
#include <utility>

struct TexturedVertexBuffer::Impl
{
	// Static meshes keep their original upload buffer. Updating a CPU-skinned
	// mesh retains that resource and stores the next pose on the CPU until Bind.
	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	D3D12_VERTEX_BUFFER_VIEW view{};
	std::vector<TexturedVertex> dynamicVertices;
	std::uint32_t vertexCount{};
	std::uint32_t capacity{};
	bool dynamic{};
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

	m_impl->dynamicVertices = vertices;
	m_impl->dynamic = true;
	m_impl->vertexCount = static_cast<std::uint32_t>(vertices.size());
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
	if (vertices.size() > (std::numeric_limits<UINT>::max)() / sizeof(TexturedVertex))
	{
		throw std::length_error("Textured vertex data exceeds the D3D12 vertex view size.");
	}
	if (buffer.m_impl == nullptr)
	{
		buffer.m_impl = std::make_unique<TexturedVertexBuffer::Impl>();
	}

	const auto bufferSize = static_cast<UINT>(vertices.size() * sizeof(TexturedVertex));
	buffer.m_impl->resource = Dx12BufferHelper::CreateUploadBufferWithData(device, vertices.data(), bufferSize);
	buffer.m_impl->vertexCount = static_cast<std::uint32_t>(vertices.size());
	buffer.m_impl->capacity = buffer.m_impl->vertexCount;
	buffer.m_impl->dynamic = false;
	buffer.m_impl->dynamicVertices.clear();
	buffer.m_impl->view.BufferLocation = buffer.m_impl->resource->GetGPUVirtualAddress();
	buffer.m_impl->view.StrideInBytes = sizeof(TexturedVertex);
	buffer.m_impl->view.SizeInBytes = bufferSize;
}

void RenderResourceAccess::Bind(
	const TexturedVertexBuffer& buffer, ID3D12GraphicsCommandList* commandList, FrameUploadBuffer& upload)
{
	if (buffer.m_impl != nullptr)
	{
		D3D12_VERTEX_BUFFER_VIEW view = buffer.m_impl->view;
		if (buffer.m_impl->dynamic)
		{
			view.SizeInBytes = static_cast<UINT>(buffer.m_impl->dynamicVertices.size() * sizeof(TexturedVertex));
			view.BufferLocation = view.SizeInBytes == 0 ? 0 :
				upload.Write(buffer.m_impl->dynamicVertices.data(), view.SizeInBytes);
		}
		commandList->IASetVertexBuffers(0, 1, &view);
	}
}
