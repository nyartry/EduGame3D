#include "StaticMeshActor.h"

#include "Dx12Renderer.h"

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
}

void StaticMeshActor::Update(float)
{
}

void StaticMeshActor::Draw(Dx12Renderer& renderer) const
{
	m_model.Draw(renderer);
}
