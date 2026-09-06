#pragma once

#include "Framework/Animation/RootMotion.h"
#include "Framework/Gameplay/Actor.h"
#include "Framework/Models/SkinnedModel.h"

#include <DirectXMath.h>

#include <string_view>

struct SkinnedMeshActorDefinition
{
	std::string_view modelPath;
	std::string_view idleAnimationPath;
	float height{ 1.8f };
	DirectX::XMFLOAT3 initialPosition{ 0.0f, 0.0f, 0.0f };
	float initialRotationY{};
	SkinningMode skinningMode{ SkinningMode::Gpu };
	RootMotionSettings rootMotion;
};

class SkinnedMeshActor : public Actor
{
public:
	void Initialize(IRenderDevice& device) override;
	void Update(float deltaTime, const Input& input) override;
	void Draw(IRenderer& renderer) const override;

	const DirectX::XMFLOAT3& GetPosition() const;
	float GetRotationY() const;
	void SetRootMotionSettings(const RootMotionSettings& settings);
	const RootMotionSettings& GetRootMotionSettings() const;
	void SetRootMotionMode(RootMotionMode mode);
	void SetRootMotionVerticalMode(RootMotionVerticalMode mode);
	void SetRootMotionBlendWeight(float weight);

protected:
	virtual const SkinnedMeshActorDefinition& GetSkinnedMeshDefinition() const = 0;

	SkinnedModel& GetModel();
	const SkinnedModel& GetModel() const;
	void SetPosition(const DirectX::XMFLOAT3& position);
	void SetRotationY(float radians);
	DirectX::XMFLOAT3 ResolveMovement(
		const DirectX::XMFLOAT3& programDisplacement,
		const RootMotionDelta& rootMotion) const;

private:
	SkinnedModel m_model;
	DirectX::XMFLOAT3 m_position{ 0.0f, 0.0f, 0.0f };
	float m_rotationY{};
	RootMotionSettings m_rootMotion;
	bool m_hasIdleAnimation{};
};
