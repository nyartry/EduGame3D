#pragma once

#include <Windows.h>
#include <wrl/client.h>

#include "Framework/Rendering/Core/IRenderDevice.h"
#include "Framework/Rendering/Core/IRenderer.h"
#include "Framework/Rendering/Core/IRenderResourceLifetime.h"
#include "Framework/Rendering/Pipelines/BasicColorPipeline.h"
#include "Framework/Rendering/Pipelines/SkinnedTexturedPipeline.h"
#include "Framework/Rendering/Pipelines/SpritePipeline.h"
#include "Framework/Rendering/Pipelines/TexturedPipeline.h"
#include "Framework/Rendering/Buffers/VertexBuffer.h"

#include <array>
#include <chrono>
#include <d3d12.h>
#include <DirectXMath.h>
#include <dxgi1_6.h>
#include <functional>
#include <vector>

class TexturedMaterial;
class SkinnedVertexBuffer;
class SpriteMaterial;
class SpriteVertexBuffer;
class TexturedVertexBuffer;

class Dx12Renderer final : public IRenderDevice, public IRenderer, public IRenderResourceLifetime
{
public:
	static constexpr UINT FrameCount = 2;

	Dx12Renderer() = default;
	~Dx12Renderer();

	Dx12Renderer(const Dx12Renderer&) = delete;
	Dx12Renderer& operator=(const Dx12Renderer&) = delete;

	void Initialize(HWND hwnd, UINT width, UINT height);
	void Resize(UINT width, UINT height);
	void BeginFrame(const DirectX::XMMATRIX& viewProjection);
	void Draw(const VertexBuffer& vertexBuffer, const DirectX::XMMATRIX& world) override;
	void DrawScreen(const VertexBuffer& vertexBuffer, const DirectX::XMMATRIX& world) override;
	void DrawTextured(const TexturedVertexBuffer& vertexBuffer, const TexturedMaterial& material, const DirectX::XMMATRIX& world) override;
	void DrawSprites(const SpriteVertexBuffer& vertexBuffer, const SpriteMaterial& material) override;
	void DrawSkinnedTextured(
		const SkinnedVertexBuffer& vertexBuffer,
		const TexturedMaterial& material,
		const DirectX::XMMATRIX& world,
		const std::vector<DirectX::XMFLOAT4X4>& boneMatrices,
		float modelCenterX,
		float modelMinY,
		float modelCenterZ,
		float modelScale) override;

	void CreateVertexBuffer(VertexBuffer& buffer, const std::vector<Vertex>& vertices) override;
	void CreateTexturedVertexBuffer(TexturedVertexBuffer& buffer, const std::vector<TexturedVertex>& vertices) override;
	void CreateSkinnedVertexBuffer(SkinnedVertexBuffer& buffer, const std::vector<SkinnedVertex>& vertices) override;
	void CreateSpriteVertexBuffer(SpriteVertexBuffer& buffer, std::uint32_t vertexCapacity) override;
	void CreateSolidColorSpriteMaterial(
		SpriteMaterial& material,
		std::uint8_t red,
		std::uint8_t green,
		std::uint8_t blue,
		std::uint8_t alpha) override;
	void CreateTextureSpriteMaterial(SpriteMaterial& material, const std::string& texturePath, bool useSrgb) override;
	void CreatePixelSpriteMaterial(
		SpriteMaterial& material,
		const std::vector<std::uint8_t>& rgbaPixels,
		std::uint32_t width,
		std::uint32_t height,
		bool useSrgb) override;
	void CreateTexturedMaterial(
		TexturedMaterial& material,
		const std::string& baseColorTexturePath,
		const std::string& opacityTexturePath,
		const std::string& normalTexturePath) override;
	void EndFrame();
	void WaitForGpu();
	void DeferRelease(std::function<void()> release) override;
	ID3D12Device* GetDevice() const;
	ID3D12CommandQueue* GetCommandQueue() const;
	ID3D12GraphicsCommandList* GetCommandList() const;
	UINT GetWidth() const override;
	UINT GetHeight() const override;

private:
	void LoadPipeline();
	void LoadAssets();
	void CreateDepthBuffer();
	void UpdateClearColor();
	void MoveToNextFrame();
	void FlushGpu();
	void CollectDeferredReleases();

	struct DeferredRelease
	{
		UINT64 fenceValue{};
		std::function<void()> release;
	};

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
	SpritePipeline m_spritePipeline;

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
	UINT64 m_nextFenceValue{ 1 };
	bool m_frameRecording{};
	FrameUploadBuffer m_dynamicVertexUpload;
	std::vector<DeferredRelease> m_deferredReleases;
};
