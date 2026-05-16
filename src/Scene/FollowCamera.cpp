#include "Scene/FollowCamera.h"

#include "Common/MathUtils.h"

#include <algorithm>
#include <cmath>

using namespace DirectX;

namespace
{
	float NormalizeAngle(float angle)
	{
		while (angle > XM_PI)
		{
			angle -= XM_2PI;
		}
		while (angle < -XM_PI)
		{
			angle += XM_2PI;
		}
		return angle;
	}

	float LerpAngle(float from, float to, float amount)
	{
		return from + NormalizeAngle(to - from) * amount;
	}
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
		m_cameraYaw = NormalizeAngle(m_cameraYaw + yawInput * m_orbitSpeed * deltaTime);
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
		m_cameraYaw = LerpAngle(m_cameraYaw, m_targetRotationY, zFocusAmount);
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
	m_lookAt = MathUtils::Lerp(m_lookAt, desiredLookAt, amount);
}

void FollowCamera::SetLens(float fovYRadians, float aspectRatio, float nearZ, float farZ)
{
	m_fovYRadians = fovYRadians;
	m_aspectRatio = aspectRatio;
	m_nearZ = nearZ;
	m_farZ = farZ;
}

void FollowCamera::SetFollowTarget(const XMFLOAT3& position, float rotationY)
{
	m_targetPosition = position;
	m_targetRotationY = rotationY;
	if (!m_hasTarget)
	{
		m_hasTarget = true;
		m_cameraYaw = rotationY;
		m_lookAt = XMFLOAT3{ position.x, position.y + m_lookAtHeight, position.z };
		m_position = XMFLOAT3
		{
			position.x + std::sin(m_cameraYaw) * m_distance,
			position.y + m_height,
			position.z + std::cos(m_cameraYaw) * m_distance
		};
	}
}

XMFLOAT3 FollowCamera::GetForwardXZ() const
{
	const XMFLOAT3 forward
	{
		m_lookAt.x - m_position.x,
		0.0f,
		m_lookAt.z - m_position.z
	};
	return MathUtils::NormalizeXZOrDefault(forward);
}

XMMATRIX FollowCamera::GetViewProjectionMatrix() const
{
	return GetViewMatrix() * GetProjectionMatrix();
}

XMMATRIX FollowCamera::GetViewMatrix() const
{
	const XMVECTOR position = XMLoadFloat3(&m_position);
	const XMVECTOR target = XMLoadFloat3(&m_lookAt);
	const XMVECTOR up = XMLoadFloat3(&m_up);
	return XMMatrixLookAtLH(position, target, up);
}

XMMATRIX FollowCamera::GetProjectionMatrix() const
{
	return XMMatrixPerspectiveFovLH(m_fovYRadians, m_aspectRatio, m_nearZ, m_farZ);
}
