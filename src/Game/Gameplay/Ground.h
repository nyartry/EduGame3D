#pragma once

#include "Game/Gameplay/CollisionComponents.h"
#include "Framework/Rendering/Buffers/VertexBuffer.h"
#include "Framework/Rendering/Geometry/Vertex.h"

#include <array>
#include <DirectXMath.h>
#include <vector>

class IRenderDevice;
class IRenderer;

class Ground
{
public:
	void Initialize(IRenderDevice& device);
	void Draw(IRenderer& renderer) const;
	bool TryGetHeightAt(const DirectX::XMFLOAT3& position, float radius, float& height) const;
	bool ResolveWallCollision(DirectX::XMFLOAT3& position, float radius) const;
	void SetCollisionEnabled(bool enabled);
	bool IsCollisionEnabled() const;

private:
	void BuildMesh();

	static constexpr float GroundHeight = 0.0f;
	static constexpr float HalfExtent = 8.0f;
	static constexpr float WallHeight = 1.6f;
	static constexpr float WallThickness = 0.35f;

	GroundHeightCollider m_collider{ GroundHeight, HalfExtent };
	GroundBoundaryCollider m_wallCollider{ HalfExtent };
	VertexBuffer m_vertexBuffer;
	std::vector<Vertex> m_vertices;
};
