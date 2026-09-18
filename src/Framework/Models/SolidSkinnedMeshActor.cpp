#include "Framework/Models/SolidSkinnedMeshActor.h"

using namespace DirectX;

XMFLOAT3 SolidSkinnedMeshActor::GetCollisionAnchorOffsetWorld() const
{
	XMFLOAT3 localAnchor = ResolveCollisionAnchorLocal();
	// The existing upright collider is anchored horizontally; its bottom is
	// still the actor's Y, independent of animation pose height.
	localAnchor.y = 0.0f;
	return GetTransform().TransformDirection(localAnchor);
}

const SkinnedMeshActorDefinition& SolidSkinnedMeshActor::GetSkinnedMeshDefinition() const
{
	return GetSolidSkinnedMeshDefinition().mesh;
}

XMFLOAT3 SolidSkinnedMeshActor::GetCollisionPosition() const
{
	const XMFLOAT3 actorPosition = GetPosition();
	const XMFLOAT3 offset = GetCollisionAnchorOffsetWorld();
	return XMFLOAT3
	{
		actorPosition.x + offset.x,
		actorPosition.y,
		actorPosition.z + offset.z
	};
}

void SolidSkinnedMeshActor::SetCollisionPosition(const XMFLOAT3& position)
{
	const XMFLOAT3 offset = GetCollisionAnchorOffsetWorld();
	SetPosition(XMFLOAT3
	{
		position.x - offset.x,
		position.y,
		position.z - offset.z
	});
}

CollisionBodyDefinition SolidSkinnedMeshActor::GetCollisionBodyDefinition() const
{
	return GetSolidSkinnedMeshDefinition().collision;
}

XMFLOAT3 SolidSkinnedMeshActor::ResolveCollisionAnchorLocal() const
{
	const SkinnedCollisionAnchorDefinition& anchor = GetSolidSkinnedMeshDefinition().collisionAnchor;
	XMFLOAT3 position{};

	switch (anchor.mode)
	{
	case SkinnedCollisionAnchorMode::ActorOrigin:
		return XMFLOAT3{};
	case SkinnedCollisionAnchorMode::RootMotionBone:
		if (GetModel().TryGetRootMotionBonePositionLocal(position))
		{
			return position;
		}
		break;
	case SkinnedCollisionAnchorMode::NamedBone:
		if (!anchor.boneName.empty() && GetModel().TryGetBonePositionLocal(anchor.boneName, position))
		{
			return position;
		}
		break;
	case SkinnedCollisionAnchorMode::AnimatedBounds:
		return GetModel().GetAnimatedBoundsCenterLocal();
	}

	if (GetModel().TryGetBonePositionLocal("Hips", position) ||
		GetModel().TryGetBonePositionLocal("Root", position) ||
		GetModel().TryGetRootMotionBonePositionLocal(position))
	{
		return position;
	}

	return XMFLOAT3{};
}
