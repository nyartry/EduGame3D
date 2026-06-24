#include "Gameplay/CollisionComponents.h"

#include "Gameplay/Ground.h"

#include <algorithm>
#include <cmath>

using namespace DirectX;

namespace
{
	float CalculateCollisionRadius(const Vertex& vertex)
	{
		return std::sqrt(
			vertex.position[0] * vertex.position[0] +
			vertex.position[2] * vertex.position[2]);
	}
}

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

	const float minCenter = -m_halfExtent + radius;
	const float maxCenter = m_halfExtent - radius;
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
	if (vertices.empty())
	{
		m_localBounds = LocalBounds{};
		return;
	}

	m_localBounds.minX = vertices.front().position[0];
	m_localBounds.minY = vertices.front().position[1];
	m_localBounds.minZ = vertices.front().position[2];
	m_localBounds.maxX = vertices.front().position[0];
	m_localBounds.maxY = vertices.front().position[1];
	m_localBounds.maxZ = vertices.front().position[2];
	m_localBounds.collisionRadius = 0.0f;
	for (const Vertex& vertex : vertices)
	{
		m_localBounds.minX = std::min(m_localBounds.minX, vertex.position[0]);
		m_localBounds.minY = std::min(m_localBounds.minY, vertex.position[1]);
		m_localBounds.minZ = std::min(m_localBounds.minZ, vertex.position[2]);
		m_localBounds.maxX = std::max(m_localBounds.maxX, vertex.position[0]);
		m_localBounds.maxY = std::max(m_localBounds.maxY, vertex.position[1]);
		m_localBounds.maxZ = std::max(m_localBounds.maxZ, vertex.position[2]);
		m_localBounds.collisionRadius = std::max(m_localBounds.collisionRadius, CalculateCollisionRadius(vertex));
	}
}

bool PrimitiveObjectCollider::TryGetTopSurfaceAt(
	const XMFLOAT3& objectPosition,
	const XMFLOAT3& queryPosition,
	float queryRadius,
	float& height) const
{
	if (!m_switch.IsEnabled() || !ContainsXZ(objectPosition, queryPosition, queryRadius))
	{
		return false;
	}

	height = GetTopY(objectPosition);
	return true;
}

float PrimitiveObjectCollider::GetBottomY(const XMFLOAT3& objectPosition) const
{
	return objectPosition.y + m_localBounds.minY;
}

float PrimitiveObjectCollider::GetTopY(const XMFLOAT3& objectPosition) const
{
	return objectPosition.y + m_localBounds.maxY;
}

float PrimitiveObjectCollider::GetCollisionRadius() const
{
	return m_localBounds.collisionRadius;
}

bool PrimitiveObjectCollider::ContainsXZ(
	const XMFLOAT3& objectPosition,
	const XMFLOAT3& queryPosition,
	float queryRadius) const
{
	const float worldMinX = objectPosition.x + m_localBounds.minX;
	const float worldMaxX = objectPosition.x + m_localBounds.maxX;
	const float worldMinZ = objectPosition.z + m_localBounds.minZ;
	const float worldMaxZ = objectPosition.z + m_localBounds.maxZ;

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

void PrimitiveGroundCollision::SetGround(const Ground* ground)
{
	m_ground = ground;
}

bool PrimitiveGroundCollision::Resolve(
	XMFLOAT3& position,
	float& verticalVelocity,
	const PrimitiveObjectCollider& collider) const
{
	if (!m_switch.IsEnabled() || m_ground == nullptr)
	{
		return false;
	}

	float groundHeight = 0.0f;
	if (!m_ground->TryGetHeightAt(position, collider.GetCollisionRadius(), groundHeight))
	{
		return false;
	}

	const float bottomY = collider.GetBottomY(position);
	if (bottomY > groundHeight)
	{
		return false;
	}

	position.y += groundHeight - bottomY;
	verticalVelocity = 0.0f;
	return true;
}
