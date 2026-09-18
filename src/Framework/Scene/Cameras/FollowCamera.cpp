#include "Framework/Scene/Cameras/FollowCamera.h"

#include "Framework/Core/Math/MathUtils.h"

#include <algorithm>
#include <cmath>

using namespace DirectX;

FollowCamera::FollowCamera()
{
	SetViewPose({ 0.0f, 4.5f, -7.0f }, { 0.0f, 1.2f, 0.0f });
}

void FollowCamera::Update(float deltaTime, const Input& input)
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
	if (yawInput != 0.0f)
	{
		m_cameraYaw = MathUtils::NormalizeAngle(m_cameraYaw + yawInput * m_orbitSpeed * deltaTime);
	}

	const bool isShiftDown = input.IsDown(InputKey::Shift);
	float verticalInput = 0.0f;
	if (input.IsDown(InputKey::Up))
	{
		verticalInput += 1.0f;
	}
	if (input.IsDown(InputKey::Down))
	{
		verticalInput -= 1.0f;
	}
	if (verticalInput != 0.0f)
	{
		if (isShiftDown)
		{
			m_distance = std::clamp(m_distance - verticalInput * m_distanceMoveSpeed * deltaTime, m_minDistance, m_maxDistance);
		}
		else
		{
			m_height = std::clamp(m_height + verticalInput * m_heightMoveSpeed * deltaTime, m_minHeight, m_maxHeight);
		}
	}

	if (input.IsDown(InputKey::Z))
	{
		const float zFocusAmount = MathUtils::SmoothAmount(m_zFocusSharpness, deltaTime);
		m_cameraYaw = MathUtils::LerpAngle(m_cameraYaw, m_targetRotationY, zFocusAmount);
	}

	const XMFLOAT3 desiredLookAt
	{
		m_targetPosition.x,
		m_targetPosition.y + m_lookAtHeight,
		m_targetPosition.z
	};

	const XMFLOAT3 desiredPosition
	{
		m_targetPosition.x + std::sin(m_cameraYaw) * m_distance,
		m_targetPosition.y + m_height,
		m_targetPosition.z + std::cos(m_cameraYaw) * m_distance
	};

	const float amount = MathUtils::SmoothAmount(m_followSharpness, deltaTime);
	m_position = MathUtils::Lerp(m_position, desiredPosition, amount);
	m_viewTarget = MathUtils::Lerp(m_viewTarget, desiredLookAt, amount);
}


void FollowCamera::SetTarget(const XMFLOAT3& position, float rotationY)
{
	m_targetPosition = position;
	m_targetRotationY = rotationY;
	if (!m_hasTarget)
	{
		m_hasTarget = true;
		m_cameraYaw = rotationY;
		m_viewTarget = XMFLOAT3{ position.x, position.y + m_lookAtHeight, position.z };
		m_position = XMFLOAT3
		{
			position.x + std::sin(m_cameraYaw) * m_distance,
			position.y + m_height,
			position.z + std::cos(m_cameraYaw) * m_distance
		};
	}
}
