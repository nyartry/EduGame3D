#include "Gameplay/Player.h"

#include "Rendering/Dx12Renderer.h"

#include <algorithm>
#include <cmath>
#include <string>

using namespace DirectX;

namespace
{
	constexpr const char* IdleAnimationName = "Idle";
	constexpr const char* JoggingAnimationName = "Jogging";
	constexpr const char* AttackAnimationName = "Attack";
	constexpr float MoveSpeed = 3.0f;
	constexpr float DefaultAttackDurationSeconds = 1.0f;

	XMFLOAT3 LerpFloat3(const XMFLOAT3& from, const XMFLOAT3& to, float amount)
	{
		return XMFLOAT3
		{
			from.x * (1.0f - amount) + to.x * amount,
			from.y * (1.0f - amount) + to.y * amount,
			from.z * (1.0f - amount) + to.z * amount
		};
	}
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
	if (input.WasPressed(InputKey::X) && m_hasAttackAnimation)
	{
		StartAttack();
	}

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

	const float length = std::sqrt(movement.x * movement.x + movement.z * movement.z);
	XMFLOAT3 inputDisplacement{};
	const bool isAttacking = m_animationState == AnimationState::Attack;
	if (!isAttacking && length > 0.0f && m_hasJoggingAnimation)
	{
		movement.x /= length;
		movement.z /= length;
		const XMFLOAT3 worldMovement = TransformInputToWorld(movement);
		inputDisplacement.x = worldMovement.x * MoveSpeed * deltaTime;
		inputDisplacement.z = worldMovement.z * MoveSpeed * deltaTime;

		SetRotationY(std::atan2(worldMovement.x, worldMovement.z) + XM_PI);
		SetAnimationState(AnimationState::Jogging);
	}
	else if (!isAttacking)
	{
		SetAnimationState(AnimationState::Idle);
		if (length > 0.0f)
		{
			movement.x /= length;
			movement.z /= length;
			const XMFLOAT3 worldMovement = TransformInputToWorld(movement);
			inputDisplacement.x = worldMovement.x * MoveSpeed * deltaTime;
			inputDisplacement.z = worldMovement.z * MoveSpeed * deltaTime;
			SetRotationY(std::atan2(worldMovement.x, worldMovement.z) + XM_PI);
		}
	}

	const RootMotionDelta rootMotionDelta = GetModel().Update(deltaTime);
	const XMFLOAT3 rootMotionDisplacement = TransformRootMotionToWorld(rootMotionDelta.translation);
	const XMFLOAT3 displacement = ChooseDisplacement(inputDisplacement, rootMotionDisplacement);
	XMFLOAT3 position = GetPosition();
	position.x += displacement.x;
	position.z += displacement.z;
	m_verticalMotion.Update(deltaTime, input.WasPressed(InputKey::Space), position, m_groundProbe);

	SetPosition(position);

	if (isAttacking)
	{
		m_attackTimeRemaining -= deltaTime;
		if (m_attackTimeRemaining <= 0.0f)
		{
			SetAnimationState(length > 0.0f && m_hasJoggingAnimation ? AnimationState::Jogging : AnimationState::Idle);
		}
	}
}

void Player::SetMovementForward(const XMFLOAT3& forward)
{
	XMFLOAT3 normalized{ forward.x, 0.0f, forward.z };
	const float length = std::sqrt(normalized.x * normalized.x + normalized.z * normalized.z);
	if (length == 0.0f)
	{
		return;
	}

	normalized.x /= length;
	normalized.z /= length;
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

XMFLOAT3 Player::ChooseDisplacement(
	const XMFLOAT3& inputDisplacement,
	const XMFLOAT3& rootMotionDisplacement) const
{
	switch (m_rootMotionMode)
	{
	case RootMotionMode::Apply:
		return rootMotionDisplacement;
	case RootMotionMode::Blend:
		return LerpFloat3(inputDisplacement, rootMotionDisplacement, m_rootMotionBlendWeight);
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

