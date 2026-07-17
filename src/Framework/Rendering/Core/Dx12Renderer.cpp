#include "Framework/Rendering/Core/Dx12Renderer.h"

#include "Framework/Common/Common.h"
#include "Framework/Rendering/Buffers/SkinnedVertexBuffer.h"
#include "Framework/Rendering/Materials/SpriteMaterial.h"
#include "Framework/Rendering/Buffers/SpriteVertexBuffer.h"
#include "Framework/Rendering/Materials/TexturedMaterial.h"
#include "Framework/Rendering/Buffers/TexturedVertexBuffer.h"
#include "Framework/Rendering/Core/RenderResourceAccess.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

void Dx12Renderer::CreateVertexBuffer(VertexBuffer& buffer, const std::vector<Vertex>& vertices)
{
	RenderResourceAccess::Initialize(buffer, m_device.Get(), vertices);
}

void Dx12Renderer::CreateTexturedVertexBuffer(TexturedVertexBuffer& buffer, const std::vector<TexturedVertex>& vertices)
{
	RenderResourceAccess::Initialize(buffer, m_device.Get(), vertices);
}

void Dx12Renderer::CreateSkinnedVertexBuffer(SkinnedVertexBuffer& buffer, const std::vector<SkinnedVertex>& vertices)
{
	RenderResourceAccess::Initialize(buffer, m_device.Get(), vertices);
}

void Dx12Renderer::CreateSpriteVertexBuffer(SpriteVertexBuffer& buffer, std::uint32_t vertexCapacity)
{
	RenderResourceAccess::Initialize(buffer, m_device.Get(), vertexCapacity);
}

void Dx12Renderer::CreateSolidColorSpriteMaterial(
	SpriteMaterial& material,
	std::uint8_t red,
	std::uint8_t green,
	std::uint8_t blue,
	std::uint8_t alpha)
{
	RenderResourceAccess::InitializeSolidColor(material, m_device.Get(), red, green, blue, alpha);
}

void Dx12Renderer::CreateTextureSpriteMaterial(SpriteMaterial& material, const std::string& texturePath, bool useSrgb)
{
	RenderResourceAccess::InitializeTexture(material, m_device.Get(), texturePath, useSrgb);
}

void Dx12Renderer::CreatePixelSpriteMaterial(
	SpriteMaterial& material,
	const std::vector<std::uint8_t>& rgbaPixels,
	std::uint32_t width,
	std::uint32_t height,
	bool useSrgb)
{
	RenderResourceAccess::InitializePixels(material, m_device.Get(), rgbaPixels, width, height, useSrgb);
}

void Dx12Renderer::CreateTexturedMaterial(
	TexturedMaterial& material,
	const std::string& baseColorTexturePath,
	const std::string& opacityTexturePath,
	const std::string& normalTexturePath)
{
	RenderResourceAccess::Initialize(material, m_device.Get(), baseColorTexturePath, opacityTexturePath, normalTexturePath);
}

namespace
{
	constexpr DXGI_FORMAT DepthStencilFormat = DXGI_FORMAT_D32_FLOAT;
}

Dx12Renderer::~Dx12Renderer()
{
	WaitForGpu();
	CollectDeferredReleases();
	for (DeferredRelease& deferred : m_deferredReleases)
	{
		deferred.release();
	}
	m_deferredReleases.clear();
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

void Dx12Renderer::Resize(UINT width, UINT height)
{
	if (width == 0 || height == 0 || m_swapChain == nullptr)
	{
		return;
	}

	if (width == m_width && height == m_height)
	{
		return;
	}

	WaitForGpu();

	for (ComPtr<ID3D12Resource>& renderTarget : m_renderTargets)
	{
		renderTarget.Reset();
	}
	m_depthStencil.Reset();

	ThrowIfFailed(m_swapChain->ResizeBuffers(
		FrameCount,
		width,
		height,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		0));

	m_width = width;
	m_height = height;
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
	for (UINT index = 0; index < FrameCount; ++index)
	{
		ThrowIfFailed(m_swapChain->GetBuffer(index, IID_PPV_ARGS(&m_renderTargets[index])));
		m_device->CreateRenderTargetView(m_renderTargets[index].Get(), nullptr, rtvHandle);
		rtvHandle.ptr += m_rtvDescriptorSize;
	}

	CreateDepthBuffer();
}

void Dx12Renderer::BeginFrame(const XMMATRIX& viewProjection)
{
	XMStoreFloat4x4(&m_viewProjection, viewProjection);
	UpdateClearColor();

	ThrowIfFailed(m_commandAllocators[m_frameIndex]->Reset());
	ThrowIfFailed(m_commandList->Reset(m_commandAllocators[m_frameIndex].Get(), m_basicColorPipeline.GetPipelineState()));

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
	m_basicColorPipeline.Bind(m_commandList.Get());

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
}

void Dx12Renderer::Draw(const VertexBuffer& vertexBuffer, const XMMATRIX& world)
{
	m_commandList->SetPipelineState(m_basicColorPipeline.GetPipelineState());
	m_basicColorPipeline.Bind(m_commandList.Get());

	const XMMATRIX viewProjection = XMLoadFloat4x4(&m_viewProjection);
	const XMMATRIX worldViewProjection = world * viewProjection;
	const D3D12_GPU_VIRTUAL_ADDRESS sceneConstantsAddress = m_basicColorPipeline.UpdateWorldViewProjection(worldViewProjection);
	m_commandList->SetGraphicsRootConstantBufferView(0, sceneConstantsAddress);
	RenderResourceAccess::Bind(vertexBuffer, m_commandList.Get());
	m_commandList->DrawInstanced(vertexBuffer.GetVertexCount(), 1, 0, 0);
}

void Dx12Renderer::DrawScreen(const VertexBuffer& vertexBuffer, const XMMATRIX& world)
{
	m_commandList->SetPipelineState(m_basicColorPipeline.GetPipelineState());
	m_basicColorPipeline.Bind(m_commandList.Get());

	const D3D12_GPU_VIRTUAL_ADDRESS sceneConstantsAddress = m_basicColorPipeline.UpdateWorldViewProjection(world);
	m_commandList->SetGraphicsRootConstantBufferView(0, sceneConstantsAddress);
	RenderResourceAccess::Bind(vertexBuffer, m_commandList.Get());
	m_commandList->DrawInstanced(vertexBuffer.GetVertexCount(), 1, 0, 0);
}

void Dx12Renderer::DrawTextured(const TexturedVertexBuffer& vertexBuffer, const TexturedMaterial& material, const XMMATRIX& world)
{
	m_commandList->SetPipelineState(m_texturedPipeline.GetPipelineState());
	m_texturedPipeline.Bind(m_commandList.Get());

	const XMMATRIX viewProjection = XMLoadFloat4x4(&m_viewProjection);
	const XMMATRIX worldViewProjection = world * viewProjection;
	const D3D12_GPU_VIRTUAL_ADDRESS sceneConstantsAddress = m_texturedPipeline.UpdateWorldViewProjection(worldViewProjection);
	m_commandList->SetGraphicsRootConstantBufferView(0, sceneConstantsAddress);
	RenderResourceAccess::Bind(material, m_commandList.Get(), 1);
	RenderResourceAccess::Bind(vertexBuffer, m_commandList.Get());
	m_commandList->DrawInstanced(vertexBuffer.GetVertexCount(), 1, 0, 0);
}

void Dx12Renderer::DrawSprites(const SpriteVertexBuffer& vertexBuffer, const SpriteMaterial& material)
{
	if (vertexBuffer.GetVertexCount() == 0)
	{
		return;
	}

	m_commandList->SetPipelineState(m_spritePipeline.GetPipelineState());
	m_spritePipeline.Bind(m_commandList.Get());

	const D3D12_GPU_VIRTUAL_ADDRESS constantsAddress = m_spritePipeline.UpdateScreenSize(m_width, m_height);
	m_commandList->SetGraphicsRootConstantBufferView(0, constantsAddress);
	RenderResourceAccess::Bind(material, m_commandList.Get(), 1);
	RenderResourceAccess::Bind(vertexBuffer, m_commandList.Get());
	m_commandList->DrawInstanced(vertexBuffer.GetVertexCount(), 1, 0, 0);
}

void Dx12Renderer::DrawSkinnedTextured(
	const SkinnedVertexBuffer& vertexBuffer,
	const TexturedMaterial& material,
	const XMMATRIX& world,
	const std::vector<XMFLOAT4X4>& boneMatrices,
	float modelCenterX,
	float modelMinY,
	float modelCenterZ,
	float modelScale)
{
	m_commandList->SetPipelineState(m_skinnedTexturedPipeline.GetPipelineState());
	m_skinnedTexturedPipeline.Bind(m_commandList.Get());
	RenderResourceAccess::Bind(material, m_commandList.Get(), 2);

	const XMMATRIX viewProjection = XMLoadFloat4x4(&m_viewProjection);
	const XMMATRIX worldViewProjection = world * viewProjection;
	const SkinnedTexturedPipeline::ConstantBufferViews constantBufferViews = m_skinnedTexturedPipeline.UpdateConstants(
		worldViewProjection,
		boneMatrices,
		modelCenterX,
		modelMinY,
		modelCenterZ,
		modelScale);
	m_commandList->SetGraphicsRootConstantBufferView(0, constantBufferViews.sceneConstants);
	m_commandList->SetGraphicsRootConstantBufferView(1, constantBufferViews.boneConstants);
	RenderResourceAccess::Bind(vertexBuffer, m_commandList.Get());
	m_commandList->DrawInstanced(vertexBuffer.GetVertexCount(), 1, 0, 0);
}

void Dx12Renderer::EndFrame()
{
	D3D12_RESOURCE_BARRIER barrierToPresent{};
	barrierToPresent.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrierToPresent.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrierToPresent.Transition.pResource = m_renderTargets[m_frameIndex].Get();
	barrierToPresent.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrierToPresent.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	barrierToPresent.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	m_commandList->ResourceBarrier(1, &barrierToPresent);

	ThrowIfFailed(m_commandList->Close());

	ID3D12CommandList* commandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	ThrowIfFailed(m_swapChain->Present(1, 0));
	MoveToNextFrame();
}

void Dx12Renderer::WaitForGpu()
{
	FlushGpu();
}

void Dx12Renderer::DeferRelease(std::function<void()> release)
{
	if (!release)
	{
		return;
	}

	const UINT64 latestSubmittedFence = m_nextFenceValue > 1 ? m_nextFenceValue - 1 : 0;
	if (m_fence == nullptr || latestSubmittedFence == 0 || m_fence->GetCompletedValue() >= latestSubmittedFence)
	{
		release();
		return;
	}

	m_deferredReleases.push_back({ latestSubmittedFence, std::move(release) });
}

ID3D12Device* Dx12Renderer::GetDevice() const
{
	return m_device.Get();
}

ID3D12CommandQueue* Dx12Renderer::GetCommandQueue() const
{
	return m_commandQueue.Get();
}

ID3D12GraphicsCommandList* Dx12Renderer::GetCommandList() const
{
	return m_commandList.Get();
}

UINT Dx12Renderer::GetWidth() const
{
	return m_width;
}

UINT Dx12Renderer::GetHeight() const
{
	return m_height;
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
	m_basicColorPipeline.Initialize(m_device.Get());
	m_texturedPipeline.Initialize(m_device.Get());
	m_skinnedTexturedPipeline.Initialize(m_device.Get());
	m_spritePipeline.Initialize(m_device.Get());

	ThrowIfFailed(m_device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		m_commandAllocators[m_frameIndex].Get(),
		m_basicColorPipeline.GetPipelineState(),
		IID_PPV_ARGS(&m_commandList)));
	ThrowIfFailed(m_commandList->Close());

	CreateDepthBuffer();

	ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));

	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (m_fenceEvent == nullptr)
	{
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	}
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

void Dx12Renderer::UpdateClearColor()
{
	const auto now = std::chrono::steady_clock::now();
	const float seconds = std::chrono::duration<float>(now - m_startTime).count();
	const float pulse = (std::sin(seconds * 2.0f) + 1.0f) * 0.5f;
	m_clearColor = { 0.07f, 0.10f + 0.03f * pulse, 0.16f + 0.04f * pulse, 1.0f };
}

void Dx12Renderer::MoveToNextFrame()
{
	const UINT64 fenceValue = m_nextFenceValue++;
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fenceValue));
	m_fenceValues[m_frameIndex] = fenceValue;

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	if (m_fence->GetCompletedValue() < m_fenceValues[m_frameIndex])
	{
		ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));
		WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
	}
	CollectDeferredReleases();

}

void Dx12Renderer::FlushGpu()
{
	if (m_commandQueue == nullptr || m_fence == nullptr || m_fenceEvent == nullptr)
	{
		return;
	}

	const UINT64 fenceValue = m_nextFenceValue++;
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fenceValue));
	ThrowIfFailed(m_fence->SetEventOnCompletion(fenceValue, m_fenceEvent));
	WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);

	for (UINT64& frameFenceValue : m_fenceValues)
	{
		frameFenceValue = fenceValue;
	}
	CollectDeferredReleases();
}

void Dx12Renderer::CollectDeferredReleases()
{
	if (m_fence == nullptr)
	{
		return;
	}

	const UINT64 completedFence = m_fence->GetCompletedValue();
	const auto removeBegin = std::remove_if(
		m_deferredReleases.begin(),
		m_deferredReleases.end(),
		[completedFence](DeferredRelease& deferred)
		{
			if (deferred.fenceValue > completedFence)
			{
				return false;
			}
			deferred.release();
			return true;
		});
	m_deferredReleases.erase(removeBegin, m_deferredReleases.end());
}
