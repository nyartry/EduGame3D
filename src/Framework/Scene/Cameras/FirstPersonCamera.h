#pragma once

#include "Framework/Scene/Cameras/CameraBase.h"

class FirstPersonCamera final : public CameraBase
{
public:
	void Update(float deltaTime, const Input& input) override;
	void SetTarget(const DirectX::XMFLOAT3& position, float rotationY) override;

private:
	void RefreshViewPose();

	DirectX::XMFLOAT3 m_followTarget{};
	float m_targetRotationY{};
	float m_yaw{};
	float m_pitch{};
	float m_eyeHeight{ 1.65f };
	float m_yawSpeed{ DirectX::XMConvertToRadians(120.0f) };
	float m_pitchSpeed{ DirectX::XMConvertToRadians(90.0f) };
	float m_maxPitch{ DirectX::XMConvertToRadians(70.0f) };
	bool m_hasTarget{};
};
