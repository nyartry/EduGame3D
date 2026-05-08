#include "Dx12Renderer.h"

#include "Common.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <d3dcompiler.h>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

namespace
{
constexpr DXGI_FORMAT DepthStencilFormat = DXGI_FORMAT_D32_FLOAT;

UINT AlignConstantBufferSize(UINT size)
{
	return (size + 255) & ~255u;
}
}

Dx12Renderer::~Dx12Renderer()
{
	if (m_fenceEvent != nullptr)
	{
		CloseHandle(m_fenceEvent);
	}
}

void Dx12Renderer::Initialize(HWND hwnd, UINT width, UINT height)
{
	m_hwnd = hwnd;
	m_width = width;
	m_height = height;

	LoadPipeline();
	LoadAssets();
	m_startTime = std::chrono::steady_clock::now();
}

void Dx12Renderer::Update()
{
	const auto now = std::chrono::steady_clock::now();
	const float seconds = std::chrono::duration<float>(now - m_startTime).count();
	const float pulse = (std::sin(seconds * 2.0f) + 1.0f) * 0.5f;
	m_clearColor = { 0.07f, 0.10f + 0.03f * pulse, 0.16f + 0.04f * pulse, 1.0f };

	const XMMATRIX world = XMMatrixIdentity();
	const XMVECTOR eye = XMVectorSet(0.0f, 5.0f, -8.0f, 1.0f);
	const XMVECTOR focus = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	const XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	const XMMATRIX view = XMMatrixLookAtLH(eye, focus, up);
	const float aspectRatio = static_cast<float>(m_width) / static_cast<float>(m_height);
	const XMMATRIX projection = XMMatrixPerspectiveFovLH(XMConvertToRadians(60.0f), aspectRatio, 0.1f, 100.0f);
	const XMMATRIX worldViewProjection = world * view * projection;

	XMStoreFloat4x4(&m_constantBufferData.worldViewProjection, XMMatrixTranspose(worldViewProjection));
	memcpy(m_constantBufferMappedData, &m_constantBufferData, sizeof(m_constantBufferData));
}

void Dx12Renderer::Render()
{
	PopulateCommandList();

	ID3D12CommandList* commandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	ThrowIfFailed(m_swapChain->Present(1, 0));
	MoveToNextFrame();
}

void Dx12Renderer::WaitForGpu()
{
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValues[m_frameIndex]));
	ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));
	WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
	++m_fenceValues[m_frameIndex];
}

void Dx12Renderer::LoadPipeline()
{
	UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
	ComPtr<ID3D12Debug> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
		dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif

	ComPtr<IDXGIFactory6> factory;
	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

	ComPtr<IDXGIAdapter1> adapter;
	for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
	{
		DXGI_ADAPTER_DESC1 desc{};
		adapter->GetDesc1(&desc);
		if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0)
		{
			continue;
		}

		if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device))))
		{
			break;
		}
	}

	if (!m_device)
	{
		ComPtr<IDXGIAdapter> warpAdapter;
		ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));
		ThrowIfFailed(D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)));
	}

	D3D12_COMMAND_QUEUE_DESC queueDesc{};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.BufferCount = FrameCount;
	swapChainDesc.Width = m_width;
	swapChainDesc.Height = m_height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	ComPtr<IDXGISwapChain1> swapChain;
	ThrowIfFailed(factory->CreateSwapChainForHwnd(
		m_commandQueue.Get(),
		m_hwnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain));

	ThrowIfFailed(factory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER));
	ThrowIfFailed(swapChain.As(&m_swapChain));
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
	rtvHeapDesc.NumDescriptors = FrameCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

	m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
	for (UINT index = 0; index < FrameCount; ++index)
	{
		ThrowIfFailed(m_swapChain->GetBuffer(index, IID_PPV_ARGS(&m_renderTargets[index])));
		m_device->CreateRenderTargetView(m_renderTargets[index].Get(), nullptr, rtvHandle);
		rtvHandle.ptr += m_rtvDescriptorSize;
		ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocators[index])));
	}

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));
}

void Dx12Renderer::LoadAssets()
{
	D3D12_ROOT_PARAMETER rootParameter{};
	rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameter.Descriptor.ShaderRegister = 0;
	rootParameter.Descriptor.RegisterSpace = 0;
	rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.NumParameters = 1;
	rootSignatureDesc.pParameters = &rootParameter;
	rootSignatureDesc.NumStaticSamplers = 0;
	rootSignatureDesc.pStaticSamplers = nullptr;
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));

	constexpr char shaderSource[] = R"(
struct VSInput
{
    float3 position : POSITION;
    float4 color : COLOR;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

cbuffer SceneConstants : register(b0)
{
    matrix worldViewProjection;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.position = mul(worldViewProjection, float4(input.position, 1.0f));
    output.color = input.color;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}
)";

	ComPtr<ID3DBlob> vertexShader;
	ComPtr<ID3DBlob> pixelShader;
	UINT compileFlags = 0;
#if defined(_DEBUG)
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
	ThrowIfFailed(D3DCompile(shaderSource, sizeof(shaderSource), nullptr, nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, &error));
	ThrowIfFailed(D3DCompile(shaderSource, sizeof(shaderSource), nullptr, nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, &error));

	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	rasterizerDesc.FrontCounterClockwise = FALSE;
	rasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	rasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	rasterizerDesc.DepthClipEnable = TRUE;
	rasterizerDesc.MultisampleEnable = FALSE;
	rasterizerDesc.AntialiasedLineEnable = FALSE;
	rasterizerDesc.ForcedSampleCount = 0;
	rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	D3D12_BLEND_DESC blendDesc{};
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
	{
		FALSE, FALSE,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP,
		D3D12_COLOR_WRITE_ENABLE_ALL,
	};
	for (auto& target : blendDesc.RenderTarget)
	{
		target = defaultRenderTargetBlendDesc;
	}

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = m_rootSignature.Get();
	psoDesc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
	psoDesc.PS = { pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };
	psoDesc.RasterizerState = rasterizerDesc;
	psoDesc.BlendState = blendDesc;
	psoDesc.DepthStencilState.DepthEnable = TRUE;
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DepthStencilFormat;
	psoDesc.SampleDesc.Count = 1;
	ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));

	ThrowIfFailed(m_device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		m_commandAllocators[m_frameIndex].Get(),
		m_pipelineState.Get(),
		IID_PPV_ARGS(&m_commandList)));
	ThrowIfFailed(m_commandList->Close());

	CreateDepthBuffer();
	CreateConstantBuffer();
	CreateVertexBuffer();

	ThrowIfFailed(m_device->CreateFence(m_fenceValues[m_frameIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
	++m_fenceValues[m_frameIndex];

	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (m_fenceEvent == nullptr)
	{
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	}
}

void Dx12Renderer::CreateVertexBuffer()
{
	const UINT vertexBufferSize = sizeof(m_groundVertices);

	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.CreationNodeMask = 1;
	heapProperties.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Alignment = 0;
	resourceDesc.Width = vertexBufferSize;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ThrowIfFailed(m_device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_vertexBuffer)));

	UINT8* vertexDataBegin = nullptr;
	D3D12_RANGE readRange{};
	ThrowIfFailed(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&vertexDataBegin)));
	memcpy(vertexDataBegin, m_groundVertices.data(), vertexBufferSize);
	m_vertexBuffer->Unmap(0, nullptr);

	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes = sizeof(Vertex);
	m_vertexBufferView.SizeInBytes = vertexBufferSize;
}

void Dx12Renderer::CreateDepthBuffer()
{
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.CreationNodeMask = 1;
	heapProperties.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC depthDesc{};
	depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthDesc.Alignment = 0;
	depthDesc.Width = m_width;
	depthDesc.Height = m_height;
	depthDesc.DepthOrArraySize = 1;
	depthDesc.MipLevels = 1;
	depthDesc.Format = DepthStencilFormat;
	depthDesc.SampleDesc.Count = 1;
	depthDesc.SampleDesc.Quality = 0;
	depthDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE clearValue{};
	clearValue.Format = DepthStencilFormat;
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.DepthStencil.Stencil = 0;

	ThrowIfFailed(m_device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&depthDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clearValue,
		IID_PPV_ARGS(&m_depthStencil)));

	m_device->CreateDepthStencilView(m_depthStencil.Get(), nullptr, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
}

void Dx12Renderer::CreateConstantBuffer()
{
	const UINT constantBufferSize = AlignConstantBufferSize(sizeof(SceneConstants));

	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.CreationNodeMask = 1;
	heapProperties.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Alignment = 0;
	resourceDesc.Width = constantBufferSize;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ThrowIfFailed(m_device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_constantBuffer)));

	D3D12_RANGE readRange{};
	ThrowIfFailed(m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_constantBufferMappedData)));
	Update();
}

void Dx12Renderer::PopulateCommandList()
{
	ThrowIfFailed(m_commandAllocators[m_frameIndex]->Reset());
	ThrowIfFailed(m_commandList->Reset(m_commandAllocators[m_frameIndex].Get(), m_pipelineState.Get()));

	D3D12_VIEWPORT viewport{};
	viewport.Width = static_cast<float>(m_width);
	viewport.Height = static_cast<float>(m_height);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	D3D12_RECT scissorRect{};
	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = static_cast<LONG>(m_width);
	scissorRect.bottom = static_cast<LONG>(m_height);

	m_commandList->RSSetViewports(1, &viewport);
	m_commandList->RSSetScissorRects(1, &scissorRect);
	m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
	m_commandList->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress());

	D3D12_RESOURCE_BARRIER barrierToRenderTarget{};
	barrierToRenderTarget.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrierToRenderTarget.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrierToRenderTarget.Transition.pResource = m_renderTargets[m_frameIndex].Get();
	barrierToRenderTarget.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrierToRenderTarget.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrierToRenderTarget.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	m_commandList->ResourceBarrier(1, &barrierToRenderTarget);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += static_cast<SIZE_T>(m_frameIndex) * m_rtvDescriptorSize;
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	m_commandList->ClearRenderTargetView(rtvHandle, m_clearColor.data(), 0, nullptr);
	m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	m_commandList->DrawInstanced(static_cast<UINT>(m_groundVertices.size()), 1, 0, 0);

	D3D12_RESOURCE_BARRIER barrierToPresent = barrierToRenderTarget;
	barrierToPresent.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrierToPresent.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	m_commandList->ResourceBarrier(1, &barrierToPresent);

	ThrowIfFailed(m_commandList->Close());
}

void Dx12Renderer::MoveToNextFrame()
{
	const UINT64 currentFenceValue = m_fenceValues[m_frameIndex];
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), currentFenceValue));

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	if (m_fence->GetCompletedValue() < m_fenceValues[m_frameIndex])
	{
		ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));
		WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
	}

	m_fenceValues[m_frameIndex] = currentFenceValue + 1;
}
