#include "Framework/Animation/RootMotion.h"

#include <algorithm>
#include <cmath>

using namespace DirectX;

float RootMotionSettings::GetWeight() const
{
	switch (mode)
	{
	case RootMotionMode::Apply:
		return 1.0f;
	case RootMotionMode::Blend:
		return std::isfinite(blendWeight) ? std::clamp(blendWeight, 0.0f, 1.0f) : 0.0f;
	case RootMotionMode::Ignore:
	default:
		return 0.0f;
	}
}

XMFLOAT3 ResolveRootMotionDisplacement(
	const XMFLOAT3& programDisplacement,
	const RootMotionDelta& rootMotion,
	float rotationY,
	const RootMotionSettings& settings)
{
	const float weight = settings.GetWeight();
	if (weight == 0.0f)
	{
		return programDisplacement;
	}

	XMFLOAT3 worldRootMotion{};
	XMStoreFloat3(&worldRootMotion, XMVector3TransformNormal(
		XMLoadFloat3(&rootMotion.translation), XMMatrixRotationY(rotationY)));
	return XMFLOAT3
	{
		std::lerp(programDisplacement.x, worldRootMotion.x, weight),
		programDisplacement.y + (settings.verticalMode == RootMotionVerticalMode::Apply
			? worldRootMotion.y * weight : 0.0f),
		std::lerp(programDisplacement.z, worldRootMotion.z, weight)
	};
}
