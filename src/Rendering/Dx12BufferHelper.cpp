#include "Rendering/Dx12BufferHelper.h"

#include "Common/Common.h"

#include <cstring>

using Microsoft::WRL::ComPtr;

namespace Dx12BufferHelper
{
	UINT AlignConstantBufferSize(UINT size)
	{
		return (size + 255) & ~255u;
	}

	ComPtr<ID3D12Resource> CreateUploadBuffer(ID3D12Device* device, UINT64 sizeInBytes)
	{
		D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
		heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProperties.CreationNodeMask = 1;
		heapProperties.VisibleNodeMask = 1;

		D3D12_RESOURCE_DESC resourceDesc{};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Alignment = 0;
		resourceDesc.Width = sizeInBytes;
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.SampleDesc.Quality = 0;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		ComPtr<ID3D12Resource> resource;
		ThrowIfFailed(device->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&resource)));

		return resource;
	}

	ComPtr<ID3D12Resource> CreateUploadBufferWithData(
		ID3D12Device* device,
		const void* data,
		UINT64 sizeInBytes)
	{
		ComPtr<ID3D12Resource> resource = CreateUploadBuffer(device, sizeInBytes);

		UINT8* mappedData = nullptr;
		D3D12_RANGE readRange{};
		ThrowIfFailed(resource->Map(0, &readRange, reinterpret_cast<void**>(&mappedData)));
		memcpy(mappedData, data, static_cast<size_t>(sizeInBytes));
		resource->Unmap(0, nullptr);

		return resource;
	}
}
