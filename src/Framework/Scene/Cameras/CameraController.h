#pragma once

#include "Framework/Scene/Cameras/ICamera.h"

#include <DirectXMath.h>
#include <memory>
#include <string_view>

enum class CameraMode
{
	Follow,
	SpringFollow,
	FirstPerson,
	Orbit,
	Spline
};

std::string_view GetCameraModeName(CameraMode mode);

// Owns the active camera strategy and preserves the scene-facing ICamera API.
// Re-selecting a mode creates a fresh camera, which also restarts spline shots.
class CameraController final : public ICamera
{
public:
	CameraController();

	void SetMode(CameraMode mode);
	CameraMode GetMode() const;

	void Update(float deltaTime, const Input& input) override;
	void SetTarget(const DirectX::XMFLOAT3& position, float rotationY) override;
	void SetLens(float fovYRadians, float aspectRatio, float nearZ, float farZ) override;
	DirectX::XMFLOAT3 GetForwardXZ() const override;
	DirectX::XMMATRIX GetViewMatrix() const override;
	DirectX::XMMATRIX GetProjectionMatrix() const override;
	DirectX::XMMATRIX GetViewProjectionMatrix() const override;

private:
	std::unique_ptr<ICamera> CreateCamera(CameraMode mode) const;

	std::unique_ptr<ICamera> m_activeCamera;
	CameraMode m_mode{ CameraMode::Follow };
	DirectX::XMFLOAT3 m_targetPosition{};
	float m_targetRotationY{};
	float m_fovYRadians{ DirectX::XMConvertToRadians(55.0f) };
	float m_aspectRatio{ 16.0f / 9.0f };
	float m_nearZ{ 0.1f };
	float m_farZ{ 100.0f };
	bool m_hasTarget{};
};
