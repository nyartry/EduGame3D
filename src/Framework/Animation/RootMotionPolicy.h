#pragma once

#include "Framework/Models/SkinnedModelData.h"

#include <string>
#include <vector>

class RootMotionPolicy
{
public:
	static int GetRootMotionPriority(
		const std::string& boneName,
		int boneIndex,
		const std::vector<BoneData>& bones);

	static bool ShouldLockTranslationToBindPose(
		const std::string& boneName,
		int boneIndex,
		const std::vector<BoneData>& bones);

private:
	static bool IsNamedRootMotionBone(const std::string& boneName);
	static bool IsAssimpFbxTranslationNode(const std::string& boneName);
};
