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
	void UpdateLocalBounds(const std::vector<Vertex>& vertices);

	const Ground* m_ground{};
	VertexBuffer m_vertexBuffer;
	DirectX::XMFLOAT3 m_position{ 0.0f, 0.0f, 0.0f };
	float m_localMinX{};
	float m_localMinY{};
	float m_localMinZ{};
	float m_localMaxX{};
	float m_localMaxY{};
	float m_localMaxZ{};
	float m_collisionRadius{};
	float m_verticalVelocity{};
	bool m_isGrounded{};
};
