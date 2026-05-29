#include "Animation/GpuSkinnedMeshProcessor.h"

#include "Rendering/Core/Dx12Renderer.h"
#include "Rendering/Materials/TexturedMaterial.h"

using namespace DirectX;

void GpuSkinnedMeshProcessor::Initialize(
	ID3D12Device* device,
	const std::vector<SkinnedVertex>& vertices,
	std::shared_ptr<TexturedMaterial> material)
{
	m_vertexBuffer.Initialize(device, vertices);
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
	Dx12Renderer& renderer,
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
