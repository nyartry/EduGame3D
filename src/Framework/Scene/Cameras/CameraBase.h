#pragma once

#include "Framework/Scene/Cameras/ICamera.h"

#include <DirectXMath.h>

// Shared view/lens implementation for cameras that only need to produce a
// position, look-at target, and up vector each frame.
class CameraBase : public ICamera
{
public:
	void SetLens(float fovYRadians, float aspectRatio, float nearZ, float farZ) final;

	DirectX::XMFLOAT3 GetForwardXZ() const final;
	DirectX::XMMATRIX GetViewMatrix() const final;
	DirectX::XMMATRIX GetProjectionMatrix() const final;
	DirectX::XMMATRIX GetViewProjectionMatrix() const final;

protected:
	void SetViewPose(
		const DirectX::XMFLOAT3& position,
		const DirectX::XMFLOAT3& target,
		const DirectX::XMFLOAT3& up = DirectX::XMFLOAT3{ 0.0f, 1.0f, 0.0f });

	DirectX::XMFLOAT3 m_position{};
	DirectX::XMFLOAT3 m_viewTarget{ 0.0f, 0.0f, 1.0f };
	DirectX::XMFLOAT3 m_up{ 0.0f, 1.0f, 0.0f };

private:
	float m_fovYRadians{ DirectX::XMConvertToRadians(55.0f) };
	float m_aspectRatio{ 16.0f / 9.0f };
	float m_nearZ{ 0.1f };
	float m_farZ{ 100.0f };
};
