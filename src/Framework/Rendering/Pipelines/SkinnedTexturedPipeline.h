#pragma once

#include "Framework/Rendering/Core/ConstantBufferRing.h"
#include "Framework/Animation/BoneSkinning.h"

#include <Windows.h>
#include <wrl/client.h>

#include <array>
#include <d3d12.h>
#include <DirectXMath.h>
#include <memory>
#include <vector>

class SkinnedTexturedPipeline
{
public:
	static constexpr size_t MaxBones = BoneSkinning::MaxBones;
	static constexpr UINT MaxDrawConstants = 1024;

	void Initialize(ID3D12Device* device);
	void BeginFrame(UINT64 completedFence)
	{
		m_sceneConstantBuffer.BeginFrame(completedFence);
		m_boneConstantBuffer.BeginFrame(completedFence);
	}
	void EndFrame(UINT64 submittedFence)
	{
		m_sceneConstantBuffer.EndFrame(submittedFence);
		m_boneConstantBuffer.EndFrame(submittedFence);
	}
	struct ConstantBufferViews
	{
		D3D12_GPU_VIRTUAL_ADDRESS sceneConstants{};
		D3D12_GPU_VIRTUAL_ADDRESS boneConstants{};
	};

	ConstantBufferViews UpdateConstants(
		const DirectX::XMMATRIX& worldViewProjection,
		const DirectX::XMMATRIX& world,
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
		DirectX::XMFLOAT4X4 world{};
		DirectX::XMFLOAT4X4 normalWorld{};
		DirectX::XMFLOAT4 modelFit{ 0.0f, 0.0f, 0.0f, 1.0f };
		UINT boneCount{};
		DirectX::XMFLOAT3 padding{};
	};

	struct BoneConstants
	{
		std::array<DirectX::XMFLOAT4X4, MaxBones> boneMatrices{};
		std::array<DirectX::XMFLOAT4X4, MaxBones> normalBoneMatrices{};
	};

	void CreateRootSignature(ID3D12Device* device);
	void CreatePipelineState(ID3D12Device* device);
	void CreateConstantBuffers(ID3D12Device* device);

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
	ConstantBufferRing<SceneConstants, MaxDrawConstants> m_sceneConstantBuffer;
	// Position + normal palettes occupy the 64 KiB CBV limit. Use 1 MiB pages.
	ConstantBufferRing<BoneConstants, 16> m_boneConstantBuffer;
	SceneConstants m_sceneConstants{};
	std::unique_ptr<BoneConstants> m_boneConstants = std::make_unique<BoneConstants>();
};
