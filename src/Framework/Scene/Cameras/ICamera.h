#pragma once

#include "Framework/Scene/Input/Input.h"

#include <DirectXMath.h>

class ICamera
{
public:
	virtual ~ICamera() = default;

	virtual void Update(float deltaTime, const Input& input) = 0;
	virtual void SetTarget(const DirectX::XMFLOAT3& position, float rotationY) = 0;
	virtual void SetLens(float fovYRadians, float aspectRatio, float nearZ, float farZ) = 0;
	virtual DirectX::XMFLOAT3 GetForwardXZ() const = 0;
	virtual DirectX::XMMATRIX GetViewMatrix() const = 0;
	virtual DirectX::XMMATRIX GetProjectionMatrix() const = 0;
	virtual DirectX::XMMATRIX GetViewProjectionMatrix() const = 0;
};
