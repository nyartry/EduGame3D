#pragma once

#include "SkinnedModelData.h"

#include <DirectXMath.h>
#include <Windows.h>

#include <memory>
#include <vector>

class Dx12Renderer;
class TexturedMaterial;
struct ID3D12Device;

class ISkinnedMeshProcessor
{
public:
	virtual ~ISkinnedMeshProcessor() = default;

	virtual void Initialize(
		ID3D12Device* device,
		const std::vector<SkinnedVertex>& vertices,
		std::shared_ptr<TexturedMaterial> material) = 0;

	virtual void Update(
		const std::vector<DirectX::XMFLOAT4X4>& boneMatrices,
		float modelCenterX,
		float modelMinY,
		float modelCenterZ,
		float modelScale) = 0;

	virtual void Draw(
		Dx12Renderer& renderer,
		const DirectX::XMMATRIX& world,
		const std::vector<DirectX::XMFLOAT4X4>& boneMatrices,
		float modelCenterX,
		float modelMinY,
		float modelCenterZ,
		float modelScale) const = 0;
};
