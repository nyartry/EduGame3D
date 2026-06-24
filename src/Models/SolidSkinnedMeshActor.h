#pragma once

#include "Gameplay/CollisionBody.h"
#include "Models/SkinnedMeshActor.h"

struct SolidSkinnedMeshActorDefinition
{
	SkinnedMeshActorDefinition mesh;
	CollisionBodyDefinition collision;
};

class SolidSkinnedMeshActor : public SkinnedMeshActor, public CollisionBody
{
public:
	DirectX::XMFLOAT3 GetCollisionPosition() const override;
	void SetCollisionPosition(const DirectX::XMFLOAT3& position) override;
	CollisionBodyDefinition GetCollisionBodyDefinition() const override;

protected:
	const SkinnedMeshActorDefinition& GetSkinnedMeshDefinition() const final;
	virtual const SolidSkinnedMeshActorDefinition& GetSolidSkinnedMeshDefinition() const = 0;
};
