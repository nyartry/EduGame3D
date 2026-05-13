#pragma once

#include "Animation/ISkinnedMeshProcessor.h"
#include "Animation/RootMotion.h"
#include "Animation/SkinningMode.h"
#include "Models/SkinnedModelData.h"
#include "Rendering/TexturedMaterial.h"
#include "Common/ModelScaleSettings.h"

#include <DirectXMath.h>
#include <Windows.h>

#include <memory>
#include <string>
#include <vector>

class Dx12Renderer;
struct ID3D12Device;

class SkinnedModel
{
public:
	void Initialize(
		ID3D12Device* device,
		const std::string& modelPath,
		const ModelScaleSettings& scaleSettings = ModelScaleSettings::OriginalSize(),
		SkinningMode skinningMode = SkinningMode::Cpu);
	void AddAnimation(const std::string& animationName, const std::string& animationPath);
	void PlayAnimation(const std::string& animationName);
	RootMotionDelta Update(float deltaTime);
	void Draw(Dx12Renderer& renderer) const;

	void SetPosition(float x, float y, float z);
	void SetRotationY(float radians);

private:
	static std::unique_ptr<ISkinnedMeshProcessor> CreateMeshProcessor(SkinningMode skinningMode);
	void FitModel(const ModelScaleSettings& scaleSettings);
	RootMotionDelta ExtractRootMotionDelta(float deltaTime) const;
	void UpdateBoneMatrices();

	DirectX::XMMATRIX GetLocalTransform(const BoneData& bone) const;

	SkinnedModelData m_modelData;
	std::vector<std::unique_ptr<ISkinnedMeshProcessor>> m_meshProcessors;
	std::vector<DirectX::XMFLOAT4X4> m_boneMatrices;
	DirectX::XMFLOAT3 m_position{ 0.0f, 0.0f, 0.0f };
	float m_animationTimeSeconds{};
	size_t m_currentAnimationIndex{};
	float m_modelScale{ 1.0f };
	float m_modelCenterX{};
	float m_modelMinY{};
	float m_modelCenterZ{};
	float m_rotationY{};
};
