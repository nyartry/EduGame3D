#pragma once

#include <Windows.h>
#include <wrl/client.h>

#include "Rendering/BasicColorPipeline.h"
#include "Rendering/SkinnedTexturedPipeline.h"
#include "Rendering/TexturedPipeline.h"
#include "Rendering/VertexBuffer.h"

#include <array>
#include <chrono>
#include <d3d12.h>
#include <DirectXMath.h>
#include <dxgi1_6.h>
#include <vector>

class TexturedMaterial;
class SkinnedVertexBuffer;
class TexturedVertexBuffer;

class Dx12Renderer
{
public:
	static constexpr UINT FrameCount = 2;

	Dx12Renderer() = default;
	~Dx12Renderer();

	Dx12Renderer(const Dx12Renderer&) = delete;
	Dx12Renderer& operator=(const Dx12Renderer&) = delete;

	void Initialize(HWND hwnd, UINT width, UINT height);
	void BeginFrame(const DirectX::XMMATRIX& viewProjection);
	void Draw(const VertexBuffer& vertexBuffer, const DirectX::XMMATRIX& world);
	void DrawTextured(const TexturedVertexBuffer& vertexBuffer, const TexturedMaterial& material, const DirectX::XMMATRIX& world);
	void DrawSkinnedTextured(
		const SkinnedVertexBuffer& vertexBuffer,
		const TexturedMaterial& material,
		const DirectX::XMMATRIX& world,
		const std::vector<DirectX::XMFLOAT4X4>& boneMatrices,
		float modelCenterX,
		float modelMinY,
		float modelCenterZ,
		float modelScale);
	void EndFrame();
	void WaitForGpu();
	ID3D12Device* GetDevice() const;

private:
	void LoadPipeline();
	void LoadAssets();
	void CreateDepthBuffer();
	void UpdateClearColor();
	void MoveToNextFrame();

	HWND m_hwnd{};
	UINT m_width{};
	UINT m_height{};
	UINT m_frameIndex{};
	UINT m_rtvDescriptorSize{};
	HANDLE m_fenceEvent{};
	std::chrono::steady_clock::time_point m_startTime{};
	std::array<float, 4> m_clearColor{ 0.08f, 0.12f, 0.18f, 1.0f };
	DirectX::XMFLOAT4X4 m_viewProjection{};
	BasicColorPipeline m_basicColorPipeline;
	TexturedPipeline m_texturedPipeline;
	SkinnedTexturedPipeline m_skinnedTexturedPipeline;

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
};
