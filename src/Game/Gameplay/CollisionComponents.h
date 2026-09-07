#pragma once

#include "Framework/Rendering/Geometry/Vertex.h"
#include "Framework/Core/Math/Aabb.h"
#include "Framework/Physics/CollisionWorld.h"

#include <DirectXMath.h>
#include <vector>

class CollisionSwitch
{
public:
	void SetEnabled(bool enabled);
	bool IsEnabled() const;

private:
	bool m_enabled{ true };
};

class GroundHeightCollider
{
public:
	GroundHeightCollider(float height, float halfExtent);

	void SetEnabled(bool enabled);
	bool IsEnabled() const;
	bool TryGetHeightAt(const DirectX::XMFLOAT3& position, float radius, float& height) const;

private:
	CollisionSwitch m_switch;
	float m_height{};
	float m_halfExtent{};
};

class GroundBoundaryCollider
{
public:
	explicit GroundBoundaryCollider(float halfExtent);

	void SetEnabled(bool enabled);
	bool IsEnabled() const;
	bool ResolveInsideBounds(DirectX::XMFLOAT3& position, float radius) const;

private:
	CollisionSwitch m_switch;
	float m_halfExtent{};
};

class PrimitiveObjectCollider
{
public:
	void SetEnabled(bool enabled);
	bool IsEnabled() const;
	void RebuildFromVertices(const std::vector<Vertex>& vertices);

	bool TryGetTopSurfaceAt(
		const DirectX::XMFLOAT3& objectPosition,
		const DirectX::XMFLOAT3& queryPosition,
		float queryRadius,
		float& height) const;

	float GetBottomY(const DirectX::XMFLOAT3& objectPosition) const;
	float GetTopY(const DirectX::XMFLOAT3& objectPosition) const;
	float GetCollisionRadius() const;

private:
	bool ContainsXZ(
		const DirectX::XMFLOAT3& objectPosition,
		const DirectX::XMFLOAT3& queryPosition,
		float queryRadius) const;

	CollisionSwitch m_switch;
	Aabb m_localBounds;
	float m_collisionRadius{};
};

class PrimitiveGroundCollision
{
public:
	void SetEnabled(bool enabled);
	bool IsEnabled() const;
	void SetGround(const ICollisionSurface* ground);
	void SetCollisionQuery(const ICollisionQuery* query);

	bool Resolve(
		DirectX::XMFLOAT3& position,
		float& verticalVelocity,
		const PrimitiveObjectCollider& collider,
		float previousBottomY,
		const ICollisionSurface* ignoredSurface = nullptr) const;

private:
	CollisionSwitch m_switch;
	const ICollisionQuery* m_query{};
	CollisionWorld m_localWorld;
};
