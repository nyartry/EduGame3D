#include "Game/Gameplay/AnimatedCubeObject.h"

using namespace DirectX;

const SolidSkinnedMeshActorDefinition& AnimatedCubeObject::GetSolidSkinnedMeshDefinition() const
{
	static const SolidSkinnedMeshActorDefinition definition
	{
		SkinnedMeshActorDefinition
		{
			"Content\\Models\\Untitled\\Untitled.fbx",
			"Content\\Models\\Untitled\\Untitled.fbx",
			1.0f,
			XMFLOAT3{ 0.0f, 0.0f, -2.0f },
			0.0f,
			SkinningMode::Gpu
		},
		CollisionBodyDefinition{ 0.5f, 1.0f, true },
		SkinnedCollisionAnchorDefinition{ SkinnedCollisionAnchorMode::ActorOrigin }
	};
	return definition;
}
