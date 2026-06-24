#include "Gameplay/CollisionBody.h"

#include <cmath>

using namespace DirectX;

namespace
{
	constexpr float Epsilon = 0.0001f;
}

bool CollisionBody::IsCollisionBodyEnabled() const
{
	return GetCollisionBodyDefinition().enabled;
}

float CollisionBody::GetCollisionRadius() const
{
	return GetCollisionBodyDefinition().radius;
}

float CollisionBody::GetCollisionBottomY() const
{
	return GetCollisionPosition().y;
}

float CollisionBody::GetCollisionTopY() const
{
	return GetCollisionBottomY() + GetCollisionBodyDefinition().height;
}

bool ResolveCollisionBodyAgainst(
	CollisionBody& movable,
	const CollisionBody& blocker)
{
	if (!movable.IsCollisionBodyEnabled() || !blocker.IsCollisionBodyEnabled())
	{
		return false;
	}

	if (movable.GetCollisionBottomY() >= blocker.GetCollisionTopY() ||
		movable.GetCollisionTopY() <= blocker.GetCollisionBottomY())
	{
		return false;
	}

	XMFLOAT3 movablePosition = movable.GetCollisionPosition();
	const XMFLOAT3 blockerPosition = blocker.GetCollisionPosition();
	const float combinedRadius = movable.GetCollisionRadius() + blocker.GetCollisionRadius();
	const float deltaX = movablePosition.x - blockerPosition.x;
	const float deltaZ = movablePosition.z - blockerPosition.z;
	const float distanceSquared = deltaX * deltaX + deltaZ * deltaZ;

	if (distanceSquared >= combinedRadius * combinedRadius)
	{
		return false;
	}

	if (distanceSquared <= Epsilon)
	{
		movablePosition.x = blockerPosition.x + combinedRadius;
		movable.SetCollisionPosition(movablePosition);
		return true;
	}

	const float distance = std::sqrt(distanceSquared);
	const float pushDistance = combinedRadius - distance;
	movablePosition.x += (deltaX / distance) * pushDistance;
	movablePosition.z += (deltaZ / distance) * pushDistance;
	movable.SetCollisionPosition(movablePosition);
	return true;
}
