#pragma once

#include <DirectXMath.h>

// A local-to-world affine value, independent of scene hierarchy and rendering.
// Left-handed coordinates: +X right, +Y up, +Z forward; radians throughout.
// DirectX row vectors multiply scale * rotation * translation in that order.
struct Transform
{
	DirectX::XMFLOAT3 position{ 0.0f, 0.0f, 0.0f };
	// Pitch (X), yaw (Y), roll (Z), using XMMatrixRotationRollPitchYaw.
	DirectX::XMFLOAT3 rotationRadians{ 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 scale{ 1.0f, 1.0f, 1.0f };

	DirectX::XMMATRIX ToMatrix() const;
	DirectX::XMFLOAT3 TransformPoint(const DirectX::XMFLOAT3& point) const;
	// Applies rotation AND scale, preserving magnitude; excludes translation.
	DirectX::XMFLOAT3 TransformDirection(const DirectX::XMFLOAT3& direction) const;
	// Uses inverse-transpose and returns a unit normal. Invalid input or a
	// singular/nonfinite transform clears the output and returns false.
	bool TryTransformNormal(const DirectX::XMFLOAT3& normal, DirectX::XMFLOAT3& transformed) const;
};
