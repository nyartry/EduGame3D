#include "PrimitiveObject.h"

#include "Dx12Renderer.h"

using namespace DirectX;

void PrimitiveObject::Initialize(ID3D12Device* device)
{
	m_vertexBuffer.Initialize(device, BuildVertices());
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

XMFLOAT3 PrimitiveObject::GetPosition() const
{
	return m_position;
}
