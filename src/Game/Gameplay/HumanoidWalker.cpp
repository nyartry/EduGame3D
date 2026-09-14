#include "Game/Gameplay/HumanoidWalker.h"

using namespace DirectX;

const SolidSkinnedMeshActorDefinition& HumanoidWalker::GetSolidSkinnedMeshDefinition() const
{
	static const SolidSkinnedMeshActorDefinition definition
	{
		SkinnedMeshActorDefinition
		{
			"Content\\Models\\EduHuman\\EduHuman_Jog.fbx",
			"Content\\Models\\EduHuman\\EduHuman_Jog.fbx",
			1.8f,
			XMFLOAT3{ 3.0f, 0.0f, 0.0f },
			XM_PI,
			SkinningMode::Gpu,
			RootMotionSettings{ .mode = RootMotionMode::Ignore }
		},
		CollisionBodyDefinition{ 0.4f, 1.8f, true },
		SkinnedCollisionAnchorDefinition{ SkinnedCollisionAnchorMode::ActorOrigin }
	};
	return definition;
}
