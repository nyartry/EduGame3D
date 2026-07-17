#include "Game/Gameplay/PrimitiveObject.h"

#include "Game/Gameplay/Ground.h"
#include "Framework/Rendering/Core/IRenderDevice.h"
#include "Framework/Rendering/Core/IRenderer.h"

using namespace DirectX;

namespace
{
	constexpr float PrimitiveGravity = -18.0f;
}

void PrimitiveObject::Initialize(IRenderDevice& device)
{
	const std::vector<Vertex> vertices = BuildVertices();
	m_collider.RebuildFromVertices(vertices);
	device.CreateVertexBuffer(m_vertexBuffer, vertices);
}

void PrimitiveObject::Update(float deltaTime)
{
	ApplyGravity(deltaTime);
	ResolveGroundCollision();
}

void PrimitiveObject::Draw(IRenderer& renderer) const
{
	const XMMATRIX world = XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
	renderer.Draw(m_vertexBuffer, world);
}

void PrimitiveObject::SetPosition(float x, float y, float z)
{
	m_position = XMFLOAT3{ x, y, z };
}

XMFLOAT3 PrimitiveObject::GetCollisionPosition() const
{
	return m_position;
}

void PrimitiveObject::SetCollisionPosition(const XMFLOAT3& position)
{
	m_position = position;
}

CollisionBodyDefinition PrimitiveObject::GetCollisionBodyDefinition() const
{
	return CollisionBodyDefinition
	{
		m_collider.GetCollisionRadius(),
		GetCollisionTopY() - GetCollisionBottomY(),
		m_collider.IsEnabled()
	};
}

float PrimitiveObject::GetCollisionBottomY() const
{
	return m_collider.GetBottomY(m_position);
}

float PrimitiveObject::GetCollisionTopY() const
{
	return m_collider.GetTopY(m_position);
}

void PrimitiveObject::SetGround(const Ground* ground)
{
	m_groundCollision.SetGround(ground);
}

void PrimitiveObject::SetGravityEnabled(bool enabled)
{
	m_gravityEnabled = enabled;
	if (!m_gravityEnabled)
	{
		m_verticalVelocity = 0.0f;
	}
}

bool PrimitiveObject::IsGravityEnabled() const
{
	return m_gravityEnabled;
}

void PrimitiveObject::SetGroundCollisionEnabled(bool enabled)
{
	m_groundCollision.SetEnabled(enabled);
	if (!enabled)
	{
		m_isGrounded = false;
	}
}

bool PrimitiveObject::IsGroundCollisionEnabled() const
{
	return m_groundCollision.IsEnabled();
}

void PrimitiveObject::SetSurfaceCollisionEnabled(bool enabled)
{
	m_collider.SetEnabled(enabled);
}

bool PrimitiveObject::IsSurfaceCollisionEnabled() const
{
	return m_collider.IsEnabled();
}

XMFLOAT3 PrimitiveObject::GetPosition() const
{
	return m_position;
}

bool PrimitiveObject::TryGetTopSurfaceAt(const XMFLOAT3& position, float radius, float& height) const
{
	return m_collider.TryGetTopSurfaceAt(m_position, position, radius, height);
}

bool PrimitiveObject::IsGrounded() const
{
	return m_isGrounded;
}

void PrimitiveObject::ApplyGravity(float deltaTime)
{
	if (!m_gravityEnabled)
	{
		m_verticalVelocity = 0.0f;
		return;
	}

	m_verticalVelocity += PrimitiveGravity * deltaTime;
	m_position.y += m_verticalVelocity * deltaTime;
}

void PrimitiveObject::ResolveGroundCollision()
{
	m_isGrounded = m_groundCollision.Resolve(m_position, m_verticalVelocity, m_collider);
}
