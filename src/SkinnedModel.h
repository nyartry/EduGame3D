#pragma once

#include "SkinnedModelData.h"
#include "TexturedMaterial.h"
#include "TexturedVertexBuffer.h"

#include <DirectXMath.h>
#include <Windows.h>

#include <memory>
#include <string>
#include <vector>

class Dx12Renderer;
struct ID3D12Device;

class SkinnedModel
{
public:
	void Initialize(ID3D12Device* device, const std::string& modelPath);
	void Update(float deltaTime);
	void Draw(Dx12Renderer& renderer) const;

	void SetPosition(float x, float y, float z);

private:
	struct MeshPart
	{
		std::vector<SkinnedVertex> sourceVertices;
		std::vector<TexturedVertex> skinnedVertices;
		TexturedVertexBuffer vertexBuffer;
		std::shared_ptr<TexturedMaterial> material;
	};

	void FitModelToHeight();
	void UpdateBoneMatrices();
	void SkinMeshes();

	DirectX::XMMATRIX GetLocalTransform(const BoneData& bone) const;
	DirectX::XMMATRIX GetAnimatedLocalTransform(const AnimationClip& clip, const BoneAnimation& boneAnimation) const;

	SkinnedModelData m_modelData;
	std::vector<MeshPart> m_meshParts;
	std::vector<DirectX::XMFLOAT4X4> m_boneMatrices;
	DirectX::XMFLOAT3 m_position{ 0.0f, 0.0f, 0.0f };
	float m_animationTimeSeconds{};
	float m_modelScale{ 1.0f };
	float m_modelCenterX{};
	float m_modelMinY{};
	float m_modelCenterZ{};
};
