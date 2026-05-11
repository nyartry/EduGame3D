#include "Player.h"

#include "Dx12Renderer.h"

#include <algorithm>
#include <cmath>

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

void Player::Initialize(ID3D12Device* device)
{
	const PlayerModelDefinition& definition = GetModelDefinition();
	m_position = definition.initialPosition;
	m_rotationY = definition.initialRotationY;
	m_rootMotionMode = definition.rootMotionMode;
	m_hasIdleAnimation = !definition.idleAnimationPath.empty();
	m_hasJoggingAnimation = !definition.joggingAnimationPath.empty();

	m_model.Initialize(
		device,
		std::string(definition.modelPath),
		ModelScaleSettings::NormalizeToHeight(definition.height),
		definition.skinningMode);
	if (m_hasIdleAnimation)
	{
		m_model.AddAnimation(IdleAnimationName, std::string(definition.idleAnimationPath));
	}
	if (m_hasJoggingAnimation)
	{
		m_model.AddAnimation(JoggingAnimationName, std::string(definition.joggingAnimationPath));
	}
	m_model.SetPosition(m_position.x, m_position.y, m_position.z);
	m_model.SetRotationY(m_rotationY);
	if (m_hasIdleAnimation)
	{
		m_model.PlayAnimation(IdleAnimationName);
	}
}

void Player::Update(float deltaTime, const Input& input)
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

	const float length = std::sqrt(movement.x * movement.x + movement.z * movement.z);
	XMFLOAT3 inputDisplacement{};
	if (length > 0.0f && m_hasJoggingAnimation)
	{
		movement.x /= length;
		movement.z /= length;
		inputDisplacement.x = movement.x * MoveSpeed * deltaTime;
		inputDisplacement.z = movement.z * MoveSpeed * deltaTime;

		m_rotationY = std::atan2(movement.x, movement.z) + XM_PI;
		m_model.SetRotationY(m_rotationY);
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
			m_rotationY = std::atan2(movement.x, movement.z) + XM_PI;
			m_model.SetRotationY(m_rotationY);
		}
	}

	const RootMotionDelta rootMotionDelta = m_model.Update(deltaTime);
	const XMFLOAT3 rootMotionDisplacement = TransformRootMotionToWorld(rootMotionDelta.translation);
	const XMFLOAT3 displacement = ChooseDisplacement(inputDisplacement, rootMotionDisplacement);
	m_position.x += displacement.x;
	if (m_rootMotionVerticalMode == RootMotionVerticalMode::Apply)
	{
		m_position.y += displacement.y;
	}
	else
	{
		m_position.y = GroundHeight;
	}
	m_position.z += displacement.z;

	m_model.SetPosition(m_position.x, m_position.y, m_position.z);
}

void Player::UpdateIdle(float deltaTime)
{
	m_model.Update(deltaTime);
	m_model.SetPosition(m_position.x, m_position.y, m_position.z);
}

void Player::Draw(Dx12Renderer& renderer) const
{
	m_model.Draw(renderer);
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
		if (m_hasIdleAnimation)
		{
			m_model.PlayAnimation(IdleAnimationName);
		}
		m_position.y = GroundHeight;
		break;
	case AnimationState::Jogging:
		if (m_hasJoggingAnimation)
		{
			m_model.PlayAnimation(JoggingAnimationName);
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
	const XMVECTOR world = XMVector3TransformNormal(local, XMMatrixRotationY(m_rotationY));

	XMFLOAT3 result{};
	XMStoreFloat3(&result, world);
	if (m_rootMotionVerticalMode == RootMotionVerticalMode::Ignore)
	{
		result.y = 0.0f;
	}
	return result;
}

