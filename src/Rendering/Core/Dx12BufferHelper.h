#pragma once

#include <Windows.h>
#include <wrl/client.h>

#include <d3d12.h>

namespace Dx12BufferHelper
{
	UINT AlignConstantBufferSize(UINT size);

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateUploadBuffer(
		ID3D12Device* device,
		UINT64 sizeInBytes);

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateUploadBufferWithData(
		ID3D12Device* device,
		const void* data,
		UINT64 sizeInBytes);
}
