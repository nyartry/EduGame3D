#include "Models/SolidStaticMeshActor.h"

using namespace DirectX;

const StaticMeshActorDefinition& SolidStaticMeshActor::GetStaticMeshDefinition() const
{
	return GetSolidStaticMeshDefinition().mesh;
}

XMFLOAT3 SolidStaticMeshActor::GetCollisionPosition() const
{
	return GetPosition();
}

void SolidStaticMeshActor::SetCollisionPosition(const XMFLOAT3& position)
{
	SetPosition(position);
}

CollisionBodyDefinition SolidStaticMeshActor::GetCollisionBodyDefinition() const
{
	return GetSolidStaticMeshDefinition().collision;
}
