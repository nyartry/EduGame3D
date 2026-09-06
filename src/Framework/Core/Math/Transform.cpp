#include "Framework/Core/Math/Transform.h"

#include "Framework/Core/Math/MathUtils.h"

using namespace DirectX;

XMMATRIX Transform::ToMatrix() const
{
	return XMMatrixScaling(scale.x, scale.y, scale.z) *
		XMMatrixRotationRollPitchYaw(rotationRadians.x, rotationRadians.y, rotationRadians.z) *
		XMMatrixTranslation(position.x, position.y, position.z);
}

XMFLOAT3 Transform::TransformPoint(const XMFLOAT3& point) const
{
	XMFLOAT3 result;
	XMStoreFloat3(&result, XMVector3TransformCoord(XMLoadFloat3(&point), ToMatrix()));
	return result;
}

XMFLOAT3 Transform::TransformDirection(const XMFLOAT3& direction) const
{
	XMFLOAT3 result;
	XMStoreFloat3(&result, XMVector3TransformNormal(XMLoadFloat3(&direction), ToMatrix()));
	return result;
}

bool Transform::TryTransformNormal(const XMFLOAT3& normal, XMFLOAT3& transformed) const
{
	// Normalize before the multiplication so extreme input magnitudes cannot
	// overflow. Capture the input before clearing output to support aliasing.
	XMFLOAT3 normalized;
	const bool validInput = MathUtils::TryNormalize(normal, normalized, 0.0f);
	transformed = {};
	XMMATRIX matrix;
	if (!validInput || !MathUtils::TryCreateNormalMatrix(ToMatrix(), matrix))
	{
		return false;
	}

	XMFLOAT4X4 coefficients;
	XMStoreFloat4x4(&coefficients, matrix);
	const double x = static_cast<double>(normalized.x) * coefficients._11 +
		static_cast<double>(normalized.y) * coefficients._21 + static_cast<double>(normalized.z) * coefficients._31;
	const double y = static_cast<double>(normalized.x) * coefficients._12 +
		static_cast<double>(normalized.y) * coefficients._22 + static_cast<double>(normalized.z) * coefficients._32;
	const double z = static_cast<double>(normalized.x) * coefficients._13 +
		static_cast<double>(normalized.y) * coefficients._23 + static_cast<double>(normalized.z) * coefficients._33;
	const double length = std::hypot(x, y, z);
	if (!std::isfinite(length) || length == 0.0)
	{
		return false;
	}
	transformed = { static_cast<float>(x / length), static_cast<float>(y / length), static_cast<float>(z / length) };
	return true;
}
