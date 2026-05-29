#pragma once

#include "Rendering/Geometry/Vertex.h"

#include <Windows.h>
#include <wrl/client.h>

#include <d3d12.h>
#include <vector>

class VertexBuffer
{
public:
	void Initialize(ID3D12Device* device, const std::vector<Vertex>& vertices);
	void Bind(ID3D12GraphicsCommandList* commandList) const;

	UINT GetVertexCount() const;

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;
	D3D12_VERTEX_BUFFER_VIEW m_view{};
	UINT m_vertexCount{};
};
