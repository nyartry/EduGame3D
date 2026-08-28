#include "Framework/Scene/Cameras/CameraBase.h"

#include "Framework/Common/MathUtils.h"

using namespace DirectX;

void CameraBase::SetLens(float fovYRadians, float aspectRatio, float nearZ, float farZ)
{
	m_fovYRadians = fovYRadians;
	m_aspectRatio = aspectRatio;
	m_nearZ = nearZ;
	m_farZ = farZ;
}

XMFLOAT3 CameraBase::GetForwardXZ() const
{
	return MathUtils::NormalizeXZOrDefault(XMFLOAT3
	{
		m_viewTarget.x - m_position.x,
		0.0f,
		m_viewTarget.z - m_position.z
	});
}

XMMATRIX CameraBase::GetViewMatrix() const
{
	return XMMatrixLookAtLH(
		XMLoadFloat3(&m_position),
		XMLoadFloat3(&m_viewTarget),
		XMLoadFloat3(&m_up));
}

XMMATRIX CameraBase::GetProjectionMatrix() const
{
	return XMMatrixPerspectiveFovLH(m_fovYRadians, m_aspectRatio, m_nearZ, m_farZ);
}

XMMATRIX CameraBase::GetViewProjectionMatrix() const
{
	return GetViewMatrix() * GetProjectionMatrix();
}

void CameraBase::SetViewPose(const XMFLOAT3& position, const XMFLOAT3& target, const XMFLOAT3& up)
{
	m_position = position;
	m_viewTarget = target;
	m_up = up;
}
