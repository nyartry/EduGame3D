#pragma once

#include <DirectXMath.h>

struct CollisionBodyDefinition
{
	float radius{ 0.35f };
	float height{ 1.8f };
	bool enabled{ true };
};

class CollisionBody
{
public:
	virtual ~CollisionBody() = default;

	virtual DirectX::XMFLOAT3 GetCollisionPosition() const = 0;
	virtual void SetCollisionPosition(const DirectX::XMFLOAT3& position) = 0;
	virtual CollisionBodyDefinition GetCollisionBodyDefinition() const = 0;

	bool IsCollisionBodyEnabled() const;
	float GetCollisionRadius() const;
	virtual float GetCollisionBottomY() const;
	virtual float GetCollisionTopY() const;
};

bool ResolveCollisionBodyAgainst(
	CollisionBody& movable,
	const CollisionBody& blocker);
