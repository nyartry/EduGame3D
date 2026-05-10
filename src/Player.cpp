#include "Player.h"

#include "Dx12Renderer.h"

#include <cmath>

using namespace DirectX;

namespace
{
	constexpr const char* IdleModelPath = "Content\\Models\\Player\\Orc Idle\\Orc Idle.fbx";
	constexpr const char* JoggingAnimationPath = "Content\\Models\\Player\\Jogging\\Jogging.fbx";
	constexpr const char* IdleAnimationName = "Idle";
	constexpr const char* JoggingAnimationName = "Jogging";
	constexpr float MoveSpeed = 3.0f;
}

void Player::Initialize(ID3D12Device* device)
{
	m_model.Initialize(device, IdleModelPath);
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
	if (length > 0.0f)
	{
		movement.x /= length;
		movement.z /= length;
		m_position.x += movement.x * MoveSpeed * deltaTime;
		m_position.z += movement.z * MoveSpeed * deltaTime;

		m_model.SetRotationY(std::atan2(movement.x, movement.z) + XM_PI);
		SetAnimationState(AnimationState::Jogging);
	}
	else
	{
		SetAnimationState(AnimationState::Idle);
	}

	m_model.SetPosition(m_position.x, m_position.y, m_position.z);
	m_model.Update(deltaTime);
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
