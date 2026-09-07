#pragma once

#include "Framework/Animation/AnimationPlayback.h"
#include "Framework/Animation/RootMotion.h"

#include <vector>

struct AnimationClip;
struct BoneData;

// Raw model-local translation, including Y. Character policy decides which axes apply.
RootMotionDelta ExtractRootMotionDelta(
	const AnimationClip& clip,
	const std::vector<BoneData>& bones,
	const AnimationPlaybackInterval& interval,
	float modelScale = 1.0f);
