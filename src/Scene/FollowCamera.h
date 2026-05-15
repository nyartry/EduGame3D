#pragma once

#include "Scene/ICamera.h"

#include <DirectXMath.h>

class FollowCamera : public ICamera
{
public:
	void Update(float deltaTime, const Input& input) override;
	void SetLens(float fovYRadians, float aspectRatio, float nearZ, float farZ) override;
	void SetFollowTarget(const DirectX::XMFLOAT3& position, float rotationY);

	DirectX::XMMATRIX GetViewProjectionMatrix() const override;

private:
	DirectX::XMMATRIX GetViewMatrix() const;
	DirectX::XMMATRIX GetProjectionMatrix() const;

	DirectX::XMFLOAT3 m_position{ 0.0f, 4.5f, -7.0f };
	DirectX::XMFLOAT3 m_lookAt{ 0.0f, 1.2f, 0.0f };
	DirectX::XMFLOAT3 m_targetPosition{ 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 m_up{ 0.0f, 1.0f, 0.0f };

	float m_targetRotationY{};
	float m_distance{ 7.0f };
	float m_height{ 4.5f };
	float m_lookAtHeight{ 1.2f };
	float m_followSharpness{ 8.0f };
	float m_fovYRadians{ DirectX::XMConvertToRadians(55.0f) };
	float m_aspectRatio{ 16.0f / 9.0f };
	float m_nearZ{ 0.1f };
	float m_farZ{ 100.0f };
	bool m_hasTarget{};
};
