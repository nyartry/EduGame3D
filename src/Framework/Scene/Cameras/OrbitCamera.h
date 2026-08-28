#pragma once

#include "Framework/Scene/Cameras/CameraBase.h"

class OrbitCamera final : public CameraBase
{
public:
	void Update(float deltaTime, const Input& input) override;
	void SetTarget(const DirectX::XMFLOAT3& position, float rotationY) override;

private:
	void RefreshViewPose();

	DirectX::XMFLOAT3 m_followTarget{};
	float m_targetRotationY{};
	float m_yaw{};
	float m_pitch{ DirectX::XMConvertToRadians(25.0f) };
	float m_distance{ 7.0f };
	float m_minDistance{ 2.5f };
	float m_maxDistance{ 12.0f };
	float m_lookAtHeight{ 1.2f };
	float m_orbitSpeed{ DirectX::XMConvertToRadians(100.0f) };
	float m_zoomSpeed{ 4.0f };
	float m_maxPitch{ DirectX::XMConvertToRadians(75.0f) };
	bool m_hasTarget{};
};
