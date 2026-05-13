#pragma once

#include "Animation/ISkinnedMeshProcessor.h"
#include "Rendering/TexturedVertexBuffer.h"

class CpuSkinnedMeshProcessor : public ISkinnedMeshProcessor
{
public:
	void Initialize(
		ID3D12Device* device,
		const std::vector<SkinnedVertex>& vertices,
		std::shared_ptr<TexturedMaterial> material) override;

	void Update(
		const std::vector<DirectX::XMFLOAT4X4>& boneMatrices,
		float modelCenterX,
		float modelMinY,
		float modelCenterZ,
		float modelScale) override;

	void Draw(
		Dx12Renderer& renderer,
		const DirectX::XMMATRIX& world,
		const std::vector<DirectX::XMFLOAT4X4>& boneMatrices,
		float modelCenterX,
		float modelMinY,
		float modelCenterZ,
		float modelScale) const override;

private:
	std::vector<SkinnedVertex> m_sourceVertices;
	std::vector<TexturedVertex> m_skinnedVertices;
	TexturedVertexBuffer m_vertexBuffer;
	std::shared_ptr<TexturedMaterial> m_material;
};
