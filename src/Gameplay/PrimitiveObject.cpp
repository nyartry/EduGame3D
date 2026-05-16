#include "Gameplay/PrimitiveObject.h"

#include "Gameplay/Ground.h"
#include "Rendering/Dx12Renderer.h"

#include <algorithm>
#include <cmath>

using namespace DirectX;

namespace
{
	constexpr float PrimitiveGravity = -18.0f;

	float CalculateCollisionRadius(const Vertex& vertex)
	{
		return std::sqrt(
			vertex.position[0] * vertex.position[0] +
			vertex.position[2] * vertex.position[2]);
	}
}

void PrimitiveObject::Initialize(ID3D12Device* device)
{
	const std::vector<Vertex> vertices = BuildVertices();
	UpdateLocalBounds(vertices);
	m_vertexBuffer.Initialize(device, vertices);
}

void PrimitiveObject::Update(float deltaTime)
{
	ApplyGravity(deltaTime);
	ResolveGroundCollision();
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
	if (!ContainsXZ(position, radius))
	{
		return false;
	}

	height = GetTopY();
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

void PrimitiveObject::ApplyGravity(float deltaTime)
{
	m_verticalVelocity += PrimitiveGravity * deltaTime;
	m_position.y += m_verticalVelocity * deltaTime;
}

void PrimitiveObject::ResolveGroundCollision()
{
	if (m_ground == nullptr)
	{
		return;
	}

	float groundHeight = 0.0f;
	if (!m_ground->TryGetHeightAt(m_position, m_localBounds.collisionRadius, groundHeight))
	{
		m_isGrounded = false;
		return;
	}

	const float bottomY = GetBottomY();
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

bool PrimitiveObject::ContainsXZ(const XMFLOAT3& position, float radius) const
{
	const float worldMinX = m_position.x + m_localBounds.minX;
	const float worldMaxX = m_position.x + m_localBounds.maxX;
	const float worldMinZ = m_position.z + m_localBounds.minZ;
	const float worldMaxZ = m_position.z + m_localBounds.maxZ;

	return position.x + radius >= worldMinX &&
		position.x - radius <= worldMaxX &&
		position.z + radius >= worldMinZ &&
		position.z - radius <= worldMaxZ;
}

float PrimitiveObject::GetBottomY() const
{
	return m_position.y + m_localBounds.minY;
}

float PrimitiveObject::GetTopY() const
{
	return m_position.y + m_localBounds.maxY;
}
