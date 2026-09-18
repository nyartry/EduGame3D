#include "Framework/Scene/Cameras/CameraController.h"

#include "Framework/Scene/Cameras/FirstPersonCamera.h"
#include "Framework/Scene/Cameras/FollowCamera.h"
#include "Framework/Scene/Cameras/OrbitCamera.h"
#include "Framework/Scene/Cameras/SplineCamera.h"
#include "Framework/Scene/Cameras/SpringFollowCamera.h"

#include <utility>

using namespace DirectX;

std::string_view GetCameraModeName(CameraMode mode)
{
	switch (mode)
	{
	case CameraMode::Follow: return "Follow";
	case CameraMode::SpringFollow: return "Spring Follow";
	case CameraMode::FirstPerson: return "First Person";
	case CameraMode::Orbit: return "Orbit";
	case CameraMode::Spline: return "Spline";
	}
	return "Unknown";
}

CameraController::CameraController()
{
	SetMode(CameraMode::Follow);
}

void CameraController::SetMode(CameraMode mode)
{
	std::unique_ptr<ICamera> camera = CreateCamera(mode);
	camera->SetLens(m_fovYRadians, m_aspectRatio, m_nearZ, m_farZ);
	if (m_hasTarget)
	{
		camera->SetTarget(m_targetPosition, m_targetRotationY);
	}
	m_activeCamera = std::move(camera);
	m_mode = mode;
}

CameraMode CameraController::GetMode() const
{
	return m_mode;
}

void CameraController::Update(float deltaTime, const Input& input)
{
	m_activeCamera->Update(deltaTime, input);
}

void CameraController::SetTarget(const XMFLOAT3& position, float rotationY)
{
	m_targetPosition = position;
	m_targetRotationY = rotationY;
	m_hasTarget = true;
	m_activeCamera->SetTarget(position, rotationY);
}

void CameraController::SetLens(float fovYRadians, float aspectRatio, float nearZ, float farZ)
{
	m_fovYRadians = fovYRadians;
	m_aspectRatio = aspectRatio;
	m_nearZ = nearZ;
	m_farZ = farZ;
	m_activeCamera->SetLens(fovYRadians, aspectRatio, nearZ, farZ);
}

XMFLOAT3 CameraController::GetForwardXZ() const
{
	return m_activeCamera->GetForwardXZ();
}

XMMATRIX CameraController::GetViewMatrix() const
{
	return m_activeCamera->GetViewMatrix();
}

XMMATRIX CameraController::GetProjectionMatrix() const
{
	return m_activeCamera->GetProjectionMatrix();
}

XMMATRIX CameraController::GetViewProjectionMatrix() const
{
	return m_activeCamera->GetViewProjectionMatrix();
}

std::unique_ptr<ICamera> CameraController::CreateCamera(CameraMode mode) const
{
	switch (mode)
	{
	case CameraMode::Follow: return std::make_unique<FollowCamera>();
	case CameraMode::SpringFollow: return std::make_unique<SpringFollowCamera>();
	case CameraMode::FirstPerson: return std::make_unique<FirstPersonCamera>();
	case CameraMode::Orbit: return std::make_unique<OrbitCamera>();
	case CameraMode::Spline: return std::make_unique<SplineCamera>();
	}
	return std::make_unique<FollowCamera>();
}
