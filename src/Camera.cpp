#include "Camera.h"

using namespace DirectX;

void Camera::SetLens(float fovYRadians, float aspectRatio, float nearZ, float farZ)
{
	m_fovYRadians = fovYRadians;
	m_aspectRatio = aspectRatio;
	m_nearZ = nearZ;
	m_farZ = farZ;
}

void Camera::LookAt(const XMFLOAT3& position, const XMFLOAT3& target, const XMFLOAT3& up)
{
	m_position = position;
	m_target = target;
	m_up = up;
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

