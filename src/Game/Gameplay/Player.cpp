#include "Game/Gameplay/Player.h"

#include "Framework/Core/Math/MathUtils.h"
#include "Framework/Models/ModelAssetCache.h"
#include "Framework/Rendering/Core/IRenderDevice.h"
#include "Game/Input/GameActions.h"

#include <cmath>
#include <string>

using namespace DirectX;

namespace
{
	constexpr const char* IdleAnimationName = "Idle";
	constexpr const char* JoggingAnimationName = "Jogging";
	constexpr const char* AttackAnimationName = "Attack";
	constexpr float DefaultAttackDurationSeconds = 1.0f;
}

const SkinnedMeshActorDefinition& Player::GetSkinnedMeshDefinition() const
{
	return GetPlayerDefinition().mesh;
}

void Player::Prepare(ModelAssetCache& assets)
{
	m_playerPrepared = false;
	const PlayerDefinition& definition = GetPlayerDefinition();
	SkinnedMeshActor::Prepare(assets);
	m_moveSpeed = definition.moveSpeed;
	m_groundProbe.SetSettings(definition.grounding);
	m_verticalMotion.SetSettings(definition.verticalMotion);
	m_hasJoggingAnimation = !definition.joggingAnimationPath.empty();
	if (m_hasJoggingAnimation)
	{
		GetModel().PrepareAnimation(assets, JoggingAnimationName, std::string(definition.joggingAnimationPath));
	}

	m_hasAttackAnimation = !definition.attackAnimationPath.empty();
	if (m_hasAttackAnimation)
	{
		GetModel().PrepareAnimation(assets, AttackAnimationName, std::string(definition.attackAnimationPath));
		m_attackDurationSeconds = GetModel().GetAnimationDurationSeconds(AttackAnimationName);
		if (m_attackDurationSeconds <= 0.0f)
		{
			m_attackDurationSeconds = DefaultAttackDurationSeconds;
		}
	}
	m_playerPrepared = true;
}

void Player::Initialize(IRenderDevice& device)
{
	ModelAssetCache assets;
	if (!m_playerPrepared) Prepare(assets);
	SkinnedMeshActor::Initialize(device);
}

void Player::Update(float deltaTime, const Input& input)
{
	m_startedJumpThisFrame = false;

	if (GameActions::WasPressed(input, GameAction::Attack) && m_hasAttackAnimation)
	{
		StartAttack();
	}

	const MovementInput movementInput = ReadMovementInput(input);
	const bool isAttacking = IsAttacking();
	const XMFLOAT3 inputDisplacement = BuildInputDisplacement(movementInput, deltaTime, isAttacking);
	ApplyMovement(deltaTime, GameActions::WasPressed(input, GameAction::Jump), inputDisplacement);
	UpdateAttackTimer(deltaTime, movementInput.hasDirection);
}

XMFLOAT3 Player::GetCollisionPosition() const
{
	return GetPosition();
}

void Player::SetCollisionPosition(const XMFLOAT3& position)
{
	SetPosition(position);
}

CollisionBodyDefinition Player::GetCollisionBodyDefinition() const
{
	const PlayerDefinition& definition = GetPlayerDefinition();
	return CollisionBodyDefinition
	{
		definition.grounding.collisionRadius,
		definition.mesh.height,
		true
	};
}

void Player::SetMovementForward(const XMFLOAT3& forward)
{
	XMFLOAT3 normalized{};
	if (!MathUtils::TryNormalizeXZ(forward, normalized))
	{
		return;
	}

	m_movementForward = normalized;
}

void Player::SetCollisionQuery(const ICollisionQuery* query)
{
	m_groundProbe.SetCollisionQuery(query);
}

void Player::SetGround(const ICollisionSurface* ground)
{
	m_groundProbe.SetGround(ground);
}

void Player::AddLandingSurface(const ICollisionSurface* surface)
{
	m_groundProbe.AddLandingSurface(surface);
}

void Player::ResolveWallCollision()
{
	XMFLOAT3 position = GetPosition();
	if (m_groundProbe.ResolveWallCollision(position))
	{
		SetPosition(position);
	}
}

void Player::StartAttack()
{
	m_attackTimeRemaining = m_attackDurationSeconds;
	SetAnimationState(AnimationState::Attack);
}

void Player::SetAnimationState(AnimationState state)
{
	if (m_animationState == state)
	{
		return;
	}

	m_animationState = state;
	switch (m_animationState)
	{
	case AnimationState::Idle:
		GetModel().PlayAnimation(IdleAnimationName);
		break;
	case AnimationState::Jogging:
		if (m_hasJoggingAnimation)
		{
			GetModel().PlayAnimation(JoggingAnimationName);
		}
		break;
	case AnimationState::Attack:
		if (m_hasAttackAnimation)
		{
			GetModel().PlayAnimation(AttackAnimationName);
		}
		break;
	}
}

Player::MovementInput Player::ReadMovementInput(const Input& input) const
{
	XMFLOAT3 movement{};
	if (GameActions::IsDown(input, GameAction::MoveForward))
	{
		movement.z += 1.0f;
	}
	if (GameActions::IsDown(input, GameAction::MoveBackward))
	{
		movement.z -= 1.0f;
	}
	if (GameActions::IsDown(input, GameAction::MoveLeft))
	{
		movement.x -= 1.0f;
	}
	if (GameActions::IsDown(input, GameAction::MoveRight))
	{
		movement.x += 1.0f;
	}

	MovementInput result{};
	result.hasDirection = MathUtils::TryNormalizeXZ(movement, result.localDirection);
	return result;
}

XMFLOAT3 Player::BuildInputDisplacement(
	const MovementInput& movementInput,
	float deltaTime,
	bool isAttacking)
{
	if (isAttacking)
	{
		return XMFLOAT3{};
	}

	SetAnimationState(GetLocomotionState(movementInput.hasDirection));
	if (!movementInput.hasDirection)
	{
		return XMFLOAT3{};
	}

	const XMFLOAT3 worldMovement = TransformInputToWorld(movementInput.localDirection);
	SetRotationY(std::atan2(worldMovement.x, worldMovement.z) + XM_PI);
	return XMFLOAT3
	{
		worldMovement.x * m_moveSpeed * deltaTime,
		0.0f,
		worldMovement.z * m_moveSpeed * deltaTime
	};
}

void Player::ApplyMovement(
	float deltaTime,
	bool wantsJump,
	const XMFLOAT3& inputDisplacement)
{
	const RootMotionDelta rootMotionDelta = GetModel().Update(deltaTime);
	const XMFLOAT3 displacement = ResolveMovement(inputDisplacement, rootMotionDelta);

	XMFLOAT3 position = GetPosition();
	position.x += displacement.x;
	position.z += displacement.z;
	m_groundProbe.ResolveWallCollision(position);
	const XMFLOAT3 jumpStartPosition = position;
	m_startedJumpThisFrame = m_verticalMotion.Update(
		deltaTime, wantsJump, position, m_groundProbe, displacement.y);
	if (m_startedJumpThisFrame)
	{
		m_lastJumpStartPosition = jumpStartPosition;
	}
	SetPosition(position);
}

void Player::UpdateAttackTimer(float deltaTime, bool hasMovementInput)
{
	if (!IsAttacking())
	{
		return;
	}

	m_attackTimeRemaining -= deltaTime;
	if (m_attackTimeRemaining <= 0.0f)
	{
		SetAnimationState(GetLocomotionState(hasMovementInput));
	}
}

Player::AnimationState Player::GetLocomotionState(bool hasMovementInput) const
{
	return hasMovementInput && m_hasJoggingAnimation
		? AnimationState::Jogging
		: AnimationState::Idle;
}

bool Player::IsAttacking() const
{
	return m_animationState == AnimationState::Attack;
}

void Player::SetGravityEnabled(bool enabled)
{
	m_verticalMotion.SetGravityEnabled(enabled);
}

bool Player::IsGravityEnabled() const
{
	return m_verticalMotion.IsGravityEnabled();
}

bool Player::IsGrounded() const
{
	return m_verticalMotion.IsGrounded();
}

bool Player::DidStartJumpThisFrame() const
{
	return m_startedJumpThisFrame;
}

XMFLOAT3 Player::GetLastJumpStartPosition() const
{
	return m_lastJumpStartPosition;
}

XMFLOAT3 Player::TransformInputToWorld(const XMFLOAT3& movement) const
{
	const XMFLOAT3 right
	{
		m_movementForward.z,
		0.0f,
		-m_movementForward.x
	};

	return XMFLOAT3
	{
		right.x * movement.x + m_movementForward.x * movement.z,
		0.0f,
		right.z * movement.x + m_movementForward.z * movement.z
	};
}
