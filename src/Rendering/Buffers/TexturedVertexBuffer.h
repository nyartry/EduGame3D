#pragma once

#include "Rendering/Geometry/TexturedVertex.h"

#include <Windows.h>
#include <wrl/client.h>

#include <d3d12.h>
#include <vector>

class TexturedVertexBuffer
{
public:
	void Initialize(ID3D12Device* device, const std::vector<TexturedVertex>& vertices);
	void Update(const std::vector<TexturedVertex>& vertices);
	void Bind(ID3D12GraphicsCommandList* commandList) const;

	UINT GetVertexCount() const;

private:
	void EnsureDynamicResource();
	void WriteVertices(const std::vector<TexturedVertex>& vertices, UINT bufferOffset);

	static constexpr UINT DynamicBufferCopies = 3;

	ID3D12Device* m_device{};
	Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;
	D3D12_VERTEX_BUFFER_VIEW m_view{};
	UINT m_vertexCount{};
	UINT m_capacity{};
	UINT m_dynamicSlotSize{};
	UINT m_nextDynamicSlot{};
	bool m_usesDynamicCopies{};
};
