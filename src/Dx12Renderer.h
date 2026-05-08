#pragma once

#include <Windows.h>
#include <wrl/client.h>

#include <array>
#include <chrono>
#include <d3d12.h>
#include <DirectXMath.h>
#include <dxgi1_6.h>

class Dx12Renderer
{
public:
	static constexpr UINT FrameCount = 2;

	Dx12Renderer() = default;
	~Dx12Renderer();

	Dx12Renderer(const Dx12Renderer&) = delete;
	Dx12Renderer& operator=(const Dx12Renderer&) = delete;

	void Initialize(HWND hwnd, UINT width, UINT height);
	void Update();
	void Render();
	void WaitForGpu();

private:
	struct Vertex
	{
		float position[3];
		float color[4];
	};

	struct SceneConstants
	{
		DirectX::XMFLOAT4X4 worldViewProjection{};
	};

	void LoadPipeline();
	void LoadAssets();
	void CreateVertexBuffer();
	void CreateDepthBuffer();
	void CreateConstantBuffer();
	void PopulateCommandList();
	void MoveToNextFrame();

	HWND m_hwnd{};
	UINT m_width{};
	UINT m_height{};
	UINT m_frameIndex{};
	UINT m_rtvDescriptorSize{};
	HANDLE m_fenceEvent{};
	std::chrono::steady_clock::time_point m_startTime{};
	std::array<float, 4> m_clearColor{ 0.08f, 0.12f, 0.18f, 1.0f };
	SceneConstants m_constantBufferData{};
	UINT8* m_constantBufferMappedData{};

	Microsoft::WRL::ComPtr<ID3D12Device> m_device;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
	Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, FrameCount> m_renderTargets;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_depthStencil;
	std::array<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>, FrameCount> m_commandAllocators;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_constantBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView{};
	Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
	std::array<UINT64, FrameCount> m_fenceValues{};

	std::array<Vertex, 6> m_groundVertices =
	{
		Vertex{ { -8.0f, 0.0f, 8.0f }, { 0.28f, 0.58f, 0.28f, 1.0f } },
		Vertex{ { 8.0f, 0.0f, 8.0f }, { 0.34f, 0.68f, 0.34f, 1.0f } },
		Vertex{ { 8.0f, 0.0f, -8.0f }, { 0.18f, 0.42f, 0.22f, 1.0f } },
		Vertex{ { -8.0f, 0.0f, 8.0f }, { 0.28f, 0.58f, 0.28f, 1.0f } },
		Vertex{ { 8.0f, 0.0f, -8.0f }, { 0.18f, 0.42f, 0.22f, 1.0f } },
		Vertex{ { -8.0f, 0.0f, -8.0f }, { 0.16f, 0.36f, 0.20f, 1.0f } },
	};
};
