#pragma once

#include "Rendering/VertexBuffer.h"
#include "Rendering/Vertex.h"

#include <Windows.h>

#include <array>
#include <DirectXMath.h>
#include <d3d12.h>
#include <vector>

class Dx12Renderer;

class Ground
{
public:
	void Initialize(ID3D12Device* device);
	void Draw(Dx12Renderer& renderer) const;
	bool TryGetHeightAt(const DirectX::XMFLOAT3& position, float radius, float& height) const;

private:
	void BuildMesh();

	static constexpr float GroundHeight = 0.0f;
	static constexpr float HalfExtent = 8.0f;

	VertexBuffer m_vertexBuffer;
	std::vector<Vertex> m_vertices;
};
