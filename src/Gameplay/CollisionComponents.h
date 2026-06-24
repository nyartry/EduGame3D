#pragma once

#include "Rendering/Geometry/Vertex.h"

#include <DirectXMath.h>
#include <vector>

class Ground;

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
	struct LocalBounds
	{
		float minX{};
		float minY{};
		float minZ{};
		float maxX{};
		float maxY{};
		float maxZ{};
		float collisionRadius{};
	};

	bool ContainsXZ(
		const DirectX::XMFLOAT3& objectPosition,
		const DirectX::XMFLOAT3& queryPosition,
		float queryRadius) const;

	CollisionSwitch m_switch;
	LocalBounds m_localBounds;
};

class PrimitiveGroundCollision
{
public:
	void SetEnabled(bool enabled);
	bool IsEnabled() const;
	void SetGround(const Ground* ground);

	bool Resolve(
		DirectX::XMFLOAT3& position,
		float& verticalVelocity,
		const PrimitiveObjectCollider& collider) const;

private:
	CollisionSwitch m_switch;
	const Ground* m_ground{};
};
