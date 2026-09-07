#pragma once

#include "Framework/Animation/ISkinnedMeshProcessor.h"
#include "Framework/Animation/AnimationPlayback.h"
#include "Framework/Animation/AnimationEvents.h"
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
class ModelAssetCache;

class SkinnedModel
{
public:
	void Prepare(ModelAssetCache& assets, const std::string& modelPath, const ModelScaleSettings& scaleSettings);
	void PrepareAnimation(ModelAssetCache& assets, const std::string& animationName, const std::string& animationPath);
	void Activate(IRenderDevice& device, SkinningMode skinningMode);
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
	// Drains the latest simulation update's events once, before the next Update.
	// Seek and clip switches clear pending events.
	std::vector<AnimationEvents::Occurrence> ConsumeAnimationEvents();
	// Animation is model-local; world placement belongs to the actor/editor.
	void Draw(IRenderer& renderer, const DirectX::XMMATRIX& world) const;

	DirectX::XMFLOAT3 GetAnimatedBoundsCenterLocal() const;
	bool TryGetBonePositionLocal(std::string_view boneName, DirectX::XMFLOAT3& position) const;
	bool TryGetRootMotionBonePositionLocal(DirectX::XMFLOAT3& position) const;

private:
	static std::unique_ptr<ISkinnedMeshProcessor> CreateMeshProcessor(SkinningMode skinningMode);
	void FitModel(const ModelScaleSettings& scaleSettings);
	void LoadAnimationEventSidecar(const std::string& modelPath, std::string_view animationAlias = {});
	void UpdateBoneMatrices();
	void UpdateAnimatedBounds() const;
	bool TryGetBonePositionLocal(int boneIndex, DirectX::XMFLOAT3& position) const;

	DirectX::XMMATRIX GetLocalTransform(const BoneData& bone) const;

	SkinnedModelData m_modelData;
	std::string m_preparedModelPath;
	std::vector<std::unique_ptr<ISkinnedMeshProcessor>> m_meshProcessors;
	std::vector<DirectX::XMFLOAT4X4> m_boneMatrices;
	std::vector<DirectX::XMFLOAT4X4> m_boneModelMatrices;
	AnimationPlayback m_playback;
	std::vector<AnimationEvents::Event> m_animationEvents;
	std::vector<AnimationEvents::Occurrence> m_pendingAnimationEvents;
	size_t m_currentAnimationIndex{};
	float m_modelScale{ 1.0f };
	float m_modelCenterX{};
	float m_modelMinY{};
	float m_modelCenterZ{};
	mutable DirectX::XMFLOAT3 m_animatedBoundsCenterLocal{};
	mutable bool m_animatedBoundsDirty{ true };
};
