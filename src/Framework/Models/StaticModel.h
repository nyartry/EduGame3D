#pragma once

#include "Framework/Common/ModelScaleSettings.h"
#include "Framework/Models/ModelLoader.h"
#include "Framework/Rendering/Materials/TexturedMaterial.h"
#include "Framework/Rendering/Buffers/TexturedVertexBuffer.h"

#include <DirectXMath.h>
#include <memory>
#include <string>
#include <vector>

class IRenderDevice;
class IRenderer;
class ModelAssetCache;

class StaticModel
{
public:
	void Prepare(ModelAssetCache& assets, const std::string& modelPath, const ModelScaleSettings& scaleSettings);
	void Activate(IRenderDevice& device);
	void Initialize(
		IRenderDevice& device,
		const std::string& modelPath,
		const ModelScaleSettings& scaleSettings = ModelScaleSettings::OriginalSize());
	// World placement belongs to the actor/editor, not the model resource.
	void Draw(IRenderer& renderer, const DirectX::XMMATRIX& world) const;

private:
	struct MeshPart
	{
		TexturedVertexBuffer vertexBuffer;
		std::shared_ptr<TexturedMaterial> material;
	};

	void FitModel(ModelData& modelData, const ModelScaleSettings& scaleSettings) const;

	std::vector<MeshPart> m_meshParts;
	ModelData m_preparedData;
};
