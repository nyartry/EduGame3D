#pragma once

#include "Framework/Models/SkinnedModelData.h"

#include <DirectXMath.h>

class AnimationSampler
{
public:
	DirectX::XMVECTOR SampleTranslation(
		const AnimationClip& clip,
		const BoneAnimation& boneAnimation,
		const BoneData& bindPose,
		float animationTimeSeconds) const;

	DirectX::XMMATRIX SampleLocalTransform(
		const AnimationClip& clip,
		const BoneAnimation& boneAnimation,
		const BoneData& bindPose,
		float animationTimeSeconds) const;

private:
	static double GetAnimationTimeTicks(const AnimationClip& clip, float animationTimeSeconds);
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
