#include "Game/Gameplay/Ground.h"

#include "Framework/Rendering/Core/Dx12Renderer.h"

using namespace DirectX;

namespace
{
	constexpr std::array<float, 4> WallColor{ 0.30f, 0.24f, 0.18f, 1.0f };

	Vertex MakeVertex(float x, float y, float z, const std::array<float, 4>& color)
	{
		return Vertex{ { x, y, z }, { color[0], color[1], color[2], color[3] } };
	}

	void AddFace(
		std::vector<Vertex>& vertices,
		const Vertex& v0,
		const Vertex& v1,
		const Vertex& v2,
		const Vertex& v3)
	{
		vertices.push_back(v0);
		vertices.push_back(v1);
		vertices.push_back(v2);
		vertices.push_back(v0);
		vertices.push_back(v2);
		vertices.push_back(v3);
	}

	void AddBox(
		std::vector<Vertex>& vertices,
		float minX,
		float minY,
		float minZ,
		float maxX,
		float maxY,
		float maxZ,
		const std::array<float, 4>& color)
	{
		const Vertex leftBottomBack = MakeVertex(minX, minY, minZ, color);
		const Vertex rightBottomBack = MakeVertex(maxX, minY, minZ, color);
		const Vertex rightTopBack = MakeVertex(maxX, maxY, minZ, color);
		const Vertex leftTopBack = MakeVertex(minX, maxY, minZ, color);

		const Vertex leftBottomFront = MakeVertex(minX, minY, maxZ, color);
		const Vertex rightBottomFront = MakeVertex(maxX, minY, maxZ, color);
		const Vertex rightTopFront = MakeVertex(maxX, maxY, maxZ, color);
		const Vertex leftTopFront = MakeVertex(minX, maxY, maxZ, color);

		AddFace(vertices, leftBottomFront, rightBottomFront, rightTopFront, leftTopFront);
		AddFace(vertices, rightBottomBack, leftBottomBack, leftTopBack, rightTopBack);
		AddFace(vertices, leftBottomBack, leftBottomFront, leftTopFront, leftTopBack);
		AddFace(vertices, rightBottomFront, rightBottomBack, rightTopBack, rightTopFront);
		AddFace(vertices, leftTopFront, rightTopFront, rightTopBack, leftTopBack);
		AddFace(vertices, leftBottomBack, rightBottomBack, rightBottomFront, leftBottomFront);
	}
}

void Ground::Initialize(ID3D12Device* device)
{
	BuildMesh();
	m_vertexBuffer.Initialize(device, m_vertices);
}

void Ground::Draw(Dx12Renderer& renderer) const
{
	renderer.Draw(m_vertexBuffer, XMMatrixIdentity());
}

bool Ground::TryGetHeightAt(const XMFLOAT3& position, float radius, float& height) const
{
	return m_collider.TryGetHeightAt(position, radius, height);
}

bool Ground::ResolveWallCollision(XMFLOAT3& position, float radius) const
{
	return m_wallCollider.ResolveInsideBounds(position, radius);
}

void Ground::SetCollisionEnabled(bool enabled)
{
	m_collider.SetEnabled(enabled);
	m_wallCollider.SetEnabled(enabled);
}

bool Ground::IsCollisionEnabled() const
{
	return m_collider.IsEnabled();
}

void Ground::BuildMesh()
{
	constexpr int TileCount = 16;
	constexpr float TileSize = (HalfExtent * 2.0f) / static_cast<float>(TileCount);

	m_vertices.clear();
	m_vertices.reserve(TileCount * TileCount * 6 + 4 * 36);

	for (int z = 0; z < TileCount; ++z)
	{
		for (int x = 0; x < TileCount; ++x)
		{
			const float left = -HalfExtent + static_cast<float>(x) * TileSize;
			const float right = left + TileSize;
			const float nearZ = -HalfExtent + static_cast<float>(z) * TileSize;
			const float farZ = nearZ + TileSize;
			const bool alternate = ((x + z) % 2) == 0;
			const std::array<float, 4> color = alternate
				? std::array<float, 4>{ 0.23f, 0.52f, 0.24f, 1.0f }
			: std::array<float, 4>{ 0.17f, 0.39f, 0.20f, 1.0f };

			const Vertex v0{ { left, 0.0f, farZ }, { color[0], color[1], color[2], color[3] } };
			const Vertex v1{ { right, 0.0f, farZ }, { color[0], color[1], color[2], color[3] } };
			const Vertex v2{ { right, 0.0f, nearZ }, { color[0], color[1], color[2], color[3] } };
			const Vertex v3{ { left, 0.0f, nearZ }, { color[0], color[1], color[2], color[3] } };

			m_vertices.push_back(v0);
			m_vertices.push_back(v1);
			m_vertices.push_back(v2);
			m_vertices.push_back(v0);
			m_vertices.push_back(v2);
			m_vertices.push_back(v3);
		}
	}

	constexpr float outer = HalfExtent + WallThickness;
	AddBox(m_vertices, -outer, GroundHeight, HalfExtent, outer, WallHeight, outer, WallColor);
	AddBox(m_vertices, -outer, GroundHeight, -outer, outer, WallHeight, -HalfExtent, WallColor);
	AddBox(m_vertices, -outer, GroundHeight, -HalfExtent, -HalfExtent, WallHeight, HalfExtent, WallColor);
	AddBox(m_vertices, HalfExtent, GroundHeight, -HalfExtent, outer, WallHeight, HalfExtent, WallColor);
}
