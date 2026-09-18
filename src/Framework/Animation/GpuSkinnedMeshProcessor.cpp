#include "Framework/Animation/GpuSkinnedMeshProcessor.h"

#include "Framework/Rendering/Core/IRenderDevice.h"
#include "Framework/Rendering/Core/IRenderer.h"
#include "Framework/Rendering/Materials/TexturedMaterial.h"

using namespace DirectX;

void GpuSkinnedMeshProcessor::Initialize(
	IRenderDevice& device,
	const std::vector<SkinnedVertex>& vertices,
	std::shared_ptr<TexturedMaterial> material)
{
	device.CreateSkinnedVertexBuffer(m_vertexBuffer, vertices);
	m_material = std::move(material);
}

void GpuSkinnedMeshProcessor::Update(
	const std::vector<XMFLOAT4X4>&,
	float,
	float,
	float,
	float)
{
}

void GpuSkinnedMeshProcessor::Draw(
	IRenderer& renderer,
	const XMMATRIX& world,
	const std::vector<XMFLOAT4X4>& boneMatrices,
	float modelCenterX,
	float modelMinY,
	float modelCenterZ,
	float modelScale) const
{
	renderer.DrawSkinnedTextured(
		m_vertexBuffer,
		*m_material,
		world,
		boneMatrices,
		modelCenterX,
		modelMinY,
		modelCenterZ,
		modelScale);
}
