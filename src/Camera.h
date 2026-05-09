#pragma once

#include "Input.h"

#include <DirectXMath.h>

class Camera
{
public:
	void Update(float deltaTime, const Input& input);
	void SetLens(float fovYRadians, float aspectRatio, float nearZ, float farZ);
	void SetPosition(float x, float y, float z);
	void SetTarget(float x, float y, float z);
	void LookAt(
		const DirectX::XMFLOAT3& position,
		const DirectX::XMFLOAT3& target,
		const DirectX::XMFLOAT3& up);
	void Move(float x, float y, float z);
	void MovePosition(float x, float y, float z);
	void MoveTarget(float x, float y, float z);

	DirectX::XMMATRIX GetViewMatrix() const;
	DirectX::XMMATRIX GetProjectionMatrix() const;
	DirectX::XMMATRIX GetViewProjectionMatrix() const;

private:
	DirectX::XMFLOAT3 m_position{ 0.0f, 9.0f, -9.0f };
	DirectX::XMFLOAT3 m_target{ 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 m_up{ 0.0f, 1.0f, 0.0f };

	float m_fovYRadians{ DirectX::XMConvertToRadians(55.0f) };
	float m_aspectRatio{ 16.0f / 9.0f };
	float m_nearZ{ 0.1f };
	float m_farZ{ 100.0f };
};

