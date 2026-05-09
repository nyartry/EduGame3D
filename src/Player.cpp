#include "Player.h"

#include "Dx12Renderer.h"

#include <DirectXMath.h>
#include <algorithm>
#include <limits>
#include <memory>
#include <stdexcept>
#include <unordered_map>

using namespace DirectX;

namespace
{
	constexpr float PlayerHeight = 1.8f;
	constexpr const char* FallbackTexturePath = "";
}

void Player::Initialize(ID3D12Device* device, const std::string& modelPath)
{
	ModelLoader loader;
	ModelData modelData;
	if (!loader.Load(modelPath, modelData))
	{
		throw std::runtime_error("Failed to load player model: " + loader.GetLastError());
	}

	FitModelToPlayerSize(modelData);

	m_meshParts.clear();
	m_meshParts.reserve(modelData.texturedMeshes.size());
	std::unordered_map<std::string, std::shared_ptr<TexturedMaterial>> materialCache;

	for (const TexturedMeshData& meshData : modelData.texturedMeshes)
	{
		const std::string baseColorTexturePath = meshData.baseColorTexturePath.empty() ? FallbackTexturePath : meshData.baseColorTexturePath;
		const std::string materialKey = baseColorTexturePath + "|" + meshData.opacityTexturePath;
		std::shared_ptr<TexturedMaterial>& material = materialCache[materialKey];
		if (material == nullptr)
		{
			material = std::make_shared<TexturedMaterial>();
			material->Initialize(device, baseColorTexturePath, meshData.opacityTexturePath);
		}

		MeshPart meshPart;
		meshPart.vertexBuffer.Initialize(device, meshData.vertices);
		meshPart.material = material;
		m_meshParts.push_back(std::move(meshPart));
	}
}

void Player::Draw(Dx12Renderer& renderer) const
{
	const XMMATRIX world = XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
	for (const MeshPart& meshPart : m_meshParts)
	{
		renderer.DrawTextured(meshPart.vertexBuffer, *meshPart.material, world);
	}
}

void Player::SetPosition(float x, float y, float z)
{
	m_position = XMFLOAT3{ x, y, z };
}

XMFLOAT3 Player::GetPosition() const
{
	return m_position;
}

void Player::FitModelToPlayerSize(ModelData& modelData) const
{
	if (modelData.texturedMeshes.empty())
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
			minX = std::min(minX, vertex.position[0]);
			minY = std::min(minY, vertex.position[1]);
			minZ = std::min(minZ, vertex.position[2]);
			maxX = std::max(maxX, vertex.position[0]);
			maxY = std::max(maxY, vertex.position[1]);
			maxZ = std::max(maxZ, vertex.position[2]);
		}
	}

	const float height = maxY - minY;
	if (height <= 0.0f)
	{
		return;
	}

	const float centerX = (minX + maxX) * 0.5f;
	const float centerZ = (minZ + maxZ) * 0.5f;
	const float scale = PlayerHeight / height;

	for (TexturedMeshData& meshData : modelData.texturedMeshes)
	{
		for (TexturedVertex& vertex : meshData.vertices)
		{
			vertex.position[0] = (vertex.position[0] - centerX) * scale;
			vertex.position[1] = (vertex.position[1] - minY) * scale;
			vertex.position[2] = (vertex.position[2] - centerZ) * scale;
		}
	}
}
