#include "Framework/Models/SolidSkinnedMeshActor.h"

using namespace DirectX;

namespace
{
	XMFLOAT3 RotateLocalOffset(const XMFLOAT3& localOffset, float rotationY)
	{
		const XMVECTOR local = XMVectorSet(localOffset.x, 0.0f, localOffset.z, 0.0f);
		const XMVECTOR world = XMVector3TransformNormal(local, XMMatrixRotationY(rotationY));

		XMFLOAT3 result{};
		XMStoreFloat3(&result, world);
		return result;
	}
}

const SkinnedMeshActorDefinition& SolidSkinnedMeshActor::GetSkinnedMeshDefinition() const
{
	return GetSolidSkinnedMeshDefinition().mesh;
}

XMFLOAT3 SolidSkinnedMeshActor::GetCollisionPosition() const
{
	const XMFLOAT3 actorPosition = GetPosition();
	const XMFLOAT3 localAnchor = ResolveCollisionAnchorLocal();
	const XMFLOAT3 offset = RotateLocalOffset(localAnchor, GetRotationY());
	return XMFLOAT3
	{
		actorPosition.x + offset.x,
		actorPosition.y,
		actorPosition.z + offset.z
	};
}

void SolidSkinnedMeshActor::SetCollisionPosition(const XMFLOAT3& position)
{
	const XMFLOAT3 localAnchor = ResolveCollisionAnchorLocal();
	const XMFLOAT3 offset = RotateLocalOffset(localAnchor, GetRotationY());
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
