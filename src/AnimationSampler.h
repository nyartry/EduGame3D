#pragma once

#include "SkinnedModelData.h"

#include <DirectXMath.h>

class AnimationSampler
{
public:
	DirectX::XMMATRIX SampleLocalTransform(
		const AnimationClip& clip,
		const BoneAnimation& boneAnimation,
		const BoneData& bindPose,
		float animationTimeSeconds) const;

private:
	static float GetInterpolationAmount(double fromTime, double toTime, double animationTimeTicks);
	static DirectX::XMVECTOR SampleVectorKey(
		const std::vector<VectorAnimationKey>& keys,
		double animationTimeTicks,
		DirectX::XMVECTOR fallback);
	static DirectX::XMVECTOR SampleQuaternionKey(
		const std::vector<QuaternionAnimationKey>& keys,
		double animationTimeTicks,
		DirectX::XMVECTOR fallback);
};
