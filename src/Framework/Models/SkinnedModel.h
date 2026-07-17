#pragma once

#include "Framework/Animation/ISkinnedMeshProcessor.h"
#include "Framework/Animation/RootMotion.h"
#include "Framework/Animation/SkinningMode.h"
#include "Framework/Models/SkinnedModelData.h"
#include "Framework/Rendering/Materials/TexturedMaterial.h"
#include "Framework/Common/ModelScaleSettings.h"

#include <DirectXMath.h>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

class IRenderDevice;
class IRenderer;

class SkinnedModel
{
public:
	void Initialize(
		IRenderDevice& device,
		const std::string& modelPath,
		const ModelScaleSettings& scaleSettings = ModelScaleSettings::OriginalSize(),
		SkinningMode skinningMode = SkinningMode::Cpu);
	void AddAnimation(const std::string& animationName, const std::string& animationPath);
	void PlayAnimation(const std::string& animationName);
	void PlayAnimationByIndex(size_t animationIndex);
	float GetAnimationDurationSeconds(const std::string& animationName) const;
	float GetCurrentAnimationDurationSeconds() const;
	float GetAnimationTimeSeconds() const;
	size_t GetCurrentAnimationIndex() const;
	const SkinnedModelData& GetModelData() const;
	void SetAnimationTimeSeconds(float animationTimeSeconds);
	RootMotionDelta Update(float deltaTime);
	void Draw(IRenderer& renderer) const;

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
