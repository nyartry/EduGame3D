#pragma once

#include "Vertex.h"

#include <Windows.h>
#include <wrl/client.h>

#include <array>
#include <d3d12.h>

class Ground
{
public:
	void Initialize(ID3D12Device* device);
	void Draw(ID3D12GraphicsCommandList* commandList) const;

private:
	void CreateVertexBuffer(ID3D12Device* device);

	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView{};

	std::array<Vertex, 6> m_vertices =
	{
		Vertex{ { -8.0f, 0.0f, 8.0f }, { 0.28f, 0.58f, 0.28f, 1.0f } },
		Vertex{ { 8.0f, 0.0f, 8.0f }, { 0.34f, 0.68f, 0.34f, 1.0f } },
		Vertex{ { 8.0f, 0.0f, -8.0f }, { 0.18f, 0.42f, 0.22f, 1.0f } },
		Vertex{ { -8.0f, 0.0f, 8.0f }, { 0.28f, 0.58f, 0.28f, 1.0f } },
		Vertex{ { 8.0f, 0.0f, -8.0f }, { 0.18f, 0.42f, 0.22f, 1.0f } },
		Vertex{ { -8.0f, 0.0f, -8.0f }, { 0.16f, 0.36f, 0.20f, 1.0f } },
	};
};

