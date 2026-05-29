#pragma once

#include "Common/Common.h"
#include "Rendering/Core/Dx12BufferHelper.h"

#include <Windows.h>
#include <wrl/client.h>

#include <cstring>
#include <d3d12.h>

template <typename T, UINT Capacity>
class ConstantBufferRing
{
public:
	void Initialize(ID3D12Device* device)
	{
		m_constantBufferSize = Dx12BufferHelper::AlignConstantBufferSize(sizeof(T));
		m_constantBuffer = Dx12BufferHelper::CreateUploadBuffer(
			device,
			static_cast<UINT64>(m_constantBufferSize) * Capacity);

		D3D12_RANGE readRange{};
		ThrowIfFailed(m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_mappedData)));
	}

	D3D12_GPU_VIRTUAL_ADDRESS Write(const T& constants)
	{
		const UINT constantBufferIndex = m_nextConstantBufferIndex;
		m_nextConstantBufferIndex = (m_nextConstantBufferIndex + 1) % Capacity;

		UINT8* destination = m_mappedData + static_cast<SIZE_T>(constantBufferIndex) * m_constantBufferSize;
		memcpy(destination, &constants, sizeof(constants));
		return m_constantBuffer->GetGPUVirtualAddress() + static_cast<UINT64>(constantBufferIndex) * m_constantBufferSize;
	}

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_constantBuffer;
	UINT8* m_mappedData{};
	UINT m_constantBufferSize{};
	UINT m_nextConstantBufferIndex{};
};
