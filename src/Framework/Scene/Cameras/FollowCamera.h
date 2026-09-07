#pragma once

#include "Framework/Scene/Cameras/CameraBase.h"

#include <DirectXMath.h>

class FollowCamera : public CameraBase
{
public:
	FollowCamera();
	void Update(float deltaTime, const Input& input) override;
	void SetTarget(const DirectX::XMFLOAT3& position, float rotationY) override;

private:
	DirectX::XMFLOAT3 m_targetPosition{ 0.0f, 0.0f, 0.0f };

	float m_targetRotationY{};
	float m_cameraYaw{};
	float m_distance{ 7.0f };
	float m_minDistance{ 3.0f };
	float m_maxDistance{ 12.0f };
	float m_height{ 4.5f };
	float m_minHeight{ 2.0f };
	float m_maxHeight{ 8.0f };
	float m_orbitSpeed{ DirectX::XMConvertToRadians(120.0f) };
	float m_heightMoveSpeed{ 4.0f };
	float m_distanceMoveSpeed{ 4.0f };
	float m_lookAtHeight{ 1.2f };
	float m_followSharpness{ 8.0f };
	float m_zFocusSharpness{ 10.0f };
	bool m_hasTarget{};
};
