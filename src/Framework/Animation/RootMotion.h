#pragma once

#include <DirectXMath.h>

enum class RootMotionMode
{
	Ignore,
	Apply,
	Blend,
};

enum class RootMotionVerticalMode
{
	// Programmatic jump and gravity remain responsible for vertical movement.
	Ignore,
	// Add weighted animation Y; Player gives an active programmatic jump priority.
	Apply,
};

struct RootMotionSettings
{
	RootMotionMode mode{ RootMotionMode::Apply };
	float blendWeight{ 0.5f };
	RootMotionVerticalMode verticalMode{ RootMotionVerticalMode::Ignore };

	float GetWeight() const;
};

struct RootMotionDelta
{
	DirectX::XMFLOAT3 translation{ 0.0f, 0.0f, 0.0f };
};

// Displacements are per update, not velocities. Root motion is model-local;
// programDisplacement and the result are world-space. Blend affects X/Z only;
// optional animation Y is additive and never scales programmatic vertical motion.
DirectX::XMFLOAT3 ResolveRootMotionDisplacement(
	const DirectX::XMFLOAT3& programDisplacement,
	const RootMotionDelta& rootMotion,
	float rotationY,
	const RootMotionSettings& settings);
