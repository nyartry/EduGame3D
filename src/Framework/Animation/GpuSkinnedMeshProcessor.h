#pragma once

#include "Framework/Animation/ISkinnedMeshProcessor.h"
#include "Framework/Rendering/Buffers/SkinnedVertexBuffer.h"

class GpuSkinnedMeshProcessor : public ISkinnedMeshProcessor
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
	SkinnedVertexBuffer m_vertexBuffer;
	std::shared_ptr<TexturedMaterial> m_material;
};
