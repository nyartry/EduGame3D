#pragma once

#include "Framework/Rendering/Core/ConstantBufferRing.h"

#include <Windows.h>
#include <wrl/client.h>

#include <d3d12.h>
#include <DirectXMath.h>

class SpritePipeline
{
public:
	void Initialize(ID3D12Device* device);
	void BeginFrame(UINT64 completedFence) { m_constantBuffer.BeginFrame(completedFence); }
	void EndFrame(UINT64 submittedFence) { m_constantBuffer.EndFrame(submittedFence); }
	D3D12_GPU_VIRTUAL_ADDRESS UpdateScreenSize(UINT width, UINT height);
	void Bind(ID3D12GraphicsCommandList* commandList) const;

	ID3D12PipelineState* GetPipelineState() const;

private:
	struct SpriteConstants
	{
		DirectX::XMFLOAT4 screenSize{}; // width, height, 1 / width, 1 / height
	};
	static constexpr UINT MaxDrawConstants = 1024;

	void CreateRootSignature(ID3D12Device* device);
	void CreatePipelineState(ID3D12Device* device);
	void CreateConstantBuffers(ID3D12Device* device);

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
	ConstantBufferRing<SpriteConstants, MaxDrawConstants> m_constantBuffer;
	SpriteConstants m_constantBufferData{};
};
