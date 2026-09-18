#pragma once

#include <DirectXMath.h>

// Surfaces own their geometry and enabled state; queries never retain a hit past
// the caller's update. Registered objects must outlive their world registration.
class ICollisionSurface
{
public:
	virtual ~ICollisionSurface() = default;
	virtual bool TryGetHeightAt(const DirectX::XMFLOAT3& position, float radius, float& height) const = 0;
	// One-way platforms catch a descending foot; solid ground also corrects a
	// foot already below the floor (the existing spawn/ground recovery policy).
	virtual bool IsOneWayFloor() const { return true; }
	virtual bool ResolveBoundaryCollision(DirectX::XMFLOAT3&, float) const { return false; }
};

struct FloorQuery
{
	DirectX::XMFLOAT3 position{};
	float radius{};
	float previousBottomY{};
	float currentBottomY{};
	float landingTolerance{ 0.05f };
	const ICollisionSurface* ignoredSurface{};
};

struct FloorHit
{
	float height{};
	const ICollisionSurface* surface{};
};

class ICollisionQuery
{
public:
	virtual ~ICollisionQuery() = default;
	virtual bool TryFindFloor(const FloorQuery& query, FloorHit& hit) const = 0;
	virtual bool ResolveBoundaries(DirectX::XMFLOAT3& position, float radius) const = 0;
};
