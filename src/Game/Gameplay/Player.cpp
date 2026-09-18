#include "Game/Gameplay/Player.h"

#include "Framework/Core/Math/MathUtils.h"
#include "Framework/Models/ModelAssetCache.h"
#include "Framework/Rendering/Core/IRenderDevice.h"
#include "Game/Input/GameActions.h"

#include <cmath>
#include <stdexcept>
#include <string>

using namespace DirectX;

namespace
{
	constexpr const char* IdleAnimationName = "Idle";
	constexpr const char* JoggingAnimationName = "Jogging";
	constexpr const char* AttackAnimationName = "Attack";
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
	m_animationState = AnimationState::Idle;
	m_comboWindowOpen = false;
	m_attackQueued = false;
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
		const float attackDuration = GetModel().GetAnimationDurationSeconds(AttackAnimationName);
		if (!std::isfinite(attackDuration) || attackDuration <= 0.0f)
		{
			throw std::runtime_error("Player attack animation must have a finite, positive duration.");
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

	const MovementInput movementInput = ReadMovementInput(input);
	UpdateAttackState(GameActions::WasPressed(input, GameAction::Attack) && m_hasAttackAnimation,
		movementInput.hasDirection);
	const bool isAttacking = IsAttacking();
	const XMFLOAT3 inputDisplacement = BuildInputDisplacement(movementInput, deltaTime, isAttacking);
	ApplyMovement(deltaTime, GameActions::WasPressed(input, GameAction::Jump), inputDisplacement);
	UpdateComboWindow();
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
	SetAnimationState(AnimationState::Attack, { .mode = AnimationPlaybackMode::Once, .restart = true });
}

void Player::SetAnimationState(AnimationState state, const AnimationPlayOptions& options)
{
	if (m_animationState == state && !options.restart)
	{
		return;
	}

	const char* animationName = nullptr;
	switch (state)
	{
	case AnimationState::Idle:
		animationName = IdleAnimationName;
		break;
	case AnimationState::Jogging:
		if (m_hasJoggingAnimation)
		{
			animationName = JoggingAnimationName;
		}
		break;
	case AnimationState::Attack:
		if (m_hasAttackAnimation)
		{
			animationName = AttackAnimationName;
		}
		break;
	}
	const bool startedPlayback = animationName != nullptr && GetModel().PlayAnimation(animationName, options);
	if (state == AnimationState::Attack && !startedPlayback)
	{
		return;
	}
	// Missing optional locomotion clips must not keep gameplay locked in Attack.
	m_animationState = state;
	m_comboWindowOpen = false;
	m_attackQueued = false;
}

void Player::UpdateAttackState(bool wantsAttack, bool hasMovementInput)
{
	if (!IsAttacking())
	{
		if (wantsAttack) StartAttack();
		return;
	}

	// Transition on the next update so the scene can consume the completed
	// attack's terminal events before a clip switch clears them.
	if (GetModel().IsAnimationFinished())
	{
		if (m_attackQueued || wantsAttack) StartAttack();
		else SetAnimationState(GetLocomotionState(hasMovementInput));
		return;
	}

	if (wantsAttack && m_comboWindowOpen)
	{
		m_attackQueued = true;
	}
}

void Player::UpdateComboWindow()
{
	if (!IsAttacking()) return;
	for (const auto& occurrence : GetModel().GetAnimationEvents())
	{
		if (occurrence.event.type == "ComboWindowOpen") m_comboWindowOpen = true;
		else if (occurrence.event.type == "ComboWindowClose") m_comboWindowOpen = false;
	}
	if (GetModel().IsAnimationFinished()) m_comboWindowOpen = false;
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
