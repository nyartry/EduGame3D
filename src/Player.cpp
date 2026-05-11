#include "Player.h"

#include "Dx12Renderer.h"

#include <algorithm>
#include <cmath>
#include <string>

using namespace DirectX;

namespace
{
	constexpr const char* IdleAnimationName = "Idle";
	constexpr const char* JoggingAnimationName = "Jogging";
	constexpr float MoveSpeed = 3.0f;
	constexpr float GroundHeight = 0.0f;

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

void Player::Update(float deltaTime, const Input& input)
{
	const PlayerDefinition& definition = GetPlayerDefinition();
	m_rootMotionMode = definition.rootMotionMode;
	m_hasJoggingAnimation = !definition.joggingAnimationPath.empty();

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
	if (length > 0.0f && m_hasJoggingAnimation)
	{
		movement.x /= length;
		movement.z /= length;
		inputDisplacement.x = movement.x * MoveSpeed * deltaTime;
		inputDisplacement.z = movement.z * MoveSpeed * deltaTime;

		SetRotationY(std::atan2(movement.x, movement.z) + XM_PI);
		SetAnimationState(AnimationState::Jogging);
	}
	else
	{
		SetAnimationState(AnimationState::Idle);
		if (length > 0.0f)
		{
			movement.x /= length;
			movement.z /= length;
			inputDisplacement.x = movement.x * MoveSpeed * deltaTime;
			inputDisplacement.z = movement.z * MoveSpeed * deltaTime;
			SetRotationY(std::atan2(movement.x, movement.z) + XM_PI);
		}
	}

	const RootMotionDelta rootMotionDelta = GetModel().Update(deltaTime);
	const XMFLOAT3 rootMotionDisplacement = TransformRootMotionToWorld(rootMotionDelta.translation);
	const XMFLOAT3 displacement = ChooseDisplacement(inputDisplacement, rootMotionDisplacement);
	XMFLOAT3 position = GetPosition();
	position.x += displacement.x;
	if (m_rootMotionVerticalMode == RootMotionVerticalMode::Apply)
	{
		position.y += displacement.y;
	}
	else
	{
		position.y = GroundHeight;
	}
	position.z += displacement.z;

	SetPosition(position);
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
			if (!m_joggingAnimationLoaded)
			{
				GetModel().AddAnimation(JoggingAnimationName, std::string(GetPlayerDefinition().joggingAnimationPath));
				m_joggingAnimationLoaded = true;
			}
			GetModel().PlayAnimation(JoggingAnimationName);
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

