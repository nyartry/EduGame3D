#include "SkinningTestCases.h"
#include "TestSupport.h"
#include "Framework/Rendering/Pipelines/SkinnedTexturedPipeline.h"

#include <array>
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include <fstream>
#include <iterator>
#include <stdexcept>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

namespace
{
	ComPtr<ID3D12Resource> CreateBuffer(ID3D12Device* device, UINT64 size, D3D12_HEAP_TYPE type,
		D3D12_RESOURCE_STATES state, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE)
	{
		D3D12_HEAP_PROPERTIES heap{};
		heap.Type = type;
		D3D12_RESOURCE_DESC desc{};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Width = size;
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.SampleDesc.Count = 1;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = flags;
		ComPtr<ID3D12Resource> result;
		ThrowIfFailed(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc, state, nullptr, IID_PPV_ARGS(&result)));
		return result;
	}
}

void RunSkinningGpuTests()
{
	TestSupport::RepositoryDirectory repository;
	const auto cases = CreateSkinningTestCases();
	constexpr UINT outputStride = 12 * sizeof(float);
	const auto resultBytes = cases.size() * outputStride;
	ComPtr<IDXGIFactory4> factory;
	ThrowIfFailed(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)));
	ComPtr<IDXGIAdapter> adapter;
	ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));
	ComPtr<ID3D12Device> device;
	ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)));
	SkinnedTexturedPipeline productionPipeline;
	productionPipeline.Initialize(device.Get());
	productionPipeline.BeginFrame(0);

	std::ifstream sourceFile("src/Framework/Rendering/Shaders/SkinnedTextured.hlsl", std::ios::binary);
	std::string source((std::istreambuf_iterator<char>(sourceFile)), std::istreambuf_iterator<char>());
	source += R"(
cbuffer TestInput : register(b2) {
 float4 testPosition; float4 testNormal; float4 testTangent;
 int4 testIndices; float4 testWeights;
};
RWByteAddressBuffer testOutput : register(u0);
[numthreads(1,1,1)] void CSMain() {
 VSInput input = (VSInput)0;
 input.position = testPosition.xyz; input.normal = testNormal.xyz;
 input.tangent = testTangent.xyz; input.boneIndices = testIndices;
 input.boneWeights = testWeights;
 PSInput result = VSMain(input);
 testOutput.Store3(0, asuint(result.position.xyz));
 testOutput.Store3(16, asuint(result.normal));
 testOutput.Store3(32, asuint(result.tangent));
})";
	ComPtr<ID3DBlob> code, errors;
	UINT shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS;
#if defined(_DEBUG)
	shaderFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	shaderFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif
	const auto compileResult = D3DCompile(source.data(), source.size(), "SkinningGpuTests", nullptr, nullptr,
		"CSMain", "cs_5_0", shaderFlags, 0, &code, &errors);
	if (FAILED(compileResult) && errors) throw std::runtime_error(static_cast<const char*>(errors->GetBufferPointer()));
	ThrowIfFailed(compileResult);
	D3D12_ROOT_PARAMETER parameters[4]{};
	for (UINT index = 0; index < 3; ++index)
	{
		parameters[index].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		parameters[index].Descriptor.ShaderRegister = index;
	}
	parameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
	D3D12_ROOT_SIGNATURE_DESC rootDesc{};
	rootDesc.NumParameters = 4;
	rootDesc.pParameters = parameters;
	ComPtr<ID3DBlob> serialized;
	ThrowIfFailed(D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serialized, &errors));
	ComPtr<ID3D12RootSignature> root;
	ThrowIfFailed(device->CreateRootSignature(0, serialized->GetBufferPointer(), serialized->GetBufferSize(), IID_PPV_ARGS(&root)));
	D3D12_COMPUTE_PIPELINE_STATE_DESC pipelineDesc{};
	pipelineDesc.pRootSignature = root.Get();
	pipelineDesc.CS = { code->GetBufferPointer(), code->GetBufferSize() };
	ComPtr<ID3D12PipelineState> computePipeline;
	ThrowIfFailed(device->CreateComputePipelineState(&pipelineDesc, IID_PPV_ARGS(&computePipeline)));
	D3D12_COMMAND_QUEUE_DESC queueDesc{};
	ComPtr<ID3D12CommandQueue> queue;
	ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&queue)));
	ComPtr<ID3D12CommandAllocator> allocator;
	ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)));
	ComPtr<ID3D12GraphicsCommandList> list;
	ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), computePipeline.Get(), IID_PPV_ARGS(&list)));
	auto output = CreateBuffer(device.Get(), resultBytes, D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	auto readback = CreateBuffer(device.Get(), resultBytes, D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_STATE_COPY_DEST);
	struct InputConstants
	{
		XMFLOAT4 position, normal, tangent;
		std::array<int, 4> indices;
		std::array<float, 4> weights;
	};
	ConstantBufferRing<InputConstants, 16> inputUpload;
	inputUpload.Initialize(device.Get());
	inputUpload.BeginFrame(0);
	const XMMATRIX world = XMMatrixScaling(0.75f, 2.0f, 1.0f) * XMMatrixRotationZ(0.3f);
	list->SetComputeRootSignature(root.Get());
	for (std::size_t index = 0; index < cases.size(); ++index)
	{
		const auto& test = cases[index];
		const auto buffers = productionPipeline.UpdateConstants(world, world, test.bones, 0.5f, -1.0f, 0.25f, 0.75f);
		const auto& v = test.vertex.vertex;
		InputConstants input{ { v.position.x, v.position.y, v.position.z, 1.0f },
			{ v.normal.x, v.normal.y, v.normal.z, 0.0f }, { v.tangent.x, v.tangent.y, v.tangent.z, 0.0f } };
		std::copy(std::begin(test.vertex.boneIndices), std::end(test.vertex.boneIndices), input.indices.begin());
		std::copy(std::begin(test.vertex.boneWeights), std::end(test.vertex.boneWeights), input.weights.begin());
		list->SetComputeRootConstantBufferView(0, buffers.sceneConstants);
		list->SetComputeRootConstantBufferView(1, buffers.boneConstants);
		list->SetComputeRootConstantBufferView(2, inputUpload.Write(input));
		list->SetComputeRootUnorderedAccessView(3, output->GetGPUVirtualAddress() + index * outputStride);
		list->Dispatch(1, 1, 1);
	}
	D3D12_RESOURCE_BARRIER transition{};
	transition.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	transition.Transition.pResource = output.Get();
	transition.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	transition.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
	transition.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	list->ResourceBarrier(1, &transition);
	list->CopyResource(readback.Get(), output.Get());
	ThrowIfFailed(list->Close());
	ID3D12CommandList* submitted[] = { list.Get() };
	queue->ExecuteCommandLists(1, submitted);
	ComPtr<ID3D12Fence> fence;
	ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
	ThrowIfFailed(queue->Signal(fence.Get(), 1));
	productionPipeline.EndFrame(1);
	inputUpload.EndFrame(1);
	HANDLE event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (event == nullptr) ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	const HRESULT waitSetup = fence->SetEventOnCompletion(1, event);
	const DWORD waitResult = SUCCEEDED(waitSetup) ? WaitForSingleObject(event, 20000) : WAIT_FAILED;
	CloseHandle(event);
	ThrowIfFailed(waitSetup);
	if (waitResult != WAIT_OBJECT_0) throw std::runtime_error("WARP skinning test timed out.");
	float* values = nullptr;
	D3D12_RANGE range{ 0, resultBytes };
	ThrowIfFailed(readback->Map(0, &range, reinterpret_cast<void**>(&values)));
	std::string failure;
	std::vector<XMFLOAT4X4> normalBones;
	for (std::size_t index = 0; index < cases.size(); ++index)
	{
		const auto& test = cases[index];
		BoneSkinning::BuildNormalPalette(test.bones, normalBones);
		auto cpu = BoneSkinning::DeformVertex(test.vertex, test.bones, normalBones);
		cpu.position = { (cpu.position.x - 0.5f) * 0.75f, (cpu.position.y + 1.0f) * 0.75f, (cpu.position.z - 0.25f) * 0.75f };
		std::array<XMFLOAT3, 3> expected;
		XMStoreFloat3(&expected[0], XMVector3TransformCoord(XMLoadFloat3(&cpu.position), world));
		XMStoreFloat3(&expected[1], XMVector3Normalize(XMVector3TransformNormal(XMLoadFloat3(&cpu.normal), BoneSkinning::CreateNormalMatrix(world))));
		XMStoreFloat3(&expected[2], XMVector3Normalize(XMVector3TransformNormal(XMLoadFloat3(&cpu.tangent), world)));
		for (std::size_t attribute = 0; attribute < 3; ++attribute)
		{
			const auto& expectedVector = expected[attribute];
			const std::array<float, 3> expectedValues{ expectedVector.x, expectedVector.y, expectedVector.z };
			for (std::size_t component = 0; component < 3; ++component)
			{
				const float actual = values[index * 12 + attribute * 4 + component];
				if (!std::isfinite(actual) || std::abs(actual - expectedValues[component]) > 1.0e-4f)
				{
					failure = std::string(test.name) + " attribute " + std::to_string(attribute) +
						" component " + std::to_string(component) + ": expected " + std::to_string(expectedValues[component]) +
						", got " + std::to_string(actual);
				}
			}
		}
	}
	D3D12_RANGE noWrites{};
	readback->Unmap(0, &noWrites);
	if (!failure.empty()) throw std::runtime_error("CPU/GPU skinning mismatch: " + failure);
}
