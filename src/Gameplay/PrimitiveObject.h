#pragma once

#include "Rendering/Vertex.h"
#include "Rendering/VertexBuffer.h"

#include <Windows.h>

#include <DirectXMath.h>
#include <vector>

class Dx12Renderer;
class Ground;
struct ID3D12Device;

class PrimitiveObject
{
public:
	virtual ~PrimitiveObject() = default;

	void Initialize(ID3D12Device* device);
	void Update(float deltaTime);
	void Draw(Dx12Renderer& renderer) const;

	void SetPosition(float x, float y, float z);
	void SetGround(const Ground* ground);
	DirectX::XMFLOAT3 GetPosition() const;
	bool TryGetTopSurfaceAt(const DirectX::XMFLOAT3& position, float radius, float& height) const;
	bool IsGrounded() const;

protected:
	virtual std::vector<Vertex> BuildVertices() const = 0;

private:
	struct LocalBounds
	{
		float minX{};
		float minY{};
		float minZ{};
		float maxX{};
		float maxY{};
		float maxZ{};
		float collisionRadius{};
	};

	void UpdateLocalBounds(const std::vector<Vertex>& vertices);
	void ApplyGravity(float deltaTime);
	void ResolveGroundCollision();
	bool ContainsXZ(const DirectX::XMFLOAT3& position, float radius) const;
	float GetBottomY() const;
	float GetTopY() const;

	const Ground* m_ground{};
	VertexBuffer m_vertexBuffer;
	DirectX::XMFLOAT3 m_position{ 0.0f, 0.0f, 0.0f };
	LocalBounds m_localBounds;
	float m_verticalVelocity{};
	bool m_isGrounded{};
};
