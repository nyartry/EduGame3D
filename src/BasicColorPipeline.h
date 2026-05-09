#pragma once

#include <Windows.h>
#include <wrl/client.h>

#include <d3d12.h>
#include <DirectXMath.h>

class BasicColorPipeline
{
public:
	void Initialize(ID3D12Device* device);
	void UpdateWorldViewProjection(const DirectX::XMMATRIX& worldViewProjection);
	void Bind(ID3D12GraphicsCommandList* commandList) const;

	ID3D12PipelineState* GetPipelineState() const;

private:
	struct SceneConstants
	{
		DirectX::XMFLOAT4X4 worldViewProjection{};
	};

	void CreateRootSignature(ID3D12Device* device);
	void CreatePipelineState(ID3D12Device* device);
	void CreateConstantBuffer(ID3D12Device* device);

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_constantBuffer;
	SceneConstants m_constantBufferData{};
	UINT8* m_constantBufferMappedData{};
};
