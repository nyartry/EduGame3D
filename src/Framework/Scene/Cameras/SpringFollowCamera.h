#pragma once

#include "Framework/Scene/Cameras/CameraBase.h"

class SpringFollowCamera final : public CameraBase
{
public:
	void Update(float deltaTime, const Input& input) override;
	void SetTarget(const DirectX::XMFLOAT3& position, float rotationY) override;

private:
	DirectX::XMFLOAT3 ComputeIdealPosition() const;
	DirectX::XMFLOAT3 ComputeViewTarget() const;

	DirectX::XMFLOAT3 m_followTarget{};
	DirectX::XMFLOAT3 m_velocity{};
	float m_targetRotationY{};
	float m_horizontalDistance{ 7.0f };
	float m_verticalDistance{ 4.5f };
	float m_targetDistance{ 1.5f };
	float m_lookAtHeight{ 1.2f };
	float m_springConstant{ 32.0f };
	bool m_hasTarget{};
};
