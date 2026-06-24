#include "Models/SolidSkinnedMeshActor.h"

using namespace DirectX;

const SkinnedMeshActorDefinition& SolidSkinnedMeshActor::GetSkinnedMeshDefinition() const
{
	return GetSolidSkinnedMeshDefinition().mesh;
}

XMFLOAT3 SolidSkinnedMeshActor::GetCollisionPosition() const
{
	return GetPosition();
}

void SolidSkinnedMeshActor::SetCollisionPosition(const XMFLOAT3& position)
{
	SetPosition(position);
}

CollisionBodyDefinition SolidSkinnedMeshActor::GetCollisionBodyDefinition() const
{
	return GetSolidSkinnedMeshDefinition().collision;
}
