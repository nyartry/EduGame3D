#include "Scene/FollowCamera.h"

#include <algorithm>
#include <cmath>

using namespace DirectX;

namespace
{
	XMFLOAT3 LerpFloat3(const XMFLOAT3& from, const XMFLOAT3& to, float amount)
	{
		return XMFLOAT3
		{
			from.x * (1.0f - amount) + to.x * amount,
			from.y * (1.0f - amount) + to.y * amount,
			from.z * (1.0f - amount) + to.z * amount
		};
	}

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

	float heightInput = 0.0f;
	if (input.IsDown(InputKey::Up))
	{
		heightInput += 1.0f;
	}
	if (input.IsDown(InputKey::Down))
	{
		heightInput -= 1.0f;
	}
	if (heightInput != 0.0f)
	{
		m_height = std::clamp(m_height + heightInput * m_heightMoveSpeed * deltaTime, m_minHeight, m_maxHeight);
	}

	if (input.IsDown(InputKey::Z))
	{
		const float zFocusAmount = std::clamp(1.0f - std::exp(-m_zFocusSharpness * deltaTime), 0.0f, 1.0f);
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

	const float amount = std::clamp(1.0f - std::exp(-m_followSharpness * deltaTime), 0.0f, 1.0f);
	m_position = LerpFloat3(m_position, desiredPosition, amount);
	m_lookAt = LerpFloat3(m_lookAt, desiredLookAt, amount);
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
	XMFLOAT3 forward
	{
		m_lookAt.x - m_position.x,
		0.0f,
		m_lookAt.z - m_position.z
	};
	const float length = std::sqrt(forward.x * forward.x + forward.z * forward.z);
	if (length == 0.0f)
	{
		return XMFLOAT3{ 0.0f, 0.0f, 1.0f };
	}

	forward.x /= length;
	forward.z /= length;
	return forward;
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
