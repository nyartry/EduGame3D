#include "Gameplay/NathanWalker.h"

using namespace DirectX;

const SolidSkinnedMeshActorDefinition& NathanWalker::GetSolidSkinnedMeshDefinition() const
{
	static const SolidSkinnedMeshActorDefinition definition
	{
		SkinnedMeshActorDefinition
		{
			"Content\\Models\\55-rp_nathan_animated_003_walking_fbx\\rp_nathan_animated_003_walking.fbx",
			"Content\\Models\\55-rp_nathan_animated_003_walking_fbx\\rp_nathan_animated_003_walking.fbx",
			1.8f,
			XMFLOAT3{ 3.0f, 0.0f, 0.0f },
			XM_PI,
			SkinningMode::Gpu
		},
		CollisionBodyDefinition{ 0.4f, 1.8f, true },
		SkinnedCollisionAnchorDefinition{ SkinnedCollisionAnchorMode::NamedBone, "Hips" }
	};
	return definition;
}
