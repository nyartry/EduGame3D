#pragma once

#include "Framework/Rendering/Core/FrameUploadBuffer.h"

#include <type_traits>

// Capacity is the page size in constants, not a per-frame draw limit. Allocations
// never wrap: each frame retires its pages against the renderer's submission fence.
template <typename T, UINT Capacity>
class ConstantBufferRing
{
	static_assert(Capacity > 0);
	static_assert(std::is_trivially_copyable_v<T>);
	static_assert(sizeof(T) <= D3D12_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16);

public:
	void Initialize(ID3D12Device* device) { m_upload.Initialize(device); }
	void BeginFrame(UINT64 completedFence) { m_upload.BeginFrame(completedFence); }
	void EndFrame(UINT64 submittedFence) { m_upload.EndFrame(submittedFence); }
	D3D12_GPU_VIRTUAL_ADDRESS Write(const T& constants) { return m_upload.Write(&constants, sizeof(constants)); }

private:
	FrameUploadBuffer m_upload{ static_cast<std::size_t>(Dx12BufferHelper::AlignConstantBufferSize(sizeof(T))) * Capacity };
};
