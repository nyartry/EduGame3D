#pragma once

#include "Rendering/ConstantBufferRing.h"

#include <Windows.h>
#include <wrl/client.h>

#include <d3d12.h>
#include <DirectXMath.h>

class BasicColorPipeline
{
public:
	void Initialize(ID3D12Device* device);
	D3D12_GPU_VIRTUAL_ADDRESS UpdateWorldViewProjection(const DirectX::XMMATRIX& worldViewProjection);
	void Bind(ID3D12GraphicsCommandList* commandList) const;

	ID3D12PipelineState* GetPipelineState() const;

private:
	struct SceneConstants
	{
		DirectX::XMFLOAT4X4 worldViewProjection{};
	};
	static constexpr UINT MaxDrawConstants = 1024;

	void CreateRootSignature(ID3D12Device* device);
	void CreatePipelineState(ID3D12Device* device);
	void CreateConstantBuffers(ID3D12Device* device);

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
	ConstantBufferRing<SceneConstants, MaxDrawConstants> m_sceneConstantBuffer;
	SceneConstants m_constantBufferData{};
};
