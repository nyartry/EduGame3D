#pragma once

#include <Windows.h>
#include <wrl/client.h>

#include <array>
#include <d3d12.h>
#include <DirectXMath.h>
#include <vector>

class SkinnedTexturedPipeline
{
public:
	static constexpr size_t MaxBones = 128;

	void Initialize(ID3D12Device* device);
	void UpdateConstants(
		const DirectX::XMMATRIX& worldViewProjection,
		const std::vector<DirectX::XMFLOAT4X4>& boneMatrices,
		float modelCenterX,
		float modelMinY,
		float modelCenterZ,
		float modelScale);
	void Bind(ID3D12GraphicsCommandList* commandList) const;

	ID3D12PipelineState* GetPipelineState() const;

private:
	struct SceneConstants
	{
		DirectX::XMFLOAT4X4 worldViewProjection{};
		DirectX::XMFLOAT4 modelFit{ 0.0f, 0.0f, 0.0f, 1.0f };
	};

	struct BoneConstants
	{
		std::array<DirectX::XMFLOAT4X4, MaxBones> boneMatrices{};
	};

	void CreateRootSignature(ID3D12Device* device);
	void CreatePipelineState(ID3D12Device* device);
	void CreateConstantBuffers(ID3D12Device* device);

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_sceneConstantBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_boneConstantBuffer;
	SceneConstants m_sceneConstants{};
	BoneConstants m_boneConstants{};
	UINT8* m_sceneConstantBufferMappedData{};
	UINT8* m_boneConstantBufferMappedData{};
};
