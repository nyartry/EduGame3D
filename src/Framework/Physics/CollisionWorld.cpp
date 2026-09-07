#include "Framework/Physics/CollisionWorld.h"

#include <algorithm>
#include <cmath>

void CollisionWorld::RegisterSurface(const ICollisionSurface& surface, const CollisionBody* body)
{
	for (Entry& entry : m_entries)
	{
		if (entry.surface == &surface || (body != nullptr && entry.body == body))
		{
			entry.surface = &surface;
			if (body != nullptr) entry.body = body;
			return;
		}
	}
	m_entries.push_back({ &surface, body });
}

void CollisionWorld::RegisterBody(const CollisionBody& body)
{
	if (std::none_of(m_entries.begin(), m_entries.end(), [&body](const Entry& entry) { return entry.body == &body; }))
	{
		m_entries.push_back({ nullptr, &body });
	}
}

void CollisionWorld::RemoveSurface(const ICollisionSurface& surface)
{
	std::erase_if(m_entries, [&surface](const Entry& entry) { return entry.surface == &surface; });
}

void CollisionWorld::RemoveBody(const CollisionBody& body)
{
	std::erase_if(m_entries, [&body](const Entry& entry) { return entry.body == &body; });
}

void CollisionWorld::Clear()
{
	m_entries.clear();
}

bool CollisionWorld::TryFindFloor(const FloorQuery& query, FloorHit& hit) const
{
	hit = {};
	if (!std::isfinite(query.position.x) || !std::isfinite(query.position.y) || !std::isfinite(query.position.z) ||
		!std::isfinite(query.radius) || query.radius < 0.0f ||
		!std::isfinite(query.previousBottomY) || !std::isfinite(query.currentBottomY) ||
		!std::isfinite(query.landingTolerance) || query.landingTolerance < 0.0f)
	{
		return false;
	}
	for (const Entry& entry : m_entries)
	{
		float height = 0.0f;
		if (entry.surface == nullptr || entry.surface == query.ignoredSurface ||
			!entry.surface->TryGetHeightAt(query.position, query.radius, height) || !std::isfinite(height))
		{
			continue;
		}
		if (entry.surface->IsOneWayFloor() &&
			(query.previousBottomY < height - query.landingTolerance || query.currentBottomY > height))
		{
			continue;
		}
		if (hit.surface == nullptr || height > hit.height)
		{
			hit = { height, entry.surface };
		}
	}
	return hit.surface != nullptr;
}

bool CollisionWorld::ResolveBoundaries(DirectX::XMFLOAT3& position, float radius) const
{
	if (!std::isfinite(position.x) || !std::isfinite(position.z) || !std::isfinite(radius) || radius < 0.0f)
	{
		return false;
	}
	bool resolved = false;
	for (const Entry& entry : m_entries)
	{
		if (entry.surface != nullptr) resolved |= entry.surface->ResolveBoundaryCollision(position, radius);
	}
	return resolved;
}

bool CollisionWorld::ResolveBodyCollisions(CollisionBody& movable, int solverPassCount) const
{
	if (!movable.IsCollisionBodyEnabled()) return false;
	bool resolved = false;
	for (int pass = 0; pass < solverPassCount; ++pass)
	{
		bool resolvedPass = false;
		for (const Entry& entry : m_entries)
		{
			if (entry.body != nullptr && entry.body != &movable)
			{
				resolvedPass |= ResolveCollisionBodyAgainst(movable, *entry.body);
			}
		}
		DirectX::XMFLOAT3 position = movable.GetCollisionPosition();
		if (ResolveBoundaries(position, movable.GetCollisionRadius()))
		{
			movable.SetCollisionPosition(position);
			resolvedPass = true;
		}
		resolved |= resolvedPass;
		if (!resolvedPass) break;
	}
	return resolved;
}
