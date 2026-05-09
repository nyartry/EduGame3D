#pragma once

#include "Vertex.h"
#include "VertexBuffer.h"

#include <Windows.h>

#include <DirectXMath.h>
#include <string>
#include <vector>

class Dx12Renderer;
struct ID3D12Device;

class Player
{
public:
	void Initialize(ID3D12Device* device, const std::string& modelPath);
	void Draw(Dx12Renderer& renderer) const;

	void SetPosition(float x, float y, float z);
	DirectX::XMFLOAT3 GetPosition() const;

private:
	void FitModelToPlayerSize(std::vector<Vertex>& vertices) const;

	VertexBuffer m_vertexBuffer;
	DirectX::XMFLOAT3 m_position{ 0.0f, 0.0f, 0.0f };
};
