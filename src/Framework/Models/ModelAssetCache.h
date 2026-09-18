#pragma once

#include "Framework/Models/ModelLoader.h"
#include "Framework/Models/SkinnedModelData.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct ImageData;

// Scoped to one scene's worker-thread Prepare. Immutable import results are
// reused within that preparation; no process-wide strong cache keeps scenes alive.
class ModelAssetCache
{
public:
	std::shared_ptr<const ModelData> LoadStatic(const std::string& path);
	std::shared_ptr<const SkinnedModelData> LoadSkinned(const std::string& path);
	std::shared_ptr<const std::vector<AnimationClip>> LoadAnimation(
		const std::string& modelPath, const std::string& animationPath, const std::string& animationName);
	std::vector<std::shared_ptr<const ImageData>> TakePreparedImages();

private:
	void PrepareImage(const std::string& path);
	std::unordered_map<std::string, std::shared_ptr<const ModelData>> m_static;
	std::unordered_map<std::string, std::shared_ptr<const SkinnedModelData>> m_skinned;
	std::unordered_map<std::string, std::shared_ptr<const std::vector<AnimationClip>>> m_animations;
	std::unordered_map<std::string, std::shared_ptr<const ImageData>> m_images;
};
