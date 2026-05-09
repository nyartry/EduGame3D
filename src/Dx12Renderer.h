#pragma once

#include <Windows.h>
#include <wrl/client.h>

#include "BasicColorPipeline.h"
#include "Camera.h"
#include "Ground.h"

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
	void LoadPipeline();
	void LoadAssets();
	void CreateDepthBuffer();
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
	Camera m_camera;
	BasicColorPipeline m_basicColorPipeline;

	Microsoft::WRL::ComPtr<ID3D12Device> m_device;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
	Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, FrameCount> m_renderTargets;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_depthStencil;
	std::array<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>, FrameCount> m_commandAllocators;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
	std::array<UINT64, FrameCount> m_fenceValues{};
	Ground m_ground;
};
