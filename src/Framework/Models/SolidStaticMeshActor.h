#pragma once

#include "Framework/Gameplay/CollisionBody.h"
#include "Framework/Models/StaticMeshActor.h"

struct SolidStaticMeshActorDefinition
{
	StaticMeshActorDefinition mesh;
	CollisionBodyDefinition collision;
};

class SolidStaticMeshActor : public StaticMeshActor, public CollisionBody
{
public:
	DirectX::XMFLOAT3 GetCollisionPosition() const override;
	void SetCollisionPosition(const DirectX::XMFLOAT3& position) override;
	CollisionBodyDefinition GetCollisionBodyDefinition() const override;

protected:
	const StaticMeshActorDefinition& GetStaticMeshDefinition() const final;
	virtual const SolidStaticMeshActorDefinition& GetSolidStaticMeshDefinition() const = 0;
};
