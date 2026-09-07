#pragma once

#include "Framework/Gameplay/CollisionBody.h"
#include "Framework/Physics/ICollisionQuery.h"

#include <vector>

// A small scene-owned registry, independent of rendering and concrete Game types.
// Remove registrations before destroying their surfaces/bodies; Clear on unload.
class CollisionWorld final : public ICollisionQuery
{
public:
	void RegisterSurface(const ICollisionSurface& surface, const CollisionBody* body = nullptr);
	void RegisterBody(const CollisionBody& body);
	void RemoveSurface(const ICollisionSurface& surface);
	void RemoveBody(const CollisionBody& body);
	void Clear();

	bool TryFindFloor(const FloorQuery& query, FloorHit& hit) const override;
	bool ResolveBoundaries(DirectX::XMFLOAT3& position, float radius) const override;
	bool ResolveBodyCollisions(CollisionBody& movable, int solverPassCount = 3) const;

private:
	struct Entry
	{
		const ICollisionSurface* surface{};
		const CollisionBody* body{};
	};
	std::vector<Entry> m_entries;
};
