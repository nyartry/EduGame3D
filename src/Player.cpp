#include "Player.h"

#include "Dx12Renderer.h"

#include <algorithm>
#include <cmath>

using namespace DirectX;

namespace
{
	constexpr const char* IdleModelPath = "Content\\Models\\Player\\Orc Idle\\Orc Idle.fbx";
	constexpr const char* JoggingAnimationPath = "Content\\Models\\Player\\Jogging\\Jogging.fbx";
	constexpr const char* IdleAnimationName = "Idle";
	constexpr const char* JoggingAnimationName = "Jogging";
	constexpr float PlayerHeight = 1.8f;
	constexpr float MoveSpeed = 3.0f;
	constexpr SkinningMode PlayerSkinningMode = SkinningMode::Gpu;
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
	m_model.Initialize(
		device,
		IdleModelPath,
		ModelScaleSettings::NormalizeToHeight(PlayerHeight),
		PlayerSkinningMode);
	m_model.AddAnimation(IdleAnimationName, IdleModelPath);
	m_model.AddAnimation(JoggingAnimationName, JoggingAnimationPath);
	m_model.SetPosition(m_position.x, m_position.y, m_position.z);
	m_model.PlayAnimation(IdleAnimationName);
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
	if (length > 0.0f)
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
		m_model.PlayAnimation(IdleAnimationName);
		break;
	case AnimationState::Jogging:
		m_model.PlayAnimation(JoggingAnimationName);
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
