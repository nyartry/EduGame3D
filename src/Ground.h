#pragma once

#include "VertexBuffer.h"
#include "Vertex.h"

#include <Windows.h>

#include <array>
#include <d3d12.h>
#include <vector>

class Dx12Renderer;

class Ground
{
public:
	void Initialize(ID3D12Device* device);
	void Draw(Dx12Renderer& renderer) const;

private:
	void BuildMesh();

	VertexBuffer m_vertexBuffer;
	std::vector<Vertex> m_vertices;
};
