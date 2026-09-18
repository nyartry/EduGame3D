#include "Framework/Animation/RootMotionExtractor.h"

#include "Framework/Animation/AnimationSampler.h"

#include <cmath>

using namespace DirectX;

RootMotionDelta ExtractRootMotionDelta(
	const AnimationClip& clip,
	const std::vector<BoneData>& bones,
	const AnimationPlaybackInterval& interval,
	float modelScale)
{
	if (!interval.advanced || !std::isfinite(modelScale) ||
		clip.rootMotionBoneAnimationIndex < 0 ||
		clip.rootMotionBoneAnimationIndex >= static_cast<int>(clip.boneAnimations.size())) return {};
	const BoneAnimation& animation = clip.boneAnimations[clip.rootMotionBoneAnimationIndex];
	if (animation.boneIndex < 0 || animation.boneIndex >= static_cast<int>(bones.size())) return {};
	const BoneData& bindPose = bones[animation.boneIndex];
	const AnimationSampler sampler;
	const XMVECTOR from = sampler.SampleTranslation(clip, animation, bindPose, interval.fromSeconds);
	const XMVECTOR to = sampler.SampleTranslation(clip, animation, bindPose, interval.toSeconds);
	XMVECTOR delta = to - from;
	if (interval.completedLoops != 0)
	{
		// Sample actual clip endpoints, not the first/last key (keys may extend past duration).
		const XMVECTOR start = sampler.SampleTranslation(clip, animation, bindPose, 0.0);
		const XMVECTOR end = sampler.SampleTranslation(clip, animation, bindPose, interval.durationSeconds);
		delta += (end - start) * static_cast<float>(interval.completedLoops);
	}
	RootMotionDelta result;
	XMStoreFloat3(&result.translation, delta * modelScale);
	return result;
}
