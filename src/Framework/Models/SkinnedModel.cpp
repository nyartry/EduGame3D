#include "Framework/Models/SkinnedModel.h"
#include "Framework/Models/ModelFit.h"
#include "Framework/Models/ModelAssetCache.h"

#include "Framework/Animation/AnimationSampler.h"
#include "Framework/Animation/RootMotionExtractor.h"
#include "Framework/Assets/AssetPathResolver.h"
#include "Framework/Animation/CpuSkinnedMeshProcessor.h"
#include "Framework/Rendering/Core/IRenderDevice.h"
#include "Framework/Rendering/Core/IRenderer.h"
#include "Framework/Animation/GpuSkinnedMeshProcessor.h"
#include "Framework/Models/SkinnedModelLoader.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

using namespace DirectX;

namespace
{
	constexpr const char* FallbackTexturePath = "";

	void WriteDebugLog(const std::string& message)
	{
		std::clog << message << '\n';
	}

	XMMATRIX LoadMatrix(const XMFLOAT4X4& matrix)
	{
		return XMLoadFloat4x4(&matrix);
	}
}

void SkinnedModel::Initialize(
	IRenderDevice& device,
	const std::string& modelPath,
	const ModelScaleSettings& scaleSettings,
	SkinningMode skinningMode)
{
	ModelAssetCache assets;
	Prepare(assets, modelPath, scaleSettings);
	Activate(device, skinningMode);
}

void SkinnedModel::Prepare(ModelAssetCache& assets, const std::string& modelPath, const ModelScaleSettings& scaleSettings)
{
	m_modelData = *assets.LoadSkinned(modelPath);
	m_preparedModelPath = modelPath;
	m_playback.Reset();
	m_currentAnimationIndex = 0;
	m_animationEvents.clear();
	m_pendingAnimationEvents.clear();
	LoadAnimationEventSidecar(modelPath);

	FitModel(scaleSettings);
	m_boneMatrices.resize(m_modelData.bones.size());
	m_boneModelMatrices.resize(m_modelData.bones.size());
	m_animatedBoundsDirty = true;
}

void SkinnedModel::PrepareAnimation(ModelAssetCache& assets, const std::string& animationName, const std::string& animationPath)
{
	const auto clips = assets.LoadAnimation(m_preparedModelPath, animationPath, animationName);
	m_modelData.animations.insert(m_modelData.animations.end(), clips->begin(), clips->end());
	LoadAnimationEventSidecar(animationPath, animationName);
}

void SkinnedModel::Activate(IRenderDevice& device, SkinningMode skinningMode)
{
	std::unordered_map<std::string, std::shared_ptr<TexturedMaterial>> materialCache;
	m_meshProcessors.clear();
	m_meshProcessors.reserve(m_modelData.meshes.size());

	for (const SkinnedMeshData& meshData : m_modelData.meshes)
	{
		const std::string baseColorTexturePath = meshData.baseColorTexturePath.empty() ? FallbackTexturePath : meshData.baseColorTexturePath;
		const std::string materialKey = baseColorTexturePath + "|" + meshData.opacityTexturePath + "|" + meshData.normalTexturePath;
		std::shared_ptr<TexturedMaterial>& material = materialCache[materialKey];
		if (material == nullptr)
		{
			material = std::make_shared<TexturedMaterial>();
			device.CreateTexturedMaterial(*material, baseColorTexturePath, meshData.opacityTexturePath, meshData.normalTexturePath);
		}

		std::unique_ptr<ISkinnedMeshProcessor> meshProcessor = CreateMeshProcessor(skinningMode);
		meshProcessor->Initialize(device, meshData.vertices, material);
		m_meshProcessors.push_back(std::move(meshProcessor));
	}

	UpdateBoneMatrices();
	for (std::unique_ptr<ISkinnedMeshProcessor>& meshProcessor : m_meshProcessors)
	{
		meshProcessor->Update(m_boneMatrices, m_modelCenterX, m_modelMinY, m_modelCenterZ, m_modelScale);
	}
}

void SkinnedModel::AddAnimation(const std::string& animationName, const std::string& animationPath)
{
	SkinnedModelLoader loader;
	if (!loader.LoadAnimation(animationPath, animationName, m_modelData))
	{
		throw std::runtime_error("Failed to load skinned animation: " + loader.GetLastError());
	}
	LoadAnimationEventSidecar(animationPath, animationName);
}

void SkinnedModel::LoadAnimationEventSidecar(const std::string& modelPath, std::string_view animationAlias)
{
	auto path = AssetPathResolver::Resolve(modelPath);
	path.replace_extension(".anim_events.json");
	std::error_code filesystemError;
	if (!std::filesystem::exists(path, filesystemError)) return;
	std::string error;
	auto data = AnimationEvents::Load(path, error);
	if (!data) throw std::runtime_error("Failed to load animation events for " + modelPath + ": " + error);
	for (auto& event : data->events)
	{
		if (!animationAlias.empty()) event.animation = animationAlias;
		m_animationEvents.push_back(std::move(event));
	}
}

std::vector<AnimationEvents::Occurrence> SkinnedModel::ConsumeAnimationEvents()
{
	std::vector<AnimationEvents::Occurrence> result;
	result.swap(m_pendingAnimationEvents);
	return result;
}

bool SkinnedModel::PlayAnimation(const std::string& animationName, const AnimationPlayOptions& options)
{
	for (size_t animationIndex = 0; animationIndex < m_modelData.animations.size(); ++animationIndex)
	{
		if (m_modelData.animations[animationIndex].name == animationName)
		{
			return PlayAnimationByIndex(animationIndex, options);
		}
	}
	return false;
}

bool SkinnedModel::PlayAnimationByIndex(size_t animationIndex, const AnimationPlayOptions& options)
{
	if (animationIndex >= m_modelData.animations.size())
	{
		return false;
	}
	if (options.mode == AnimationPlaybackMode::Once &&
		::GetAnimationDurationSeconds(m_modelData.animations[animationIndex]) <= 0.0)
	{
		return false;
	}

	if (m_currentAnimationIndex != animationIndex || m_playback.GetMode() != options.mode || options.restart)
	{
		m_currentAnimationIndex = animationIndex;
		m_playback.Reset(options.mode);
		m_pendingAnimationEvents.clear();
		UpdateBoneMatrices();
		for (std::unique_ptr<ISkinnedMeshProcessor>& meshProcessor : m_meshProcessors)
		{
			meshProcessor->Update(m_boneMatrices, m_modelCenterX, m_modelMinY, m_modelCenterZ, m_modelScale);
		}
	}
	return true;
}

float SkinnedModel::GetAnimationDurationSeconds(const std::string& animationName) const
{
	for (const AnimationClip& clip : m_modelData.animations)
	{
		if (clip.name == animationName)
		{
			return static_cast<float>(::GetAnimationDurationSeconds(clip));
		}
	}
	return 0.0f;
}

float SkinnedModel::GetCurrentAnimationDurationSeconds() const
{
	if (m_currentAnimationIndex >= m_modelData.animations.size())
	{
		return 0.0f;
	}

	const AnimationClip& clip = m_modelData.animations[m_currentAnimationIndex];
	return static_cast<float>(::GetAnimationDurationSeconds(clip));
}

float SkinnedModel::GetAnimationTimeSeconds() const
{
	return static_cast<float>(m_playback.GetLocalTimeSeconds());
}

size_t SkinnedModel::GetCurrentAnimationIndex() const
{
	return m_currentAnimationIndex;
}

const SkinnedModelData& SkinnedModel::GetModelData() const
{
	return m_modelData;
}

void SkinnedModel::SetAnimationTimeSeconds(float animationTimeSeconds)
{
	const double duration = m_currentAnimationIndex < m_modelData.animations.size()
		? ::GetAnimationDurationSeconds(m_modelData.animations[m_currentAnimationIndex]) : 0.0;
	m_playback.Seek(animationTimeSeconds, duration);
	m_pendingAnimationEvents.clear();
	UpdateBoneMatrices();
	for (std::unique_ptr<ISkinnedMeshProcessor>& meshProcessor : m_meshProcessors)
	{
		meshProcessor->Update(m_boneMatrices, m_modelCenterX, m_modelMinY, m_modelCenterZ, m_modelScale);
	}
}

RootMotionDelta SkinnedModel::Update(float deltaTime)
{
	RootMotionDelta rootMotionDelta;
	m_pendingAnimationEvents.clear();
	if (m_currentAnimationIndex < m_modelData.animations.size())
	{
		const AnimationClip& clip = m_modelData.animations[m_currentAnimationIndex];
		const auto interval = m_playback.Advance(deltaTime, ::GetAnimationDurationSeconds(clip));
		rootMotionDelta = ExtractRootMotionDelta(clip, m_modelData.bones, interval, m_modelScale);
		m_pendingAnimationEvents = AnimationEvents::Collect(m_animationEvents, clip.name, interval);
	}
	UpdateBoneMatrices();
	for (std::unique_ptr<ISkinnedMeshProcessor>& meshProcessor : m_meshProcessors)
	{
		meshProcessor->Update(m_boneMatrices, m_modelCenterX, m_modelMinY, m_modelCenterZ, m_modelScale);
	}
	return rootMotionDelta;
}

void SkinnedModel::Draw(IRenderer& renderer, const XMMATRIX& world) const
{
	for (const std::unique_ptr<ISkinnedMeshProcessor>& meshProcessor : m_meshProcessors)
	{
		meshProcessor->Draw(renderer, world, m_boneMatrices, m_modelCenterX, m_modelMinY, m_modelCenterZ, m_modelScale);
	}
}

XMFLOAT3 SkinnedModel::GetAnimatedBoundsCenterLocal() const
{
	if (m_animatedBoundsDirty)
	{
		UpdateAnimatedBounds();
	}

	return m_animatedBoundsCenterLocal;
}

bool SkinnedModel::TryGetBonePositionLocal(std::string_view boneName, XMFLOAT3& position) const
{
	const std::string suffixWithColon = ":" + std::string(boneName);
	const std::string suffixWithUnderscore = "_" + std::string(boneName);

	for (size_t boneIndex = 0; boneIndex < m_modelData.bones.size(); ++boneIndex)
	{
		const std::string& currentName = m_modelData.bones[boneIndex].name;
		const bool matches =
			currentName == boneName ||
			currentName.ends_with(suffixWithColon) ||
			currentName.ends_with(suffixWithUnderscore);
		if (matches && TryGetBonePositionLocal(static_cast<int>(boneIndex), position))
		{
			return true;
		}
	}

	return false;
}

bool SkinnedModel::TryGetRootMotionBonePositionLocal(XMFLOAT3& position) const
{
	if (m_modelData.animations.empty() || m_currentAnimationIndex >= m_modelData.animations.size())
	{
		return false;
	}

	const AnimationClip& clip = m_modelData.animations[m_currentAnimationIndex];
	if (clip.rootMotionBoneAnimationIndex < 0 ||
		clip.rootMotionBoneAnimationIndex >= static_cast<int>(clip.boneAnimations.size()))
	{
		return false;
	}

	return TryGetBonePositionLocal(clip.boneAnimations[clip.rootMotionBoneAnimationIndex].boneIndex, position);
}

void SkinnedModel::FitModel(const ModelScaleSettings& scaleSettings)
{
	m_modelCenterX = 0.0f;
	m_modelMinY = 0.0f;
	m_modelCenterZ = 0.0f;
	m_modelScale = 1.0f;
	if (!scaleSettings.normalizeHeight)
	{
		return;
	}

	Aabb bounds;

	for (const SkinnedMeshData& meshData : m_modelData.meshes)
	{
		for (const SkinnedVertex& vertex : meshData.vertices)
		{
			if (!bounds.AddPoint(vertex.vertex.position))
			{
				WriteDebugLog("[SkinnedModel] nonfinite vertex prevents model fit");
				return;
			}
		}
	}

	const XMFLOAT3 size = bounds.Size();
	std::ostringstream stream;
	stream << "[SkinnedModel] rawAabb min=(" << bounds.Min().x << ", " << bounds.Min().y << ", " << bounds.Min().z << ")"
		<< " max=(" << bounds.Max().x << ", " << bounds.Max().y << ", " << bounds.Max().z << ")"
		<< " size=(" << size.x << ", " << size.y << ", " << size.z << ")"
		<< " targetHeight=" << scaleSettings.targetHeight;
	WriteDebugLog(stream.str());

	ModelFit fit;
	if (!TryCreateModelFit(bounds, scaleSettings.targetHeight, fit))
	{
		WriteDebugLog("[SkinnedModel] empty or invalid height; using identity model fit");
		return;
	}

	m_modelCenterX = fit.origin.x;
	m_modelMinY = fit.origin.y;
	m_modelCenterZ = fit.origin.z;
	m_modelScale = fit.scale;

	std::ostringstream scaleStream;
	scaleStream << "[SkinnedModel] modelScale=" << m_modelScale
		<< " fittedSize=("
		<< size.x * m_modelScale << ", "
		<< size.y * m_modelScale << ", "
		<< size.z * m_modelScale << ")";
	WriteDebugLog(scaleStream.str());
}

std::unique_ptr<ISkinnedMeshProcessor> SkinnedModel::CreateMeshProcessor(SkinningMode skinningMode)
{
	switch (skinningMode)
	{
	case SkinningMode::Gpu:
		return std::make_unique<GpuSkinnedMeshProcessor>();
	case SkinningMode::Cpu:
	default:
		return std::make_unique<CpuSkinnedMeshProcessor>();
	}
}

void SkinnedModel::UpdateBoneMatrices()
{
	std::vector<XMMATRIX> globalTransforms(m_modelData.bones.size(), XMMatrixIdentity());
	const AnimationSampler animationSampler;
	const XMMATRIX rootInverse = LoadMatrix(m_modelData.rootInverseTransform);

	for (size_t boneIndex = 0; boneIndex < m_modelData.bones.size(); ++boneIndex)
	{
		const BoneData& bone = m_modelData.bones[boneIndex];
		XMMATRIX localTransform = GetLocalTransform(bone);

		if (!m_modelData.animations.empty() && m_currentAnimationIndex < m_modelData.animations.size())
		{
			const AnimationClip& clip = m_modelData.animations[m_currentAnimationIndex];
			if (boneIndex < clip.boneAnimationIndicesByBone.size())
			{
				const int boneAnimationIndex = clip.boneAnimationIndicesByBone[boneIndex];
				if (boneAnimationIndex >= 0 && boneAnimationIndex < static_cast<int>(clip.boneAnimations.size()))
				{
					const BoneAnimation& boneAnimation = clip.boneAnimations[boneAnimationIndex];
					localTransform = animationSampler.SampleLocalTransform(
						clip,
						boneAnimation,
						m_modelData.bones[boneAnimation.boneIndex],
						m_playback.GetLocalTimeSeconds());
				}
			}
		}

		if (bone.parentIndex >= 0)
		{
			globalTransforms[boneIndex] = localTransform * globalTransforms[bone.parentIndex];
		}
		else
		{
			globalTransforms[boneIndex] = localTransform;
		}

		const XMMATRIX offset = LoadMatrix(bone.offsetMatrix);
		const XMMATRIX modelTransform = globalTransforms[boneIndex] * rootInverse;
		XMStoreFloat4x4(&m_boneModelMatrices[boneIndex], modelTransform);
		XMStoreFloat4x4(&m_boneMatrices[boneIndex], offset * modelTransform);
	}

	m_animatedBoundsDirty = true;
}

void SkinnedModel::UpdateAnimatedBounds() const
{
	Aabb bounds;
	const ModelFit fit{ { m_modelCenterX, m_modelMinY, m_modelCenterZ }, m_modelScale };

	for (const SkinnedMeshData& meshData : m_modelData.meshes)
	{
		for (const SkinnedVertex& vertex : meshData.vertices)
		{
			XMVECTOR position = XMVectorZero();
			float totalWeight = 0.0f;
			const XMVECTOR sourcePosition = XMLoadFloat3(&vertex.vertex.position);

			for (int slot = 0; slot < 4; ++slot)
			{
				const int boneIndex = vertex.boneIndices[slot];
				const float weight = vertex.boneWeights[slot];
				if (boneIndex < 0 || weight == 0.0f || boneIndex >= static_cast<int>(m_boneMatrices.size()))
				{
					continue;
				}

				const XMMATRIX boneMatrix = XMLoadFloat4x4(&m_boneMatrices[boneIndex]);
				position += XMVector3TransformCoord(sourcePosition, boneMatrix) * weight;
				totalWeight += weight;
			}

			if (totalWeight == 0.0f)
			{
				position = sourcePosition;
			}
			else if (totalWeight != 1.0f)
			{
				position /= totalWeight;
			}

			XMFLOAT3 fittedPosition{};
			XMStoreFloat3(&fittedPosition, position);
			bounds.AddPoint(fit.Apply(fittedPosition));
		}
	}

	m_animatedBoundsCenterLocal = bounds.Center();
	m_animatedBoundsDirty = false;
}

bool SkinnedModel::TryGetBonePositionLocal(int boneIndex, XMFLOAT3& position) const
{
	if (boneIndex < 0 || boneIndex >= static_cast<int>(m_boneModelMatrices.size()))
	{
		return false;
	}

	const XMMATRIX boneModel = XMLoadFloat4x4(&m_boneModelMatrices[boneIndex]);
	const XMVECTOR modelPosition = XMVector3TransformCoord(XMVectorZero(), boneModel);
	const XMVECTOR fittedPosition = XMVectorSet(
		(XMVectorGetX(modelPosition) - m_modelCenterX) * m_modelScale,
		(XMVectorGetY(modelPosition) - m_modelMinY) * m_modelScale,
		(XMVectorGetZ(modelPosition) - m_modelCenterZ) * m_modelScale,
		1.0f);

	XMStoreFloat3(&position, fittedPosition);
	return true;
}

XMMATRIX SkinnedModel::GetLocalTransform(const BoneData& bone) const
{
	return LoadMatrix(bone.localBindTransform);
}
