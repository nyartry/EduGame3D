#include "Gameplay/Ground.h"

#include "Rendering/Dx12Renderer.h"

using namespace DirectX;

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

void Ground::SetCollisionEnabled(bool enabled)
{
	m_collider.SetEnabled(enabled);
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
	m_vertices.reserve(TileCount * TileCount * 6);

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
}
