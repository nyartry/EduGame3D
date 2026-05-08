#include "Ground.h"

#include "Common.h"

#include <cstdint>
#include <cstring>

void Ground::Initialize(ID3D12Device* device)
{
	BuildMesh();
	CreateVertexBuffer(device);
}

void Ground::Draw(ID3D12GraphicsCommandList* commandList) const
{
	commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	commandList->DrawInstanced(static_cast<UINT>(m_vertices.size()), 1, 0, 0);
}

void Ground::CreateVertexBuffer(ID3D12Device* device)
{
	const UINT vertexBufferSize = static_cast<UINT>(m_vertices.size() * sizeof(Vertex));

	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.CreationNodeMask = 1;
	heapProperties.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Alignment = 0;
	resourceDesc.Width = vertexBufferSize;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ThrowIfFailed(device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_vertexBuffer)));

	UINT8* vertexDataBegin = nullptr;
	D3D12_RANGE readRange{};
	ThrowIfFailed(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&vertexDataBegin)));
	memcpy(vertexDataBegin, m_vertices.data(), vertexBufferSize);
	m_vertexBuffer->Unmap(0, nullptr);

	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes = sizeof(Vertex);
	m_vertexBufferView.SizeInBytes = vertexBufferSize;
}

void Ground::BuildMesh()
{
	constexpr int TileCount = 16;
	constexpr float HalfExtent = 8.0f;
	constexpr float TileSize = (HalfExtent * 2.0f) / static_cast<float>(TileCount);

	m_vertices.clear();
	m_vertices.reserve(TileCount * TileCount * 6);

	for (int z = 0; z < TileCount; ++z)
	{
		for (int x = 0; x < TileCount; ++x)
		{
			const float left = -HalfExtent + static_cast<float>(x) * TileSize;
			const float right = left + TileSize;
			const float nearZ = -HalfExtent + static_cast<float>(z) * TileSize;
			const float farZ = nearZ + TileSize;
			const bool alternate = ((x + z) % 2) == 0;
			const std::array<float, 4> color = alternate
				? std::array<float, 4>{ 0.23f, 0.52f, 0.24f, 1.0f }
			: std::array<float, 4>{ 0.17f, 0.39f, 0.20f, 1.0f };

			const Vertex v0{ { left, 0.0f, farZ }, { color[0], color[1], color[2], color[3] } };
			const Vertex v1{ { right, 0.0f, farZ }, { color[0], color[1], color[2], color[3] } };
			const Vertex v2{ { right, 0.0f, nearZ }, { color[0], color[1], color[2], color[3] } };
			const Vertex v3{ { left, 0.0f, nearZ }, { color[0], color[1], color[2], color[3] } };

			m_vertices.push_back(v0);
			m_vertices.push_back(v1);
			m_vertices.push_back(v2);
			m_vertices.push_back(v0);
			m_vertices.push_back(v2);
			m_vertices.push_back(v3);
		}
	}
}
