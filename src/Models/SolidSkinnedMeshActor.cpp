#include "Models/SolidSkinnedMeshActor.h"

using namespace DirectX;

const SkinnedMeshActorDefinition& SolidSkinnedMeshActor::GetSkinnedMeshDefinition() const
{
	return GetSolidSkinnedMeshDefinition().mesh;
}

XMFLOAT3 SolidSkinnedMeshActor::GetCollisionPosition() const
{
	const XMFLOAT3 actorPosition = GetPosition();
	const XMFLOAT3 localCenter = GetModel().GetAnimatedBoundsCenterLocal();
	const XMVECTOR localOffset = XMVectorSet(localCenter.x, 0.0f, localCenter.z, 0.0f);
	const XMVECTOR worldOffset = XMVector3TransformNormal(localOffset, XMMatrixRotationY(GetRotationY()));

	XMFLOAT3 offset{};
	XMStoreFloat3(&offset, worldOffset);
	return XMFLOAT3
	{
		actorPosition.x + offset.x,
		actorPosition.y,
		actorPosition.z + offset.z
	};
}

void SolidSkinnedMeshActor::SetCollisionPosition(const XMFLOAT3& position)
{
	const XMFLOAT3 localCenter = GetModel().GetAnimatedBoundsCenterLocal();
	const XMVECTOR localOffset = XMVectorSet(localCenter.x, 0.0f, localCenter.z, 0.0f);
	const XMVECTOR worldOffset = XMVector3TransformNormal(localOffset, XMMatrixRotationY(GetRotationY()));

	XMFLOAT3 offset{};
	XMStoreFloat3(&offset, worldOffset);
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
