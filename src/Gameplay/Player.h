#pragma once

#include "Scene/Input.h"
#include "Models/SkinnedMeshActor.h"

#include <DirectXMath.h>
#include <Windows.h>

#include <string_view>

class Ground;

struct PlayerDefinition
{
	SkinnedMeshActorDefinition mesh;
	std::string_view joggingAnimationPath;
	std::string_view attackAnimationPath;
	RootMotionMode rootMotionMode{ RootMotionMode::Apply };
};

class Player : public SkinnedMeshActor
{
public:
	void Initialize(ID3D12Device* device) override;
	void Update(float deltaTime, const Input& input) override;
	void SetMovementForward(const DirectX::XMFLOAT3& forward);
	void SetGround(const Ground* ground);
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
		Attack,
	};

	void StartAttack();
	void SetAnimationState(AnimationState state);
	DirectX::XMFLOAT3 ChooseDisplacement(
		const DirectX::XMFLOAT3& inputDisplacement,
		const DirectX::XMFLOAT3& rootMotionDisplacement) const;
	void ApplyVerticalPhysics(float deltaTime, const Input& input, DirectX::XMFLOAT3& position);
	DirectX::XMFLOAT3 TransformInputToWorld(const DirectX::XMFLOAT3& movement) const;
	DirectX::XMFLOAT3 TransformRootMotionToWorld(const DirectX::XMFLOAT3& localRootMotion) const;

	AnimationState m_animationState{ AnimationState::Idle };
	const Ground* m_ground{};
	DirectX::XMFLOAT3 m_movementForward{ 0.0f, 0.0f, 1.0f };
	RootMotionMode m_rootMotionMode{ RootMotionMode::Apply };
	RootMotionVerticalMode m_rootMotionVerticalMode{ RootMotionVerticalMode::Apply };
	float m_rootMotionBlendWeight{ 0.5f };
	float m_verticalVelocity{};
	float m_attackTimeRemaining{};
	float m_attackDurationSeconds{};
	bool m_isGrounded{};
	bool m_hasJoggingAnimation{};
	bool m_hasAttackAnimation{};
};
