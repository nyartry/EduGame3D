#pragma once

#include <Windows.h>
#include <wrl/client.h>

#include <array>
#include <chrono>
#include <d3d12.h>
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

	void LoadPipeline();
	void LoadAssets();
	void CreateVertexBuffer();
	void PopulateCommandList();
	void MoveToNextFrame();

	HWND m_hwnd{};
	UINT m_width{};
	UINT m_height{};
	UINT m_frameIndex{};
	UINT m_rtvDescriptorSize{};
	HANDLE m_fenceEvent{};
	std::chrono::steady_clock::time_point m_startTime{};
	std::array<float, 4> m_clearColor{ 0.04f, 0.06f, 0.10f, 1.0f };

	Microsoft::WRL::ComPtr<ID3D12Device> m_device;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
	Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, FrameCount> m_renderTargets;
	std::array<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>, FrameCount> m_commandAllocators;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView{};
	Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
	std::array<UINT64, FrameCount> m_fenceValues{};

	std::array<Vertex, 3> m_triangleVertices =
	{
		Vertex{ { 0.0f, 0.45f, 0.0f }, { 0.10f, 0.85f, 1.0f, 1.0f } },
		Vertex{ { 0.45f, -0.35f, 0.0f }, { 1.0f, 0.75f, 0.10f, 1.0f } },
		Vertex{ { -0.45f, -0.35f, 0.0f }, { 0.95f, 0.20f, 0.35f, 1.0f } },
	};
};

