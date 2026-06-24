#include "Models/StaticMeshActor.h"

#include "Rendering/Core/Dx12Renderer.h"

#include <string>

void StaticMeshActor::Initialize(ID3D12Device* device)
{
	const StaticMeshActorDefinition& definition = GetStaticMeshDefinition();
	m_position = definition.initialPosition;
	m_model.Initialize(
		device,
		std::string(definition.modelPath),
		ModelScaleSettings::NormalizeToHeight(definition.height));
	m_model.SetPosition(m_position.x, m_position.y, m_position.z);
	m_model.SetRotationY(definition.initialRotationY);
}

void StaticMeshActor::Update(float, const Input&)
{
	m_model.SetPosition(m_position.x, m_position.y, m_position.z);
}

void StaticMeshActor::Draw(Dx12Renderer& renderer) const
{
	m_model.Draw(renderer);
}

const DirectX::XMFLOAT3& StaticMeshActor::GetPosition() const
{
	return m_position;
}

void StaticMeshActor::SetPosition(const DirectX::XMFLOAT3& position)
{
	m_position = position;
	m_model.SetPosition(m_position.x, m_position.y, m_position.z);
}
