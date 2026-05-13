#pragma once

#include "Rendering/Vertex.h"
#include "Rendering/VertexBuffer.h"

#include <Windows.h>

#include <DirectXMath.h>
#include <vector>

class Dx12Renderer;
struct ID3D12Device;

class PrimitiveObject
{
public:
	virtual ~PrimitiveObject() = default;

	void Initialize(ID3D12Device* device);
	void Draw(Dx12Renderer& renderer) const;

	void SetPosition(float x, float y, float z);
	DirectX::XMFLOAT3 GetPosition() const;

protected:
	virtual std::vector<Vertex> BuildVertices() const = 0;

private:
	VertexBuffer m_vertexBuffer;
	DirectX::XMFLOAT3 m_position{ 0.0f, 0.0f, 0.0f };
};
