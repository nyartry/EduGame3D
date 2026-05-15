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
}

void FollowCamera::Update(float deltaTime, const Input&)
{
	if (!m_hasTarget)
	{
		return;
	}

	const XMFLOAT3 desiredLookAt
	{
		m_targetPosition.x,
		m_targetPosition.y + m_lookAtHeight,
		m_targetPosition.z
	};

	const XMFLOAT3 desiredPosition
	{
		m_targetPosition.x + std::sin(m_targetRotationY) * m_distance,
		m_targetPosition.y + m_height,
		m_targetPosition.z + std::cos(m_targetRotationY) * m_distance
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
		m_lookAt = XMFLOAT3{ position.x, position.y + m_lookAtHeight, position.z };
		m_position = XMFLOAT3
		{
			position.x + std::sin(rotationY) * m_distance,
			position.y + m_height,
			position.z + std::cos(rotationY) * m_distance
		};
	}
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
