#include "Framework/Models/ModelAssetCache.h"

#include "Framework/Assets/AssetPathResolver.h"
#include "Framework/Assets/ImageLoader.h"
#include "Framework/Models/SkinnedModelLoader.h"

#include <stdexcept>

namespace
{
	std::string KeyPart(const std::string& text)
	{
		return std::to_string(text.size()) + ":" + text;
	}
}

std::shared_ptr<const ModelData> ModelAssetCache::LoadStatic(const std::string& path)
{
	const std::string key = AssetPathResolver::ResolveUtf8(path);
	if (const auto found = m_static.find(key); found != m_static.end()) return found->second;
	auto data = std::make_shared<ModelData>();
	ModelLoader loader;
	if (!loader.Load(key, *data)) throw std::runtime_error("Failed to prepare static model " + path + ": " + loader.GetLastError());
	for (const auto& mesh : data->texturedMeshes)
	{
		PrepareImage(mesh.baseColorTexturePath);
		PrepareImage(mesh.opacityTexturePath);
		PrepareImage(mesh.normalTexturePath);
	}
	m_static.emplace(key, data);
	return data;
}

std::shared_ptr<const SkinnedModelData> ModelAssetCache::LoadSkinned(const std::string& path)
{
	const std::string key = AssetPathResolver::ResolveUtf8(path);
	if (const auto found = m_skinned.find(key); found != m_skinned.end()) return found->second;
	auto data = std::make_shared<SkinnedModelData>();
	SkinnedModelLoader loader;
	if (!loader.Load(key, *data)) throw std::runtime_error("Failed to prepare skinned model " + path + ": " + loader.GetLastError());
	for (const auto& mesh : data->meshes)
	{
		PrepareImage(mesh.baseColorTexturePath);
		PrepareImage(mesh.opacityTexturePath);
		PrepareImage(mesh.normalTexturePath);
	}
	m_skinned.emplace(key, data);
	return data;
}

std::shared_ptr<const std::vector<AnimationClip>> ModelAssetCache::LoadAnimation(
	const std::string& modelPath, const std::string& animationPath, const std::string& animationName)
{
	// Include the destination model: bone indices/translation policy depend on
	// its skeleton, so a path-only animation cache would attach incorrect bones.
	const std::string modelKey = AssetPathResolver::ResolveUtf8(modelPath);
	const std::string animationKey = AssetPathResolver::ResolveUtf8(animationPath);
	const std::string key = KeyPart(modelKey) + KeyPart(animationKey) + KeyPart(animationName);
	if (const auto found = m_animations.find(key); found != m_animations.end()) return found->second;
	const auto source = LoadSkinned(modelKey);
	auto clips = std::make_shared<std::vector<AnimationClip>>();
	if (modelKey == animationKey)
	{
		// The model import already contains these clips mapped to this skeleton.
		for (const auto& clip : source->animations)
		{
			if (clip.boneAnimations.empty()) continue;
			clips->push_back(clip);
			if (!animationName.empty()) clips->back().name = animationName;
		}
	}
	else
	{
		SkinnedModelData target;
		target.bones = source->bones;
		SkinnedModelLoader loader;
		if (!loader.LoadAnimation(animationKey, animationName, target))
		{
			throw std::runtime_error("Failed to prepare animation " + animationPath + ": " + loader.GetLastError());
		}
		*clips = std::move(target.animations);
	}
	if (clips->empty()) throw std::runtime_error("Prepared animation has no matching bone tracks: " + animationPath);
	m_animations.emplace(key, clips);
	return clips;
}

void ModelAssetCache::PrepareImage(const std::string& path)
{
	if (path.empty()) return;
	const std::string key = AssetPathResolver::ResolveUtf8(path);
	if (!m_images.contains(key)) m_images.emplace(key, ImageLoader::Load(key));
}

std::vector<std::shared_ptr<const ImageData>> ModelAssetCache::TakePreparedImages()
{
	std::vector<std::shared_ptr<const ImageData>> images;
	images.reserve(m_images.size());
	for (auto& entry : m_images) images.push_back(std::move(entry.second));
	m_images.clear();
	return images;
}
