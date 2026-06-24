#pragma once

#include "Animation/ISkinnedMeshProcessor.h"
#include "Animation/RootMotion.h"
#include "Animation/SkinningMode.h"
#include "Models/SkinnedModelData.h"
#include "Rendering/Materials/TexturedMaterial.h"
#include "Common/ModelScaleSettings.h"

#include <DirectXMath.h>
#include <Windows.h>

#include <memory>
#include <string>
#include <string_view>
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
	float GetAnimationDurationSeconds(const std::string& animationName) const;
	RootMotionDelta Update(float deltaTime);
	void Draw(Dx12Renderer& renderer) const;

	void SetPosition(float x, float y, float z);
	void SetRotationY(float radians);
	DirectX::XMFLOAT3 GetAnimatedBoundsCenterLocal() const;
	bool TryGetBonePositionLocal(std::string_view boneName, DirectX::XMFLOAT3& position) const;
	bool TryGetRootMotionBonePositionLocal(DirectX::XMFLOAT3& position) const;

private:
	static std::unique_ptr<ISkinnedMeshProcessor> CreateMeshProcessor(SkinningMode skinningMode);
	void FitModel(const ModelScaleSettings& scaleSettings);
	RootMotionDelta ExtractRootMotionDelta(float deltaTime) const;
	void UpdateBoneMatrices();
	void UpdateAnimatedBounds() const;
	bool TryGetBonePositionLocal(int boneIndex, DirectX::XMFLOAT3& position) const;

	DirectX::XMMATRIX GetLocalTransform(const BoneData& bone) const;

	SkinnedModelData m_modelData;
	std::vector<std::unique_ptr<ISkinnedMeshProcessor>> m_meshProcessors;
	std::vector<DirectX::XMFLOAT4X4> m_boneMatrices;
	std::vector<DirectX::XMFLOAT4X4> m_boneModelMatrices;
	DirectX::XMFLOAT3 m_position{ 0.0f, 0.0f, 0.0f };
	float m_animationTimeSeconds{};
	size_t m_currentAnimationIndex{};
	float m_modelScale{ 1.0f };
	float m_modelCenterX{};
	float m_modelMinY{};
	float m_modelCenterZ{};
	float m_rotationY{};
	mutable DirectX::XMFLOAT3 m_animatedBoundsCenterLocal{};
	mutable bool m_animatedBoundsDirty{ true };
};
