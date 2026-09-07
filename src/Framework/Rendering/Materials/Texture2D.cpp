#include "Framework/Rendering/Materials/Texture2D.h"

#include "Framework/Assets/ImageLoader.h"
#include "Framework/Common/Common.h"
#include "Framework/Rendering/Core/Dx12BufferHelper.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <vector>
#include <atomic>


using Microsoft::WRL::ComPtr;

namespace
{
	std::atomic<std::uint64_t> uploadCount{};
	void ExecuteAndWait(ID3D12Device* device, ID3D12CommandQueue* commandQueue, ID3D12GraphicsCommandList* commandList)
	{
		ThrowIfFailed(commandList->Close());

		ID3D12CommandList* commandLists[] = { commandList };
		commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

		ComPtr<ID3D12Fence> fence;
		ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

		HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (fenceEvent == nullptr)
		{
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
		}

		ThrowIfFailed(commandQueue->Signal(fence.Get(), 1));
		ThrowIfFailed(fence->SetEventOnCompletion(1, fenceEvent));
		WaitForSingleObjectEx(fenceEvent, INFINITE, FALSE);
		CloseHandle(fenceEvent);
	}
}

void Texture2D::Initialize(ID3D12Device* device, const std::string& filePath, bool useSrgb)
{
	const auto image = ImageLoader::Load(filePath);
	CreateTextureResource(device, image->pixels.data(), image->width, image->height, useSrgb);
}

void Texture2D::InitializeSolidColor(ID3D12Device* device, UINT8 red, UINT8 green, UINT8 blue, UINT8 alpha, bool useSrgb)
{
	const UINT8 pixels[] = { red, green, blue, alpha };
	CreateTextureResource(device, pixels, 1, 1, useSrgb);
}

void Texture2D::InitializeFromPixels(ID3D12Device* device, const void* rgbaPixels, UINT width, UINT height, bool useSrgb)
{
	if (rgbaPixels == nullptr || width == 0 || height == 0)
	{
		throw std::runtime_error("Texture pixel data is invalid.");
	}

	CreateTextureResource(device, rgbaPixels, width, height, useSrgb);
}

std::uint64_t Texture2D::GetUploadCount() { return uploadCount.load(); }

void Texture2D::CreateTextureResource(ID3D12Device* device, const void* pixels, UINT width, UINT height, bool useSrgb)
{
	m_format = useSrgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;

	D3D12_RESOURCE_DESC textureDesc{};
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureDesc.Alignment = 0;
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.MipLevels = 1;
	textureDesc.Format = m_format;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12_HEAP_PROPERTIES defaultHeapProperties{};
	defaultHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	defaultHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	defaultHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	defaultHeapProperties.CreationNodeMask = 1;
	defaultHeapProperties.VisibleNodeMask = 1;

	ThrowIfFailed(device->CreateCommittedResource(
		&defaultHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&m_resource)));

	UploadPixels(device, pixels, textureDesc);
	++uploadCount;
}

void Texture2D::UploadPixels(ID3D12Device* device, const void* pixels, const D3D12_RESOURCE_DESC& textureDesc)
{
	UINT64 uploadBufferSize = 0;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout{};
	UINT rowCount = 0;
	UINT64 rowSizeInBytes = 0;
	device->GetCopyableFootprints(&textureDesc, 0, 1, 0, &layout, &rowCount, &rowSizeInBytes, &uploadBufferSize);

	ComPtr<ID3D12Resource> uploadBuffer = Dx12BufferHelper::CreateUploadBuffer(device, uploadBufferSize);

	UINT8* mappedData = nullptr;
	D3D12_RANGE readRange{};
	ThrowIfFailed(uploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mappedData)));

	const UINT8* sourcePixels = static_cast<const UINT8*>(pixels);
	for (UINT row = 0; row < rowCount; ++row)
	{
		memcpy(
			mappedData + layout.Offset + static_cast<size_t>(layout.Footprint.RowPitch) * row,
			sourcePixels + static_cast<size_t>(rowSizeInBytes) * row,
			static_cast<size_t>(rowSizeInBytes));
	}
	uploadBuffer->Unmap(0, nullptr);

	ComPtr<ID3D12CommandQueue> commandQueue;
	D3D12_COMMAND_QUEUE_DESC queueDesc{};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));

	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));

	ComPtr<ID3D12GraphicsCommandList> commandList;
	ThrowIfFailed(device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		commandAllocator.Get(),
		nullptr,
		IID_PPV_ARGS(&commandList)));

	D3D12_TEXTURE_COPY_LOCATION destination{};
	destination.pResource = m_resource.Get();
	destination.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	destination.SubresourceIndex = 0;

	D3D12_TEXTURE_COPY_LOCATION source{};
	source.pResource = uploadBuffer.Get();
	source.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	source.PlacedFootprint = layout;

	commandList->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = m_resource.Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	commandList->ResourceBarrier(1, &barrier);

	ExecuteAndWait(device, commandQueue.Get(), commandList.Get());
}

void Texture2D::CreateShaderResourceView(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE handle) const
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = m_format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	device->CreateShaderResourceView(m_resource.Get(), &srvDesc, handle);
}
