#pragma once

#include "Framework/Common/Common.h"
#include "Framework/Rendering/Core/Dx12BufferHelper.h"
#include "Framework/Rendering/Core/FenceRetiredPagePool.h"

#include <cstring>
#include <limits>
#include <utility>

// Render-thread upload storage. Each Write creates an immutable snapshot used by
// one draw. Pages grow on demand and survive until the renderer has drained its
// queue at shutdown; only fence-completed pages are overwritten by later frames.
class FrameUploadBuffer
{
public:
	explicit FrameUploadBuffer(std::size_t pageSize = 1024 * 1024)
		: m_allocations(pageSize)
	{
	}

	FrameUploadBuffer(const FrameUploadBuffer&) = delete;
	FrameUploadBuffer& operator=(const FrameUploadBuffer&) = delete;

	void Initialize(ID3D12Device* device)
	{
		if (device == nullptr || m_device != nullptr)
		{
			throw std::logic_error("An upload buffer must be initialized once with a device.");
		}
		m_device = device;
	}

	void BeginFrame(UINT64 completedFence)
	{
		if (m_device == nullptr)
		{
			throw std::logic_error("The upload buffer has not been initialized.");
		}
		m_allocations.BeginFrame(completedFence);
	}

	void EndFrame(UINT64 submittedFence)
	{
		m_allocations.EndFrame(submittedFence);
	}

	D3D12_GPU_VIRTUAL_ADDRESS Write(const void* data, std::size_t size)
	{
		constexpr std::size_t alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
		if (data == nullptr || size == 0 || size > (std::numeric_limits<std::size_t>::max)() - (alignment - 1))
		{
			throw std::invalid_argument("Upload data must have a nonzero, representable size.");
		}
		const auto alignedSize = (size + alignment - 1) & ~(alignment - 1);
		const auto allocation = m_allocations.Allocate(alignedSize);
		while (m_pages.size() <= allocation.page)
		{
			Page page;
			page.resource = Dx12BufferHelper::CreateUploadBuffer(
				m_device.Get(), m_allocations.GetPageCapacity(m_pages.size()));
			D3D12_RANGE readRange{};
			ThrowIfFailed(page.resource->Map(0, &readRange, reinterpret_cast<void**>(&page.mappedData)));
			m_pages.push_back(std::move(page));
		}
		const Page& page = m_pages[allocation.page];
		std::memcpy(page.mappedData + allocation.slot, data, size);
		return page.resource->GetGPUVirtualAddress() + allocation.slot;
	}

private:
	struct Page
	{
		Microsoft::WRL::ComPtr<ID3D12Resource> resource;
		UINT8* mappedData{};
	};

	Microsoft::WRL::ComPtr<ID3D12Device> m_device;
	std::vector<Page> m_pages;
	FenceRetiredPagePool m_allocations;
};
