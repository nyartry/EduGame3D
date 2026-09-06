#pragma once

#include "Game/Gameplay/CharacterGrounding.h"
#include "Framework/Gameplay/CollisionBody.h"
#include "Framework/Scene/Input/Input.h"
#include "Framework/Models/SkinnedMeshActor.h"

#include <DirectXMath.h>
#include <string_view>

class Ground;
class PrimitiveObject;

struct PlayerDefinition
{
	SkinnedMeshActorDefinition mesh;
	std::string_view joggingAnimationPath;
	std::string_view attackAnimationPath;
	float moveSpeed{ 3.0f };
	CharacterGroundingSettings grounding;
	CharacterVerticalMotionSettings verticalMotion;
};

class Player : public SkinnedMeshActor, public CollisionBody
{
public:
	void Initialize(IRenderDevice& device) override;
	void Update(float deltaTime, const Input& input) override;
	DirectX::XMFLOAT3 GetCollisionPosition() const override;
	void SetCollisionPosition(const DirectX::XMFLOAT3& position) override;
	CollisionBodyDefinition GetCollisionBodyDefinition() const override;
	void SetMovementForward(const DirectX::XMFLOAT3& forward);
	void SetGround(const Ground* ground);
	void AddLandingSurface(const PrimitiveObject* surface);
	void ResolveWallCollision();
	void SetGravityEnabled(bool enabled);
	bool IsGravityEnabled() const;
	bool IsGrounded() const;
	bool DidStartJumpThisFrame() const;
	DirectX::XMFLOAT3 GetLastJumpStartPosition() const;

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
	DirectX::XMFLOAT3 TransformInputToWorld(const DirectX::XMFLOAT3& movement) const;

	AnimationState m_animationState{ AnimationState::Idle };
	CharacterGroundProbe m_groundProbe;
	CharacterVerticalMotion m_verticalMotion;
	DirectX::XMFLOAT3 m_movementForward{ 0.0f, 0.0f, 1.0f };
	float m_moveSpeed{ 3.0f };
	float m_attackTimeRemaining{};
	float m_attackDurationSeconds{};
	bool m_hasJoggingAnimation{};
	bool m_hasAttackAnimation{};
	bool m_startedJumpThisFrame{};
	DirectX::XMFLOAT3 m_lastJumpStartPosition{};
};
