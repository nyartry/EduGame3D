#include "Framework/Scene/Cameras/OrbitCamera.h"

#include "Framework/Scene/Input/Input.h"

#include <algorithm>
#include <cmath>

using namespace DirectX;

void OrbitCamera::Update(float deltaTime, const Input& input)
{
	if (!m_hasTarget)
	{
		return;
	}

	float yawInput = 0.0f;
	if (input.IsDown(InputKey::Left))
	{
		yawInput += 1.0f;
	}
	if (input.IsDown(InputKey::Right))
	{
		yawInput -= 1.0f;
	}
	m_yaw += yawInput * m_orbitSpeed * deltaTime;

	float verticalInput = 0.0f;
	if (input.IsDown(InputKey::Up))
	{
		verticalInput += 1.0f;
	}
	if (input.IsDown(InputKey::Down))
	{
		verticalInput -= 1.0f;
	}
	if (input.IsDown(InputKey::Shift))
	{
		m_distance = std::clamp(
			m_distance - verticalInput * m_zoomSpeed * deltaTime,
			m_minDistance,
			m_maxDistance);
	}
	else
	{
		m_pitch = std::clamp(
			m_pitch + verticalInput * m_orbitSpeed * deltaTime,
			-m_maxPitch,
			m_maxPitch);
	}

	if (input.IsDown(InputKey::Z))
	{
		m_yaw = m_targetRotationY;
	}

	RefreshViewPose();
}

void OrbitCamera::SetTarget(const XMFLOAT3& position, float rotationY)
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

void OrbitCamera::RefreshViewPose()
{
	const XMFLOAT3 viewTarget
	{
		m_followTarget.x,
		m_followTarget.y + m_lookAtHeight,
		m_followTarget.z
	};
	const float horizontalDistance = std::cos(m_pitch) * m_distance;
	const XMFLOAT3 position
	{
		viewTarget.x + std::sin(m_yaw) * horizontalDistance,
		viewTarget.y + std::sin(m_pitch) * m_distance,
		viewTarget.z + std::cos(m_yaw) * horizontalDistance
	};
	SetViewPose(position, viewTarget);
}
