#include "Gameplay/PrimitiveObject.h"

#include "Gameplay/Ground.h"
#include "Rendering/Dx12Renderer.h"

#include <algorithm>
#include <cmath>

using namespace DirectX;

namespace
{
	constexpr float Gravity = -18.0f;
}

void PrimitiveObject::Initialize(ID3D12Device* device)
{
	const std::vector<Vertex> vertices = BuildVertices();
	UpdateLocalBounds(vertices);
	m_vertexBuffer.Initialize(device, vertices);
}

void PrimitiveObject::Update(float deltaTime)
{
	m_verticalVelocity += Gravity * deltaTime;
	m_position.y += m_verticalVelocity * deltaTime;

	if (m_ground == nullptr)
	{
		return;
	}

	float groundHeight = 0.0f;
	if (!m_ground->TryGetHeightAt(m_position, m_collisionRadius, groundHeight))
	{
		m_isGrounded = false;
		return;
	}

	const float bottomY = m_position.y + m_localMinY;
	if (bottomY <= groundHeight)
	{
		m_position.y += groundHeight - bottomY;
		m_verticalVelocity = 0.0f;
		m_isGrounded = true;
	}
	else
	{
		m_isGrounded = false;
	}
}

void PrimitiveObject::Draw(Dx12Renderer& renderer) const
{
	const XMMATRIX world = XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
	renderer.Draw(m_vertexBuffer, world);
}

void PrimitiveObject::SetPosition(float x, float y, float z)
{
	m_position = XMFLOAT3{ x, y, z };
}

void PrimitiveObject::SetGround(const Ground* ground)
{
	m_ground = ground;
}

XMFLOAT3 PrimitiveObject::GetPosition() const
{
	return m_position;
}

bool PrimitiveObject::TryGetTopSurfaceAt(const XMFLOAT3& position, float radius, float& height) const
{
	const float worldMinX = m_position.x + m_localMinX;
	const float worldMaxX = m_position.x + m_localMaxX;
	const float worldMinZ = m_position.z + m_localMinZ;
	const float worldMaxZ = m_position.z + m_localMaxZ;

	if (position.x + radius < worldMinX ||
		position.x - radius > worldMaxX ||
		position.z + radius < worldMinZ ||
		position.z - radius > worldMaxZ)
	{
		return false;
	}

	height = m_position.y + m_localMaxY;
	return true;
}

bool PrimitiveObject::IsGrounded() const
{
	return m_isGrounded;
}

void PrimitiveObject::UpdateLocalBounds(const std::vector<Vertex>& vertices)
{
	if (vertices.empty())
	{
		m_localMinX = 0.0f;
		m_localMinY = 0.0f;
		m_localMinZ = 0.0f;
		m_localMaxX = 0.0f;
		m_localMaxY = 0.0f;
		m_localMaxZ = 0.0f;
		m_collisionRadius = 0.0f;
		return;
	}

	m_localMinX = vertices.front().position[0];
	m_localMinY = vertices.front().position[1];
	m_localMinZ = vertices.front().position[2];
	m_localMaxX = vertices.front().position[0];
	m_localMaxY = vertices.front().position[1];
	m_localMaxZ = vertices.front().position[2];
	m_collisionRadius = 0.0f;
	for (const Vertex& vertex : vertices)
	{
		m_localMinX = std::min(m_localMinX, vertex.position[0]);
		m_localMinY = std::min(m_localMinY, vertex.position[1]);
		m_localMinZ = std::min(m_localMinZ, vertex.position[2]);
		m_localMaxX = std::max(m_localMaxX, vertex.position[0]);
		m_localMaxY = std::max(m_localMaxY, vertex.position[1]);
		m_localMaxZ = std::max(m_localMaxZ, vertex.position[2]);
		m_collisionRadius = std::max(
			m_collisionRadius,
			std::sqrt(vertex.position[0] * vertex.position[0] + vertex.position[2] * vertex.position[2]));
	}
}
