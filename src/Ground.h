#pragma once

#include "Vertex.h"

#include <Windows.h>
#include <wrl/client.h>

#include <array>
#include <d3d12.h>
#include <vector>

class Ground
{
public:
	void Initialize(ID3D12Device* device);
	void Draw(ID3D12GraphicsCommandList* commandList) const;

private:
	void BuildMesh();
	void CreateVertexBuffer(ID3D12Device* device);

	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView{};
	std::vector<Vertex> m_vertices;
};
