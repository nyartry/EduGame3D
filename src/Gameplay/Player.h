#pragma once

#include "Gameplay/CharacterGrounding.h"
#include "Scene/Input.h"
#include "Models/SkinnedMeshActor.h"

#include <DirectXMath.h>
#include <Windows.h>

#include <string_view>

class Ground;
class PrimitiveObject;

struct PlayerDefinition
{
	SkinnedMeshActorDefinition mesh;
	std::string_view joggingAnimationPath;
	std::string_view attackAnimationPath;
	RootMotionMode rootMotionMode{ RootMotionMode::Apply };
	float moveSpeed{ 3.0f };
	CharacterGroundingSettings grounding;
	CharacterVerticalMotionSettings verticalMotion;
};

class Player : public SkinnedMeshActor
{
public:
	void Initialize(ID3D12Device* device) override;
	void Update(float deltaTime, const Input& input) override;
	void SetMovementForward(const DirectX::XMFLOAT3& forward);
	void SetGround(const Ground* ground);
	void AddLandingSurface(const PrimitiveObject* surface);
	void SetRootMotionMode(RootMotionMode mode);
	void SetRootMotionVerticalMode(RootMotionVerticalMode mode);
	void SetRootMotionBlendWeight(float weight);
	void SetGravityEnabled(bool enabled);
	bool IsGravityEnabled() const;

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

	struct MovementInput
	{
		DirectX::XMFLOAT3 localDirection{};
		bool hasDirection{};
	};

	void StartAttack();
	void SetAnimationState(AnimationState state);
	MovementInput ReadMovementInput(const Input& input) const;
	DirectX::XMFLOAT3 BuildInputDisplacement(
		const MovementInput& movementInput,
		float deltaTime,
		bool isAttacking);
	void ApplyMovement(
		float deltaTime,
		bool wantsJump,
		const DirectX::XMFLOAT3& inputDisplacement);
	void UpdateAttackTimer(float deltaTime, bool hasMovementInput);
	AnimationState GetLocomotionState(bool hasMovementInput) const;
	bool IsAttacking() const;
	DirectX::XMFLOAT3 ChooseDisplacement(
		const DirectX::XMFLOAT3& inputDisplacement,
		const DirectX::XMFLOAT3& rootMotionDisplacement) const;
	DirectX::XMFLOAT3 TransformInputToWorld(const DirectX::XMFLOAT3& movement) const;
	DirectX::XMFLOAT3 TransformRootMotionToWorld(const DirectX::XMFLOAT3& localRootMotion) const;

	AnimationState m_animationState{ AnimationState::Idle };
	CharacterGroundProbe m_groundProbe;
	CharacterVerticalMotion m_verticalMotion;
	DirectX::XMFLOAT3 m_movementForward{ 0.0f, 0.0f, 1.0f };
	RootMotionMode m_rootMotionMode{ RootMotionMode::Apply };
	RootMotionVerticalMode m_rootMotionVerticalMode{ RootMotionVerticalMode::Apply };
	float m_rootMotionBlendWeight{ 0.5f };
	float m_moveSpeed{ 3.0f };
	float m_attackTimeRemaining{};
	float m_attackDurationSeconds{};
	bool m_hasJoggingAnimation{};
	bool m_hasAttackAnimation{};
};
