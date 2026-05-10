#pragma once

#include "ModelLoader.h"
#include "TexturedMaterial.h"
#include "TexturedVertexBuffer.h"

#include <Windows.h>

#include <DirectXMath.h>
#include <memory>
#include <string>
#include <vector>

class Dx12Renderer;
struct ID3D12Device;

class StaticModel
{
public:
	void Initialize(ID3D12Device* device, const std::string& modelPath);
	void Draw(Dx12Renderer& renderer) const;

	void SetPosition(float x, float y, float z);
	DirectX::XMFLOAT3 GetPosition() const;

private:
	struct MeshPart
	{
		TexturedVertexBuffer vertexBuffer;
		std::shared_ptr<TexturedMaterial> material;
	};

	void FitModelToHeight(ModelData& modelData) const;

	std::vector<MeshPart> m_meshParts;
	DirectX::XMFLOAT3 m_position{ 0.0f, 0.0f, 0.0f };
};
