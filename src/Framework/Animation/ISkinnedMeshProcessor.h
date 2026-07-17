#pragma once

#include "Framework/Models/SkinnedModelData.h"

#include <DirectXMath.h>
#include <memory>
#include <vector>

class IRenderDevice;
class IRenderer;
class TexturedMaterial;

class ISkinnedMeshProcessor
{
public:
	virtual ~ISkinnedMeshProcessor() = default;

	virtual void Initialize(
		IRenderDevice& device,
		const std::vector<SkinnedVertex>& vertices,
		std::shared_ptr<TexturedMaterial> material) = 0;

	virtual void Update(
		const std::vector<DirectX::XMFLOAT4X4>& boneMatrices,
		float modelCenterX,
		float modelMinY,
		float modelCenterZ,
		float modelScale) = 0;

	virtual void Draw(
		IRenderer& renderer,
		const DirectX::XMMATRIX& world,
		const std::vector<DirectX::XMFLOAT4X4>& boneMatrices,
		float modelCenterX,
		float modelMinY,
		float modelCenterZ,
		float modelScale) const = 0;
};
