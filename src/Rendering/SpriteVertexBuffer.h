#pragma once

#include "Rendering/SpriteVertex.h"

#include <Windows.h>
#include <wrl/client.h>

#include <d3d12.h>
#include <vector>

class SpriteVertexBuffer
{
public:
	void Initialize(ID3D12Device* device, UINT vertexCapacity);
	void Update(const std::vector<SpriteVertex>& vertices);
	void Bind(ID3D12GraphicsCommandList* commandList) const;

	UINT GetVertexCount() const;
	UINT GetVertexCapacity() const;

private:
	static constexpr UINT DynamicBufferCopies = 3;

	ID3D12Device* m_device{};
	Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;
	D3D12_VERTEX_BUFFER_VIEW m_view{};
	UINT m_vertexCount{};
	UINT m_capacity{};
	UINT m_slotSize{};
	UINT m_nextSlot{};
};
