#include "Framework/Models/StaticMeshActor.h"
#include "Framework/Models/ModelAssetCache.h"

#include "Framework/Rendering/Core/IRenderDevice.h"
#include "Framework/Rendering/Core/IRenderer.h"

#include <string>

void StaticMeshActor::Prepare(ModelAssetCache& assets)
{
	m_prepared = false;
	const StaticMeshActorDefinition& definition = GetStaticMeshDefinition();
	m_transform = Transform{};
	m_transform.position = definition.initialPosition;
	m_transform.rotationRadians.y = definition.initialRotationY;
	m_model.Prepare(
		assets,
		std::string(definition.modelPath),
		ModelScaleSettings::NormalizeToHeight(definition.height));
	m_prepared = true;
}

void StaticMeshActor::Initialize(IRenderDevice& device)
{
	ModelAssetCache assets;
	if (!m_prepared) Prepare(assets);
	m_model.Activate(device);
}

void StaticMeshActor::Update(float, const Input&)
{
}

void StaticMeshActor::Draw(IRenderer& renderer) const
{
	m_model.Draw(renderer, m_transform.ToMatrix());
}

const DirectX::XMFLOAT3& StaticMeshActor::GetPosition() const
{
	return m_transform.position;
}

const Transform& StaticMeshActor::GetTransform() const
{
	return m_transform;
}

void StaticMeshActor::SetPosition(const DirectX::XMFLOAT3& position)
{
	m_transform.position = position;
}
