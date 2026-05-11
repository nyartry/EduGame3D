#pragma once

#include "Input.h"
#include "SkinnedMeshActor.h"

#include <DirectXMath.h>
#include <Windows.h>

#include <string_view>

struct PlayerDefinition
{
	SkinnedMeshActorDefinition mesh;
	std::string_view joggingAnimationPath;
	RootMotionMode rootMotionMode{ RootMotionMode::Apply };
};

class Player : public SkinnedMeshActor
{
public:
	void Initialize(ID3D12Device* device) override;
	void Update(float deltaTime, const Input& input) override;
	void SetRootMotionMode(RootMotionMode mode);
	void SetRootMotionVerticalMode(RootMotionVerticalMode mode);
	void SetRootMotionBlendWeight(float weight);

protected:
	const SkinnedMeshActorDefinition& GetSkinnedMeshDefinition() const final;
	virtual const PlayerDefinition& GetPlayerDefinition() const = 0;

private:
	enum class AnimationState
	{
		Idle,
		Jogging,
	};

	void SetAnimationState(AnimationState state);
	DirectX::XMFLOAT3 ChooseDisplacement(
		const DirectX::XMFLOAT3& inputDisplacement,
		const DirectX::XMFLOAT3& rootMotionDisplacement) const;
	DirectX::XMFLOAT3 TransformRootMotionToWorld(const DirectX::XMFLOAT3& localRootMotion) const;

	AnimationState m_animationState{ AnimationState::Idle };
	RootMotionMode m_rootMotionMode{ RootMotionMode::Apply };
	RootMotionVerticalMode m_rootMotionVerticalMode{ RootMotionVerticalMode::Apply };
	float m_rootMotionBlendWeight{ 0.5f };
	bool m_hasJoggingAnimation{};
};
