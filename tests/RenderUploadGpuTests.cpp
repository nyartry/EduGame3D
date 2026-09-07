#include "Framework/Rendering/Core/ConstantBufferRing.h"

#include <algorithm>
#include <array>
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include <stdexcept>

using Microsoft::WRL::ComPtr;

namespace
{
	ComPtr<ID3D12Resource> CreateBuffer(ID3D12Device* device, UINT64 size, D3D12_HEAP_TYPE heapType,
		D3D12_RESOURCE_STATES state, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE)
	{
		D3D12_HEAP_PROPERTIES heap{};
		heap.Type = heapType;
		D3D12_RESOURCE_DESC desc{};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Width = size;
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.SampleDesc.Count = 1;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = flags;
		ComPtr<ID3D12Resource> resource;
		ThrowIfFailed(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc, state, nullptr, IID_PPV_ARGS(&resource)));
		return resource;
	}

	// Release a deliberately blocked queue even if setup or command recording fails.
	struct QueueGate
	{
		ComPtr<ID3D12Fence> fence;
		~QueueGate() { if (fence) fence->Signal(1); }
	};
}

void RunRenderUploadGpuTests()
{
	constexpr UINT drawsPerFrame = 1025;
	constexpr UINT frameCount = 3;
	constexpr UINT resultCount = drawsPerFrame * frameCount * 2;
	constexpr UINT resultBytes = resultCount * sizeof(UINT);
	ComPtr<IDXGIFactory4> factory;
	ThrowIfFailed(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)));
	ComPtr<IDXGIAdapter> adapter;
	ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));
	ComPtr<ID3D12Device> device;
	ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)));
	D3D12_COMMAND_QUEUE_DESC queueDesc{};
	ComPtr<ID3D12CommandQueue> queue;
	ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&queue)));
	ComPtr<ID3D12Fence> completed;
	ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&completed)));

	D3D12_ROOT_PARAMETER parameters[2]{};
	parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
	D3D12_ROOT_SIGNATURE_DESC signatureDesc{};
	signatureDesc.NumParameters = 2;
	signatureDesc.pParameters = parameters;
	ComPtr<ID3DBlob> serialized, errors;
	ThrowIfFailed(D3D12SerializeRootSignature(&signatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serialized, &errors));
	ComPtr<ID3D12RootSignature> signature;
	ThrowIfFailed(device->CreateRootSignature(0, serialized->GetBufferPointer(), serialized->GetBufferSize(), IID_PPV_ARGS(&signature)));
	constexpr char shader[] = "cbuffer Value : register(b0) { uint value; };"
		"RWByteAddressBuffer output : register(u0);"
		"[numthreads(1,1,1)] void main() { output.Store(0, value); }";
	ComPtr<ID3DBlob> shaderCode;
	ThrowIfFailed(D3DCompile(shader, sizeof(shader) - 1, nullptr, nullptr, nullptr, "main", "cs_5_0", 0, 0, &shaderCode, &errors));
	D3D12_COMPUTE_PIPELINE_STATE_DESC pipelineDesc{};
	pipelineDesc.pRootSignature = signature.Get();
	pipelineDesc.CS = { shaderCode->GetBufferPointer(), shaderCode->GetBufferSize() };
	ComPtr<ID3D12PipelineState> pipeline;
	ThrowIfFailed(device->CreateComputePipelineState(&pipelineDesc, IID_PPV_ARGS(&pipeline)));
	auto output = CreateBuffer(device.Get(), resultBytes, D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	auto readback = CreateBuffer(device.Get(), resultBytes, D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_STATE_COPY_DEST);
	ConstantBufferRing<UINT, 1024> constants;
	constants.Initialize(device.Get());
	FrameUploadBuffer vertexUpload(512);
	vertexUpload.Initialize(device.Get());
	std::array<ComPtr<ID3D12CommandAllocator>, frameCount + 1> allocators;
	std::array<ComPtr<ID3D12GraphicsCommandList>, frameCount + 1> lists;
	for (UINT index = 0; index <= frameCount; ++index)
	{
		ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocators[index])));
		ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocators[index].Get(), pipeline.Get(), IID_PPV_ARGS(&lists[index])));
	}
	QueueGate gate;
	ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&gate.fence)));
	ThrowIfFailed(queue->Wait(gate.fence.Get(), 1));
	D3D12_GPU_VIRTUAL_ADDRESS firstAddress{};
	for (UINT frame = 0; frame < frameCount; ++frame)
	{
		constants.BeginFrame(completed->GetCompletedValue());
		vertexUpload.BeginFrame(completed->GetCompletedValue());
		auto* list = lists[frame].Get();
		list->SetComputeRootSignature(signature.Get());
		for (UINT draw = 0; draw < drawsPerFrame; ++draw)
		{
			const UINT expected = (frame * drawsPerFrame + draw) * 2 + 1;
			const auto address = constants.Write(expected);
			if (frame == 0 && draw == 0) firstAddress = address;
			list->SetComputeRootConstantBufferView(0, address);
			list->SetComputeRootUnorderedAccessView(1, output->GetGPUVirtualAddress() + (expected - 1) * sizeof(UINT));
			list->Dispatch(1, 1, 1);

			// The same upload buffer snapshots mutable sprite/skinned vertices at
			// draw time. Exercise ordinary and oversized pages, then mutate the
			// original CPU array repeatedly while all frames are still in flight.
			std::vector<UINT> vertices(draw % 5 == 0 ? 513 : 9, expected + 1);
			const auto vertexAddress = vertexUpload.Write(vertices.data(), vertices.size() * sizeof(UINT));
			std::fill(vertices.begin(), vertices.end(), 0xBADu);
			list->SetComputeRootConstantBufferView(0, vertexAddress);
			list->SetComputeRootUnorderedAccessView(1, output->GetGPUVirtualAddress() + expected * sizeof(UINT));
			list->Dispatch(1, 1, 1);
		}
		ThrowIfFailed(list->Close());
		ID3D12CommandList* submitted[] = { list };
		queue->ExecuteCommandLists(1, submitted);
		ThrowIfFailed(queue->Signal(completed.Get(), frame + 1));
		constants.EndFrame(frame + 1);
		vertexUpload.EndFrame(frame + 1);
	}
	auto* copyList = lists[frameCount].Get();
	D3D12_RESOURCE_BARRIER transition{};
	transition.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	transition.Transition.pResource = output.Get();
	transition.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	transition.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
	transition.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	copyList->ResourceBarrier(1, &transition);
	copyList->CopyResource(readback.Get(), output.Get());
	ThrowIfFailed(copyList->Close());
	ID3D12CommandList* copyCommands[] = { copyList };
	queue->ExecuteCommandLists(1, copyCommands);
	ThrowIfFailed(queue->Signal(completed.Get(), frameCount + 1));
	ThrowIfFailed(gate.fence->Signal(1));
	HANDLE event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (event == nullptr) ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	const HRESULT waitSetup = completed->SetEventOnCompletion(frameCount + 1, event);
	const DWORD waitResult = SUCCEEDED(waitSetup) ? WaitForSingleObject(event, 20000) : WAIT_FAILED;
	CloseHandle(event);
	ThrowIfFailed(waitSetup);
	if (waitResult != WAIT_OBJECT_0) throw std::runtime_error("WARP upload test timed out waiting for the GPU.");

	UINT* values = nullptr;
	D3D12_RANGE range{ 0, resultBytes };
	ThrowIfFailed(readback->Map(0, &range, reinterpret_cast<void**>(&values)));
	bool valid = true;
	for (UINT index = 0; index < resultCount; ++index)
	{
		if (values[index] != index + 1) valid = false;
	}
	D3D12_RANGE noWrites{};
	readback->Unmap(0, &noWrites);
	if (!valid) throw std::runtime_error("The GPU observed overwritten draw constants or vertex snapshots.");
	constants.BeginFrame(completed->GetCompletedValue());
	if (constants.Write(42) != firstAddress) throw std::runtime_error("Completed upload pages were not reused.");
	constants.EndFrame(frameCount + 2);
}
