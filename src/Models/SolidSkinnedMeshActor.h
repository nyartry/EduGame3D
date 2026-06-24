#pragma once

#include "Gameplay/CollisionBody.h"
#include "Models/SkinnedMeshActor.h"

#include <string_view>

enum class SkinnedCollisionAnchorMode
{
	ActorOrigin,
	RootMotionBone,
	NamedBone,
	AnimatedBounds,
};

struct SkinnedCollisionAnchorDefinition
{
	SkinnedCollisionAnchorMode mode{ SkinnedCollisionAnchorMode::ActorOrigin };
	std::string_view boneName{};
};

struct SolidSkinnedMeshActorDefinition
{
	SkinnedMeshActorDefinition mesh;
	CollisionBodyDefinition collision;
	SkinnedCollisionAnchorDefinition collisionAnchor;
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

private:
	DirectX::XMFLOAT3 ResolveCollisionAnchorLocal() const;
};
