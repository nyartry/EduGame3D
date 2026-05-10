#include "RootMotionPolicy.h"

bool RootMotionPolicy::ShouldLockTranslationToBindPose(
	const std::string& boneName,
	int boneIndex,
	const std::vector<BoneData>& bones)
{
	if (IsNamedRootMotionBone(boneName) || IsAssimpFbxTranslationNode(boneName))
	{
		return true;
	}

	return boneIndex >= 0 &&
		boneIndex < static_cast<int>(bones.size()) &&
		bones[boneIndex].parentIndex < 0;
}

bool RootMotionPolicy::IsNamedRootMotionBone(const std::string& boneName)
{
	return boneName == "Root" ||
		boneName == "RootNode" ||
		boneName == "Armature" ||
		boneName == "Hips" ||
		boneName.ends_with(":Root") ||
		boneName.ends_with("_Root") ||
		boneName.ends_with(":Hips") ||
		boneName.ends_with("_Hips");
}

bool RootMotionPolicy::IsAssimpFbxTranslationNode(const std::string& boneName)
{
	return boneName.find("_$AssimpFbx$_Translation") != std::string::npos;
}
