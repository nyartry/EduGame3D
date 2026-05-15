#include "Models/SkinnedModel.h"

#include "Animation/AnimationSampler.h"
#include "Animation/CpuSkinnedMeshProcessor.h"
#include "Rendering/Dx12Renderer.h"
#include "Animation/GpuSkinnedMeshProcessor.h"
#include "Models/SkinnedModelLoader.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <stdexcept>
#include <unordered_map>

using namespace DirectX;

namespace
{
	constexpr const char* FallbackTexturePath = "";

	XMMATRIX LoadMatrix(const XMFLOAT4X4& matrix)
	{
		return XMLoadFloat4x4(&matrix);
	}
}

void SkinnedModel::Initialize(
	ID3D12Device* device,
	const std::string& modelPath,
	const ModelScaleSettings& scaleSettings,
	SkinningMode skinningMode)
{
	SkinnedModelLoader loader;
	if (!loader.Load(modelPath, m_modelData))
	{
		throw std::runtime_error("Failed to load skinned model: " + loader.GetLastError());
	}

	FitModel(scaleSettings);
	m_boneMatrices.resize(m_modelData.bones.size());

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
			material->Initialize(device, baseColorTexturePath, meshData.opacityTexturePath, meshData.normalTexturePath);
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
}

void SkinnedModel::PlayAnimation(const std::string& animationName)
{
	for (size_t animationIndex = 0; animationIndex < m_modelData.animations.size(); ++animationIndex)
	{
		if (m_modelData.animations[animationIndex].name == animationName)
		{
			if (m_currentAnimationIndex != animationIndex)
			{
				m_currentAnimationIndex = animationIndex;
				m_animationTimeSeconds = 0.0f;
			}
			return;
		}
	}
}

float SkinnedModel::GetAnimationDurationSeconds(const std::string& animationName) const
{
	for (const AnimationClip& clip : m_modelData.animations)
	{
		if (clip.name == animationName && clip.ticksPerSecond > 0.0)
		{
			return static_cast<float>(clip.durationTicks / clip.ticksPerSecond);
		}
	}
	return 0.0f;
}

RootMotionDelta SkinnedModel::Update(float deltaTime)
{
	const RootMotionDelta rootMotionDelta = ExtractRootMotionDelta(deltaTime);
	m_animationTimeSeconds += deltaTime;
	UpdateBoneMatrices();
	for (std::unique_ptr<ISkinnedMeshProcessor>& meshProcessor : m_meshProcessors)
	{
		meshProcessor->Update(m_boneMatrices, m_modelCenterX, m_modelMinY, m_modelCenterZ, m_modelScale);
	}
	return rootMotionDelta;
}

void SkinnedModel::Draw(Dx12Renderer& renderer) const
{
	const XMMATRIX world = XMMatrixRotationY(m_rotationY) * XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
	for (const std::unique_ptr<ISkinnedMeshProcessor>& meshProcessor : m_meshProcessors)
	{
		meshProcessor->Draw(renderer, world, m_boneMatrices, m_modelCenterX, m_modelMinY, m_modelCenterZ, m_modelScale);
	}
}

void SkinnedModel::SetPosition(float x, float y, float z)
{
	m_position = XMFLOAT3{ x, y, z };
}

void SkinnedModel::SetRotationY(float radians)
{
	m_rotationY = radians;
}

void SkinnedModel::FitModel(const ModelScaleSettings& scaleSettings)
{
	if (!scaleSettings.normalizeHeight)
	{
		m_modelCenterX = 0.0f;
		m_modelMinY = 0.0f;
		m_modelCenterZ = 0.0f;
		m_modelScale = 1.0f;
		return;
	}

	float minX = std::numeric_limits<float>::max();
	float minY = std::numeric_limits<float>::max();
	float minZ = std::numeric_limits<float>::max();
	float maxX = std::numeric_limits<float>::lowest();
	float maxY = std::numeric_limits<float>::lowest();
	float maxZ = std::numeric_limits<float>::lowest();

	for (const SkinnedMeshData& meshData : m_modelData.meshes)
	{
		for (const SkinnedVertex& vertex : meshData.vertices)
		{
			minX = std::min(minX, vertex.vertex.position.x);
			minY = std::min(minY, vertex.vertex.position.y);
			minZ = std::min(minZ, vertex.vertex.position.z);
			maxX = std::max(maxX, vertex.vertex.position.x);
			maxY = std::max(maxY, vertex.vertex.position.y);
			maxZ = std::max(maxZ, vertex.vertex.position.z);
		}
	}

	const float height = maxY - minY;
	if (height <= 0.0f)
	{
		m_modelScale = 1.0f;
		return;
	}

	m_modelCenterX = (minX + maxX) * 0.5f;
	m_modelMinY = minY;
	m_modelCenterZ = (minZ + maxZ) * 0.5f;
	m_modelScale = scaleSettings.targetHeight / height;
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

RootMotionDelta SkinnedModel::ExtractRootMotionDelta(float deltaTime) const
{
	if (m_modelData.animations.empty() || m_currentAnimationIndex >= m_modelData.animations.size())
	{
		return {};
	}

	const AnimationClip& clip = m_modelData.animations[m_currentAnimationIndex];
	if (clip.rootMotionBoneAnimationIndex < 0 ||
		clip.rootMotionBoneAnimationIndex >= static_cast<int>(clip.boneAnimations.size()))
	{
		return {};
	}

	const BoneAnimation& boneAnimation = clip.boneAnimations[clip.rootMotionBoneAnimationIndex];
	const BoneData& bindPose = m_modelData.bones[boneAnimation.boneIndex];
	const AnimationSampler animationSampler;

	const XMVECTOR previousTranslation = animationSampler.SampleTranslation(
		clip,
		boneAnimation,
		bindPose,
		m_animationTimeSeconds);
	const XMVECTOR nextTranslation = animationSampler.SampleTranslation(
		clip,
		boneAnimation,
		bindPose,
		m_animationTimeSeconds + deltaTime);

	XMVECTOR translationDelta = nextTranslation - previousTranslation;
	const double durationSeconds = clip.durationTicks / clip.ticksPerSecond;
	if (durationSeconds > 0.0)
	{
		const double previousTime = std::fmod(m_animationTimeSeconds, static_cast<float>(durationSeconds));
		const double nextTime = std::fmod(m_animationTimeSeconds + deltaTime, static_cast<float>(durationSeconds));
		if (nextTime < previousTime && !boneAnimation.translations.empty())
		{
			const XMVECTOR firstTranslation = XMLoadFloat3(&boneAnimation.translations.front().value);
			const XMVECTOR lastTranslation = XMLoadFloat3(&boneAnimation.translations.back().value);
			translationDelta = (lastTranslation - previousTranslation) + (nextTranslation - firstTranslation);
		}
	}

	translationDelta *= m_modelScale;

	RootMotionDelta result;
	XMStoreFloat3(&result.translation, translationDelta);
	return result;
}

void SkinnedModel::UpdateBoneMatrices()
{
	std::vector<XMMATRIX> globalTransforms(m_modelData.bones.size(), XMMatrixIdentity());
	const AnimationSampler animationSampler;

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
						m_animationTimeSeconds);
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
		const XMMATRIX rootInverse = LoadMatrix(m_modelData.rootInverseTransform);
		XMStoreFloat4x4(&m_boneMatrices[boneIndex], offset * globalTransforms[boneIndex] * rootInverse);
	}
}

XMMATRIX SkinnedModel::GetLocalTransform(const BoneData& bone) const
{
	return LoadMatrix(bone.localBindTransform);
}
