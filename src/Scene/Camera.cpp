#include "Scene/Camera.h"

using namespace DirectX;

namespace
{
	constexpr float CameraMoveSpeed = 5.0f;
}

void Camera::Update(float deltaTime, const Input& input)
{
	const float moveDistance = CameraMoveSpeed * deltaTime;
	float moveX = 0.0f;
	float moveY = 0.0f;

	if (input.IsDown(InputKey::Left))
	{
		moveX -= moveDistance;
	}
	if (input.IsDown(InputKey::Right))
	{
		moveX += moveDistance;
	}
	if (input.IsDown(InputKey::Up))
	{
		moveY += moveDistance;
	}
	if (input.IsDown(InputKey::Down))
	{
		moveY -= moveDistance;
	}

	MovePosition(moveX, moveY, 0.0f);
}

void Camera::SetLens(float fovYRadians, float aspectRatio, float nearZ, float farZ)
{
	m_fovYRadians = fovYRadians;
	m_aspectRatio = aspectRatio;
	m_nearZ = nearZ;
	m_farZ = farZ;
}

void Camera::SetPosition(float x, float y, float z)
{
	m_position = XMFLOAT3{ x, y, z };
}

void Camera::SetTarget(float x, float y, float z)
{
	m_target = XMFLOAT3{ x, y, z };
}

void Camera::LookAt(const XMFLOAT3& position, const XMFLOAT3& target, const XMFLOAT3& up)
{
	m_position = position;
	m_target = target;
	m_up = up;
}

void Camera::Move(float x, float y, float z)
{
	MovePosition(x, y, z);
	MoveTarget(x, y, z);
}

void Camera::MovePosition(float x, float y, float z)
{
	m_position.x += x;
	m_position.y += y;
	m_position.z += z;
}

void Camera::MoveTarget(float x, float y, float z)
{
	m_target.x += x;
	m_target.y += y;
	m_target.z += z;
}

XMMATRIX Camera::GetViewMatrix() const
{
	const XMVECTOR position = XMLoadFloat3(&m_position);
	const XMVECTOR target = XMLoadFloat3(&m_target);
	const XMVECTOR up = XMLoadFloat3(&m_up);
	return XMMatrixLookAtLH(position, target, up);
}

XMMATRIX Camera::GetProjectionMatrix() const
{
	return XMMatrixPerspectiveFovLH(m_fovYRadians, m_aspectRatio, m_nearZ, m_farZ);
}

XMMATRIX Camera::GetViewProjectionMatrix() const
{
	return GetViewMatrix() * GetProjectionMatrix();
}

