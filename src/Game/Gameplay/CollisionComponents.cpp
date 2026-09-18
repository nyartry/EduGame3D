#include "Game/Gameplay/CollisionComponents.h"

#include "Framework/Core/Math/MathUtils.h"

#include <algorithm>
#include <cmath>

using namespace DirectX;

void CollisionSwitch::SetEnabled(bool enabled)
{
	m_enabled = enabled;
}

bool CollisionSwitch::IsEnabled() const
{
	return m_enabled;
}

GroundHeightCollider::GroundHeightCollider(float height, float halfExtent)
	: m_height(height)
	, m_halfExtent(halfExtent)
{
}

void GroundHeightCollider::SetEnabled(bool enabled)
{
	m_switch.SetEnabled(enabled);
}

bool GroundHeightCollider::IsEnabled() const
{
	return m_switch.IsEnabled();
}

bool GroundHeightCollider::TryGetHeightAt(const XMFLOAT3& position, float radius, float& height) const
{
	if (!m_switch.IsEnabled())
	{
		return false;
	}

	if (position.x + radius < -m_halfExtent ||
		position.x - radius > m_halfExtent ||
		position.z + radius < -m_halfExtent ||
		position.z - radius > m_halfExtent)
	{
		return false;
	}

	height = m_height;
	return true;
}

GroundBoundaryCollider::GroundBoundaryCollider(float halfExtent)
	: m_halfExtent(halfExtent)
{
}

void GroundBoundaryCollider::SetEnabled(bool enabled)
{
	m_switch.SetEnabled(enabled);
}

bool GroundBoundaryCollider::IsEnabled() const
{
	return m_switch.IsEnabled();
}

bool GroundBoundaryCollider::ResolveInsideBounds(XMFLOAT3& position, float radius) const
{
	if (!m_switch.IsEnabled())
	{
		return false;
	}

	const float maxCenter = std::max(0.0f, m_halfExtent - radius);
	const float minCenter = -maxCenter;
	const float resolvedX = std::clamp(position.x, minCenter, maxCenter);
	const float resolvedZ = std::clamp(position.z, minCenter, maxCenter);
	const bool changed = resolvedX != position.x || resolvedZ != position.z;

	position.x = resolvedX;
	position.z = resolvedZ;
	return changed;
}

void PrimitiveObjectCollider::SetEnabled(bool enabled)
{
	m_switch.SetEnabled(enabled);
}

bool PrimitiveObjectCollider::IsEnabled() const
{
	return m_switch.IsEnabled();
}

void PrimitiveObjectCollider::RebuildFromVertices(const std::vector<Vertex>& vertices)
{
	m_localBounds = {};
	m_collisionRadius = 0.0f;
	for (const Vertex& vertex : vertices)
	{
		const XMFLOAT3 position{ vertex.position[0], vertex.position[1], vertex.position[2] };
		if (m_localBounds.AddPoint(position))
		{
			m_collisionRadius = std::max(m_collisionRadius, MathUtils::LengthXZ(position));
		}
	}
}

bool PrimitiveObjectCollider::TryGetTopSurfaceAt(
	const XMFLOAT3& objectPosition,
	const XMFLOAT3& queryPosition,
	float queryRadius,
	float& height) const
{
	if (!m_switch.IsEnabled() || m_localBounds.IsEmpty() || !ContainsXZ(objectPosition, queryPosition, queryRadius))
	{
		return false;
	}

	height = GetTopY(objectPosition);
	return true;
}

float PrimitiveObjectCollider::GetBottomY(const XMFLOAT3& objectPosition) const
{
	return objectPosition.y + m_localBounds.Min().y;
}

float PrimitiveObjectCollider::GetTopY(const XMFLOAT3& objectPosition) const
{
	return objectPosition.y + m_localBounds.Max().y;
}

float PrimitiveObjectCollider::GetCollisionRadius() const
{
	return m_collisionRadius;
}

bool PrimitiveObjectCollider::ContainsXZ(
	const XMFLOAT3& objectPosition,
	const XMFLOAT3& queryPosition,
	float queryRadius) const
{
	const float worldMinX = objectPosition.x + m_localBounds.Min().x;
	const float worldMaxX = objectPosition.x + m_localBounds.Max().x;
	const float worldMinZ = objectPosition.z + m_localBounds.Min().z;
	const float worldMaxZ = objectPosition.z + m_localBounds.Max().z;

	return queryPosition.x + queryRadius >= worldMinX &&
		queryPosition.x - queryRadius <= worldMaxX &&
		queryPosition.z + queryRadius >= worldMinZ &&
		queryPosition.z - queryRadius <= worldMaxZ;
}

void PrimitiveGroundCollision::SetEnabled(bool enabled)
{
	m_switch.SetEnabled(enabled);
}

bool PrimitiveGroundCollision::IsEnabled() const
{
	return m_switch.IsEnabled();
}

void PrimitiveGroundCollision::SetGround(const ICollisionSurface* ground)
{
	m_localWorld.Clear();
	if (ground != nullptr) m_localWorld.RegisterSurface(*ground);
}

void PrimitiveGroundCollision::SetCollisionQuery(const ICollisionQuery* query)
{
	m_query = query;
}

bool PrimitiveGroundCollision::Resolve(
	XMFLOAT3& position,
	float& verticalVelocity,
	const PrimitiveObjectCollider& collider,
	float previousBottomY,
	const ICollisionSurface* ignoredSurface) const
{
	if (!m_switch.IsEnabled())
	{
		return false;
	}

	const float bottomY = collider.GetBottomY(position);
	const ICollisionQuery& world = m_query != nullptr ? *m_query : m_localWorld;
	FloorHit hit;
	if (!world.TryFindFloor({ position, collider.GetCollisionRadius(), previousBottomY, bottomY, 0.05f, ignoredSurface }, hit))
	{
		return false;
	}

	if (bottomY > hit.height)
	{
		return false;
	}

	position.y += hit.height - bottomY;
	verticalVelocity = 0.0f;
	return true;
}
