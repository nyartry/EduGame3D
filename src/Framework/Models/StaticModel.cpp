#include "Framework/Models/StaticModel.h"
#include "Framework/Models/ModelFit.h"
#include "Framework/Models/ModelAssetCache.h"

#include "Framework/Rendering/Core/IRenderDevice.h"
#include "Framework/Rendering/Core/IRenderer.h"

#include <DirectXMath.h>
#include <memory>
#include <stdexcept>
#include <unordered_map>

using namespace DirectX;

namespace
{
	constexpr const char* FallbackTexturePath = "";
}

void StaticModel::Initialize(
	IRenderDevice& device,
	const std::string& modelPath,
	const ModelScaleSettings& scaleSettings)
{
	ModelAssetCache assets;
	Prepare(assets, modelPath, scaleSettings);
	Activate(device);
}

void StaticModel::Prepare(ModelAssetCache& assets, const std::string& modelPath, const ModelScaleSettings& scaleSettings)
{
	m_preparedData = *assets.LoadStatic(modelPath);
	FitModel(m_preparedData, scaleSettings);
}

void StaticModel::Activate(IRenderDevice& device)
{
	m_meshParts.clear();
	m_meshParts.reserve(m_preparedData.texturedMeshes.size());
	std::unordered_map<std::string, std::shared_ptr<TexturedMaterial>> materialCache;

	for (const TexturedMeshData& meshData : m_preparedData.texturedMeshes)
	{
		const std::string baseColorTexturePath = meshData.baseColorTexturePath.empty() ? FallbackTexturePath : meshData.baseColorTexturePath;
		const std::string materialKey = baseColorTexturePath + "|" + meshData.opacityTexturePath + "|" + meshData.normalTexturePath;
		std::shared_ptr<TexturedMaterial>& material = materialCache[materialKey];
		if (material == nullptr)
		{
			material = std::make_shared<TexturedMaterial>();
			device.CreateTexturedMaterial(*material, baseColorTexturePath, meshData.opacityTexturePath, meshData.normalTexturePath);
		}

		MeshPart meshPart;
		device.CreateTexturedVertexBuffer(meshPart.vertexBuffer, meshData.vertices);
		meshPart.material = material;
		m_meshParts.push_back(std::move(meshPart));
	}
}

void StaticModel::Draw(IRenderer& renderer, const XMMATRIX& world) const
{
	for (const MeshPart& meshPart : m_meshParts)
	{
		renderer.DrawTextured(meshPart.vertexBuffer, *meshPart.material, world);
	}
}

void StaticModel::FitModel(ModelData& modelData, const ModelScaleSettings& scaleSettings) const
{
	if (modelData.texturedMeshes.empty() || !scaleSettings.normalizeHeight)
	{
		return;
	}

	Aabb bounds;

	for (const TexturedMeshData& meshData : modelData.texturedMeshes)
	{
		for (const TexturedVertex& vertex : meshData.vertices)
		{
			if (!bounds.AddPoint(vertex.position)) return;
		}
	}

	ModelFit fit;
	if (!TryCreateModelFit(bounds, scaleSettings.targetHeight, fit))
	{
		return;
	}

	for (TexturedMeshData& meshData : modelData.texturedMeshes)
	{
		for (TexturedVertex& vertex : meshData.vertices)
		{
			vertex.position = fit.Apply(vertex.position);
		}
	}
}
