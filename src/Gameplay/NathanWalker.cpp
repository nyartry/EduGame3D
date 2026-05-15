#include "Gameplay/NathanWalker.h"

using namespace DirectX;

const SkinnedMeshActorDefinition& NathanWalker::GetSkinnedMeshDefinition() const
{
	static const SkinnedMeshActorDefinition definition
	{
		"Content\\Models\\55-rp_nathan_animated_003_walking_fbx\\rp_nathan_animated_003_walking.fbx",
		"Content\\Models\\55-rp_nathan_animated_003_walking_fbx\\rp_nathan_animated_003_walking.fbx",
		1.8f,
		XMFLOAT3{ 3.0f, 0.0f, 0.0f },
		XM_PI,
		SkinningMode::Gpu
	};
	return definition;
}
