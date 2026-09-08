#include "Framework/Animation/AnimationSampler.h"
#include "Framework/Animation/AnimationPlayback.h"

#include <algorithm>
#include <cmath>

using namespace DirectX;

namespace
{
	XMMATRIX LoadMatrix(const XMFLOAT4X4& matrix)
	{
		return XMLoadFloat4x4(&matrix);
	}

	XMMATRIX BuildTransform(const XMVECTOR& translation, const XMVECTOR& rotation, const XMVECTOR& scale)
	{
		return XMMatrixScalingFromVector(scale) *
			XMMatrixRotationQuaternion(rotation) *
			XMMatrixTranslationFromVector(translation);
	}

	XMVECTOR RemoveRootMotionTranslation(const BoneAnimation& boneAnimation, XMVECTOR translation, XMVECTOR bindTranslation)
	{
		if (!boneAnimation.lockTranslationToBindPose)
		{
			return translation;
		}

		return bindTranslation;
	}
}

XMMATRIX AnimationSampler::SampleLocalTransform(
	const AnimationClip& clip,
	const BoneAnimation& boneAnimation,
	const BoneData& bindPose,
	double animationTimeSeconds) const
{
	XMVECTOR bindScale = XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);
	XMVECTOR bindRotation = XMQuaternionIdentity();
	XMVECTOR bindTranslation = XMVectorZero();
	XMMatrixDecompose(&bindScale, &bindRotation, &bindTranslation, LoadMatrix(bindPose.localBindTransform));

	const double animationTimeTicks = GetAnimationTimeTicks(clip, animationTimeSeconds);
	const XMVECTOR translation = RemoveRootMotionTranslation(
		boneAnimation,
		SampleVectorKey(boneAnimation.translations, animationTimeTicks, bindTranslation),
		bindTranslation);
	const XMVECTOR rotation = SampleQuaternionKey(boneAnimation.rotations, animationTimeTicks, bindRotation);
	const XMVECTOR scale = SampleVectorKey(boneAnimation.scales, animationTimeTicks, bindScale);

	return BuildTransform(translation, rotation, scale);
}

XMVECTOR AnimationSampler::SampleTranslation(
	const AnimationClip& clip,
	const BoneAnimation& boneAnimation,
	const BoneData& bindPose,
	double animationTimeSeconds) const
{
	XMVECTOR bindScale = XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);
	XMVECTOR bindRotation = XMQuaternionIdentity();
	XMVECTOR bindTranslation = XMVectorZero();
	XMMatrixDecompose(&bindScale, &bindRotation, &bindTranslation, LoadMatrix(bindPose.localBindTransform));

	return SampleVectorKey(
		boneAnimation.translations,
		GetAnimationTimeTicks(clip, animationTimeSeconds),
		bindTranslation);
}

double AnimationSampler::GetAnimationTimeTicks(const AnimationClip& clip, double animationTimeSeconds)
{
	const double durationSeconds = GetAnimationDurationSeconds(clip);
	if (durationSeconds <= 0.0 || !std::isfinite(animationTimeSeconds)) return 0.0;
	return std::clamp(animationTimeSeconds, 0.0, durationSeconds) * clip.ticksPerSecond;
}

float AnimationSampler::GetInterpolationAmount(double fromTime, double toTime, double animationTimeTicks)
{
	const double duration = toTime - fromTime;
	if (duration <= 0.0)
	{
		return 0.0f;
	}

	return static_cast<float>((animationTimeTicks - fromTime) / duration);
}

XMVECTOR AnimationSampler::SampleVectorKey(
	const std::vector<VectorAnimationKey>& keys,
	double animationTimeTicks,
	XMVECTOR fallback)
{
	if (keys.empty())
	{
		return fallback;
	}

	if (keys.size() == 1 || animationTimeTicks <= keys.front().time)
	{
		return XMLoadFloat3(&keys.front().value);
	}

	for (size_t keyIndex = 1; keyIndex < keys.size(); ++keyIndex)
	{
		if (animationTimeTicks <= keys[keyIndex].time)
		{
			const XMVECTOR from = XMLoadFloat3(&keys[keyIndex - 1].value);
			const XMVECTOR to = XMLoadFloat3(&keys[keyIndex].value);
			return XMVectorLerp(from, to, GetInterpolationAmount(keys[keyIndex - 1].time, keys[keyIndex].time, animationTimeTicks));
		}
	}

	return XMLoadFloat3(&keys.back().value);
}

XMVECTOR AnimationSampler::SampleQuaternionKey(
	const std::vector<QuaternionAnimationKey>& keys,
	double animationTimeTicks,
	XMVECTOR fallback)
{
	if (keys.empty())
	{
		return fallback;
	}

	if (keys.size() == 1 || animationTimeTicks <= keys.front().time)
	{
		return XMQuaternionNormalize(XMLoadFloat4(&keys.front().value));
	}

	for (size_t keyIndex = 1; keyIndex < keys.size(); ++keyIndex)
	{
		if (animationTimeTicks <= keys[keyIndex].time)
		{
			const XMVECTOR from = XMLoadFloat4(&keys[keyIndex - 1].value);
			const XMVECTOR to = XMLoadFloat4(&keys[keyIndex].value);
			return XMQuaternionNormalize(XMQuaternionSlerp(from, to, GetInterpolationAmount(keys[keyIndex - 1].time, keys[keyIndex].time, animationTimeTicks)));
		}
	}

	return XMQuaternionNormalize(XMLoadFloat4(&keys.back().value));
}
