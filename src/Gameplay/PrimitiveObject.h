#pragma once

#include "Gameplay/CollisionBody.h"
#include "Gameplay/CollisionComponents.h"
#include "Rendering/Geometry/Vertex.h"
#include "Rendering/Buffers/VertexBuffer.h"

#include <Windows.h>

#include <DirectXMath.h>
#include <vector>

class Dx12Renderer;
class Ground;
struct ID3D12Device;

class PrimitiveObject : public CollisionBody
{
public:
	virtual ~PrimitiveObject() = default;

	void Initialize(ID3D12Device* device);
	void Update(float deltaTime);
	void Draw(Dx12Renderer& renderer) const;

	void SetPosition(float x, float y, float z);
	DirectX::XMFLOAT3 GetCollisionPosition() const override;
	void SetCollisionPosition(const DirectX::XMFLOAT3& position) override;
	CollisionBodyDefinition GetCollisionBodyDefinition() const override;
	float GetCollisionBottomY() const override;
	float GetCollisionTopY() const override;
	void SetGround(const Ground* ground);
	void SetGravityEnabled(bool enabled);
	bool IsGravityEnabled() const;
	void SetGroundCollisionEnabled(bool enabled);
	bool IsGroundCollisionEnabled() const;
	void SetSurfaceCollisionEnabled(bool enabled);
	bool IsSurfaceCollisionEnabled() const;
	DirectX::XMFLOAT3 GetPosition() const;
	bool TryGetTopSurfaceAt(const DirectX::XMFLOAT3& position, float radius, float& height) const;
	bool IsGrounded() const;

protected:
	virtual std::vector<Vertex> BuildVertices() const = 0;

private:
	void ApplyGravity(float deltaTime);
	void ResolveGroundCollision();

	PrimitiveObjectCollider m_collider;
	PrimitiveGroundCollision m_groundCollision;
	VertexBuffer m_vertexBuffer;
	DirectX::XMFLOAT3 m_position{ 0.0f, 0.0f, 0.0f };
	float m_verticalVelocity{};
	bool m_isGrounded{};
	bool m_gravityEnabled{ true };
};
