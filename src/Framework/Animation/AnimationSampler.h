#pragma once

#include "Framework/Models/SkinnedModelData.h"

#include <DirectXMath.h>

class AnimationSampler
{
public:
	// Takes the shared playback's local seconds. The exact duration samples the
	// final key for root extraction; this sampler never wraps independently.
	DirectX::XMVECTOR SampleTranslation(
		const AnimationClip& clip,
		const BoneAnimation& boneAnimation,
		const BoneData& bindPose,
		double animationTimeSeconds) const;

	DirectX::XMMATRIX SampleLocalTransform(
		const AnimationClip& clip,
		const BoneAnimation& boneAnimation,
		const BoneData& bindPose,
		double animationTimeSeconds) const;

private:
	static double GetAnimationTimeTicks(const AnimationClip& clip, double animationTimeSeconds);
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
