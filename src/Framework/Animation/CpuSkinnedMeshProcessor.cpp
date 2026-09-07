#include "Framework/Animation/CpuSkinnedMeshProcessor.h"
#include "Framework/Animation/BoneSkinning.h"

#include "Framework/Rendering/Core/IRenderDevice.h"
#include "Framework/Rendering/Core/IRenderer.h"
#include "Framework/Rendering/Materials/TexturedMaterial.h"

using namespace DirectX;

void CpuSkinnedMeshProcessor::Initialize(
	IRenderDevice& device,
	const std::vector<SkinnedVertex>& vertices,
	std::shared_ptr<TexturedMaterial> material)
{
	m_sourceVertices = vertices;
	m_skinnedVertices.resize(vertices.size());
	for (size_t index = 0; index < vertices.size(); ++index)
	{
		m_skinnedVertices[index] = vertices[index].vertex;
	}

	device.CreateTexturedVertexBuffer(m_vertexBuffer, m_skinnedVertices);
	m_material = std::move(material);
}

void CpuSkinnedMeshProcessor::Update(
	const std::vector<XMFLOAT4X4>& boneMatrices,
	float modelCenterX,
	float modelMinY,
	float modelCenterZ,
	float modelScale)
{
	BoneSkinning::BuildNormalPalette(boneMatrices, m_normalBoneMatrices);
	for (size_t vertexIndex = 0; vertexIndex < m_sourceVertices.size(); ++vertexIndex)
	{
		const SkinnedVertex& sourceVertex = m_sourceVertices[vertexIndex];
		TexturedVertex skinnedVertex = BoneSkinning::DeformVertex(sourceVertex, boneMatrices, m_normalBoneMatrices);
		skinnedVertex.position = {
			(skinnedVertex.position.x - modelCenterX) * modelScale,
			(skinnedVertex.position.y - modelMinY) * modelScale,
			(skinnedVertex.position.z - modelCenterZ) * modelScale
		};
		m_skinnedVertices[vertexIndex] = skinnedVertex;
	}

	m_vertexBuffer.Update(m_skinnedVertices);
}

void CpuSkinnedMeshProcessor::Draw(
	IRenderer& renderer,
	const XMMATRIX& world,
	const std::vector<XMFLOAT4X4>&,
	float,
	float,
	float,
	float) const
{
	renderer.DrawTextured(m_vertexBuffer, *m_material, world);
}
