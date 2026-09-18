#include "Framework/Scene/Cameras/FirstPersonCamera.h"

#include "Framework/Scene/Input/Input.h"

#include <algorithm>
#include <cmath>

using namespace DirectX;

void FirstPersonCamera::Update(float deltaTime, const Input& input)
{
	if (!m_hasTarget)
	{
		return;
	}

	float yawInput = 0.0f;
	if (input.IsDown(InputKey::Left))
	{
		yawInput -= 1.0f;
	}
	if (input.IsDown(InputKey::Right))
	{
		yawInput += 1.0f;
	}

	float pitchInput = 0.0f;
	if (input.IsDown(InputKey::Up))
	{
		pitchInput += 1.0f;
	}
	if (input.IsDown(InputKey::Down))
	{
		pitchInput -= 1.0f;
	}

	m_yaw += yawInput * m_yawSpeed * deltaTime;
	m_pitch = std::clamp(
		m_pitch + pitchInput * m_pitchSpeed * deltaTime,
		-m_maxPitch,
		m_maxPitch);

	if (input.IsDown(InputKey::Z))
	{
		m_yaw = m_targetRotationY;
	}

	RefreshViewPose();
}

void FirstPersonCamera::SetTarget(const XMFLOAT3& position, float rotationY)
{
	m_followTarget = position;
	m_targetRotationY = rotationY;
	if (!m_hasTarget)
	{
		m_hasTarget = true;
		m_yaw = rotationY;
	}
	RefreshViewPose();
}

void FirstPersonCamera::RefreshViewPose()
{
	const float horizontalScale = std::cos(m_pitch);
	const XMFLOAT3 forward
	{
		-std::sin(m_yaw) * horizontalScale,
		std::sin(m_pitch),
		-std::cos(m_yaw) * horizontalScale
	};
	const XMFLOAT3 position
	{
		m_followTarget.x,
		m_followTarget.y + m_eyeHeight,
		m_followTarget.z
	};
	const XMFLOAT3 viewTarget
	{
		position.x + forward.x,
		position.y + forward.y,
		position.z + forward.z
	};
	SetViewPose(position, viewTarget);
}
