#include "Gameplay/Player.h"

#include "Common/MathUtils.h"
#include "Rendering/Core/Dx12Renderer.h"

#include <algorithm>
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

void Player::Initialize(ID3D12Device* device)
{
	const PlayerDefinition& definition = GetPlayerDefinition();
	SkinnedMeshActor::Initialize(device);
	m_rootMotionMode = definition.rootMotionMode;
	m_moveSpeed = definition.moveSpeed;
	m_groundProbe.SetSettings(definition.grounding);
	m_verticalMotion.SetSettings(definition.verticalMotion);
	m_hasJoggingAnimation = !definition.joggingAnimationPath.empty();
	if (m_hasJoggingAnimation)
	{
		GetModel().AddAnimation(JoggingAnimationName, std::string(definition.joggingAnimationPath));
	}

	m_hasAttackAnimation = !definition.attackAnimationPath.empty();
	if (m_hasAttackAnimation)
	{
		GetModel().AddAnimation(AttackAnimationName, std::string(definition.attackAnimationPath));
		m_attackDurationSeconds = GetModel().GetAnimationDurationSeconds(AttackAnimationName);
		if (m_attackDurationSeconds <= 0.0f)
		{
			m_attackDurationSeconds = DefaultAttackDurationSeconds;
		}
	}
}

void Player::Update(float deltaTime, const Input& input)
{
	m_startedJumpThisFrame = false;

	if (input.WasPressed(InputKey::X) && m_hasAttackAnimation)
	{
		StartAttack();
	}

	const MovementInput movementInput = ReadMovementInput(input);
	const bool isAttacking = IsAttacking();
	const XMFLOAT3 inputDisplacement = BuildInputDisplacement(movementInput, deltaTime, isAttacking);
	ApplyMovement(deltaTime, input.WasPressed(InputKey::Space), inputDisplacement);
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

void Player::SetGround(const Ground* ground)
{
	m_groundProbe.SetGround(ground);
}

void Player::AddLandingSurface(const PrimitiveObject* surface)
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
	if (input.IsDown(InputKey::W))
	{
		movement.z += 1.0f;
	}
	if (input.IsDown(InputKey::S))
	{
		movement.z -= 1.0f;
	}
	if (input.IsDown(InputKey::A))
	{
		movement.x -= 1.0f;
	}
	if (input.IsDown(InputKey::D))
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
	const XMFLOAT3 rootMotionDisplacement = TransformRootMotionToWorld(rootMotionDelta.translation);
	const XMFLOAT3 displacement = ChooseDisplacement(inputDisplacement, rootMotionDisplacement);

	XMFLOAT3 position = GetPosition();
	position.x += displacement.x;
	position.z += displacement.z;
	m_groundProbe.ResolveWallCollision(position);
	const XMFLOAT3 jumpStartPosition = position;
	const bool wasGrounded = m_verticalMotion.IsGrounded();
	m_verticalMotion.Update(deltaTime, wantsJump, position, m_groundProbe);
	m_startedJumpThisFrame = wantsJump && wasGrounded && !m_verticalMotion.IsGrounded();
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

void Player::SetRootMotionMode(RootMotionMode mode)
{
	m_rootMotionMode = mode;
}

void Player::SetRootMotionVerticalMode(RootMotionVerticalMode mode)
{
	m_rootMotionVerticalMode = mode;
}

void Player::SetRootMotionBlendWeight(float weight)
{
	m_rootMotionBlendWeight = std::clamp(weight, 0.0f, 1.0f);
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

XMFLOAT3 Player::ChooseDisplacement(
	const XMFLOAT3& inputDisplacement,
	const XMFLOAT3& rootMotionDisplacement) const
{
	switch (m_rootMotionMode)
	{
	case RootMotionMode::Apply:
		return rootMotionDisplacement;
	case RootMotionMode::Blend:
		return MathUtils::Lerp(inputDisplacement, rootMotionDisplacement, m_rootMotionBlendWeight);
	case RootMotionMode::Ignore:
	default:
		return inputDisplacement;
	}
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

XMFLOAT3 Player::TransformRootMotionToWorld(const XMFLOAT3& localRootMotion) const
{
	const XMVECTOR local = XMLoadFloat3(&localRootMotion);
	const XMVECTOR world = XMVector3TransformNormal(local, XMMatrixRotationY(GetRotationY()));

	XMFLOAT3 result{};
	XMStoreFloat3(&result, world);
	if (m_rootMotionVerticalMode == RootMotionVerticalMode::Ignore)
	{
		result.y = 0.0f;
	}
	return result;
}

