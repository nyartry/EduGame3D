#include "Models/StaticModel.h"

#include "Rendering/Dx12Renderer.h"

#include <DirectXMath.h>
#include <algorithm>
#include <limits>
#include <memory>
#include <stdexcept>
#include <unordered_map>

using namespace DirectX;

namespace
{
	constexpr const char* FallbackTexturePath = "";
}

void StaticModel::Initialize(
	ID3D12Device* device,
	const std::string& modelPath,
	const ModelScaleSettings& scaleSettings)
{
	ModelLoader loader;
	ModelData modelData;
	if (!loader.Load(modelPath, modelData))
	{
		throw std::runtime_error("Failed to load static model: " + loader.GetLastError());
	}

	FitModel(modelData, scaleSettings);

	m_meshParts.clear();
	m_meshParts.reserve(modelData.texturedMeshes.size());
	std::unordered_map<std::string, std::shared_ptr<TexturedMaterial>> materialCache;

	for (const TexturedMeshData& meshData : modelData.texturedMeshes)
	{
		const std::string baseColorTexturePath = meshData.baseColorTexturePath.empty() ? FallbackTexturePath : meshData.baseColorTexturePath;
		const std::string materialKey = baseColorTexturePath + "|" + meshData.opacityTexturePath + "|" + meshData.normalTexturePath;
		std::shared_ptr<TexturedMaterial>& material = materialCache[materialKey];
		if (material == nullptr)
		{
			material = std::make_shared<TexturedMaterial>();
			material->Initialize(device, baseColorTexturePath, meshData.opacityTexturePath, meshData.normalTexturePath);
		}

		MeshPart meshPart;
		meshPart.vertexBuffer.Initialize(device, meshData.vertices);
		meshPart.material = material;
		m_meshParts.push_back(std::move(meshPart));
	}
}

void StaticModel::Draw(Dx12Renderer& renderer) const
{
	const XMMATRIX world = XMMatrixRotationY(m_rotationY) * XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
	for (const MeshPart& meshPart : m_meshParts)
	{
		renderer.DrawTextured(meshPart.vertexBuffer, *meshPart.material, world);
	}
}

void StaticModel::SetPosition(float x, float y, float z)
{
	m_position = XMFLOAT3{ x, y, z };
}

XMFLOAT3 StaticModel::GetPosition() const
{
	return m_position;
}

void StaticModel::SetRotationY(float radians)
{
	m_rotationY = radians;
}

void StaticModel::FitModel(ModelData& modelData, const ModelScaleSettings& scaleSettings) const
{
	if (modelData.texturedMeshes.empty() || !scaleSettings.normalizeHeight)
	{
		return;
	}

	float minX = std::numeric_limits<float>::max();
	float minY = std::numeric_limits<float>::max();
	float minZ = std::numeric_limits<float>::max();
	float maxX = std::numeric_limits<float>::lowest();
	float maxY = std::numeric_limits<float>::lowest();
	float maxZ = std::numeric_limits<float>::lowest();

	for (const TexturedMeshData& meshData : modelData.texturedMeshes)
	{
		for (const TexturedVertex& vertex : meshData.vertices)
		{
			minX = std::min(minX, vertex.position.x);
			minY = std::min(minY, vertex.position.y);
			minZ = std::min(minZ, vertex.position.z);
			maxX = std::max(maxX, vertex.position.x);
			maxY = std::max(maxY, vertex.position.y);
			maxZ = std::max(maxZ, vertex.position.z);
		}
	}

	const float height = maxY - minY;
	if (height <= 0.0f)
	{
		return;
	}

	const float centerX = (minX + maxX) * 0.5f;
	const float centerZ = (minZ + maxZ) * 0.5f;
	const float scale = scaleSettings.targetHeight / height;

	for (TexturedMeshData& meshData : modelData.texturedMeshes)
	{
		for (TexturedVertex& vertex : meshData.vertices)
		{
			vertex.position.x = (vertex.position.x - centerX) * scale;
			vertex.position.y = (vertex.position.y - minY) * scale;
			vertex.position.z = (vertex.position.z - centerZ) * scale;
		}
	}
}
