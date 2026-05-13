#include "Animation/CpuSkinnedMeshProcessor.h"

#include "Rendering/Dx12Renderer.h"
#include "Rendering/TexturedMaterial.h"

using namespace DirectX;

namespace
{
	XMVECTOR NormalizeOrDefault(XMVECTOR vector, XMVECTOR defaultVector)
	{
		const XMVECTOR length = XMVector3LengthSq(vector);
		if (XMVectorGetX(length) <= 0.0f)
		{
			return defaultVector;
		}

		return XMVector3Normalize(vector);
	}
}

void CpuSkinnedMeshProcessor::Initialize(
	ID3D12Device* device,
	const std::vector<SkinnedVertex>& vertices,
	std::shared_ptr<TexturedMaterial> material)
{
	m_sourceVertices = vertices;
	m_skinnedVertices.resize(vertices.size());
	for (size_t index = 0; index < vertices.size(); ++index)
	{
		m_skinnedVertices[index] = vertices[index].vertex;
	}

	m_vertexBuffer.Initialize(device, m_skinnedVertices);
	m_material = std::move(material);
}

void CpuSkinnedMeshProcessor::Update(
	const std::vector<XMFLOAT4X4>& boneMatrices,
	float modelCenterX,
	float modelMinY,
	float modelCenterZ,
	float modelScale)
{
	for (size_t vertexIndex = 0; vertexIndex < m_sourceVertices.size(); ++vertexIndex)
	{
		const SkinnedVertex& sourceVertex = m_sourceVertices[vertexIndex];
		TexturedVertex skinnedVertex = sourceVertex.vertex;

		XMVECTOR position = XMVectorZero();
		XMVECTOR normal = XMVectorZero();
		XMVECTOR tangent = XMVectorZero();
		float totalWeight = 0.0f;

		const XMVECTOR sourcePosition = XMLoadFloat3(&sourceVertex.vertex.position);
		const XMVECTOR sourceNormal = XMLoadFloat3(&sourceVertex.vertex.normal);
		const XMVECTOR sourceTangent = XMLoadFloat3(&sourceVertex.vertex.tangent);

		for (int slot = 0; slot < 4; ++slot)
		{
			const int boneIndex = sourceVertex.boneIndices[slot];
			const float weight = sourceVertex.boneWeights[slot];
			if (boneIndex < 0 || weight == 0.0f || boneIndex >= static_cast<int>(boneMatrices.size()))
			{
				continue;
			}

			const XMMATRIX boneMatrix = XMLoadFloat4x4(&boneMatrices[boneIndex]);
			position += XMVector3TransformCoord(sourcePosition, boneMatrix) * weight;
			normal += XMVector3TransformNormal(sourceNormal, boneMatrix) * weight;
			tangent += XMVector3TransformNormal(sourceTangent, boneMatrix) * weight;
			totalWeight += weight;
		}

		if (totalWeight == 0.0f)
		{
			position = sourcePosition;
			normal = sourceNormal;
			tangent = sourceTangent;
		}
		else if (totalWeight != 1.0f)
		{
			position /= totalWeight;
			normal /= totalWeight;
			tangent /= totalWeight;
		}

		position = XMVectorSet(
			(XMVectorGetX(position) - modelCenterX) * modelScale,
			(XMVectorGetY(position) - modelMinY) * modelScale,
			(XMVectorGetZ(position) - modelCenterZ) * modelScale,
			1.0f);

		XMStoreFloat3(&skinnedVertex.position, position);
		XMStoreFloat3(&skinnedVertex.normal, NormalizeOrDefault(normal, sourceNormal));
		XMStoreFloat3(&skinnedVertex.tangent, NormalizeOrDefault(tangent, sourceTangent));
		m_skinnedVertices[vertexIndex] = skinnedVertex;
	}

	m_vertexBuffer.Update(m_skinnedVertices);
}

void CpuSkinnedMeshProcessor::Draw(
	Dx12Renderer& renderer,
	const XMMATRIX& world,
	const std::vector<XMFLOAT4X4>&,
	float,
	float,
	float,
	float) const
{
	renderer.DrawTextured(m_vertexBuffer, *m_material, world);
}
