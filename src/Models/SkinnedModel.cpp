#include "Models/SkinnedModel.h"

#include "Animation/AnimationSampler.h"
#include "Animation/CpuSkinnedMeshProcessor.h"
#include "Rendering/Core/Dx12Renderer.h"
#include "Animation/GpuSkinnedMeshProcessor.h"
#include "Models/SkinnedModelLoader.h"

#include <algorithm>
#include <cmath>
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
		OutputDebugStringA(message.c_str());
		OutputDebugStringA("\n");
	}

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
	m_boneModelMatrices.resize(m_modelData.bones.size());

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
	const float width = maxX - minX;
	const float depth = maxZ - minZ;
	std::ostringstream stream;
	stream << "[SkinnedModel] rawAabb min=(" << minX << ", " << minY << ", " << minZ << ")"
		<< " max=(" << maxX << ", " << maxY << ", " << maxZ << ")"
		<< " size=(" << width << ", " << height << ", " << depth << ")"
		<< " targetHeight=" << scaleSettings.targetHeight;
	WriteDebugLog(stream.str());

	if (height <= 0.0f)
	{
		m_modelScale = 1.0f;
		WriteDebugLog("[SkinnedModel] height is not positive. modelScale=1");
		return;
	}

	m_modelCenterX = (minX + maxX) * 0.5f;
	m_modelMinY = minY;
	m_modelCenterZ = (minZ + maxZ) * 0.5f;
	m_modelScale = scaleSettings.targetHeight / height;

	std::ostringstream scaleStream;
	scaleStream << "[SkinnedModel] modelScale=" << m_modelScale
		<< " fittedSize=("
		<< width * m_modelScale << ", "
		<< height * m_modelScale << ", "
		<< depth * m_modelScale << ")";
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
		const XMMATRIX modelTransform = globalTransforms[boneIndex] * rootInverse;
		XMStoreFloat4x4(&m_boneModelMatrices[boneIndex], modelTransform);
		XMStoreFloat4x4(&m_boneMatrices[boneIndex], offset * modelTransform);
	}

	m_animatedBoundsDirty = true;
}

void SkinnedModel::UpdateAnimatedBounds() const
{
	XMFLOAT3 minBounds
	{
		std::numeric_limits<float>::max(),
		std::numeric_limits<float>::max(),
		std::numeric_limits<float>::max()
	};
	XMFLOAT3 maxBounds
	{
		std::numeric_limits<float>::lowest(),
		std::numeric_limits<float>::lowest(),
		std::numeric_limits<float>::lowest()
	};
	bool hasVertex = false;

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

			position = XMVectorSet(
				(XMVectorGetX(position) - m_modelCenterX) * m_modelScale,
				(XMVectorGetY(position) - m_modelMinY) * m_modelScale,
				(XMVectorGetZ(position) - m_modelCenterZ) * m_modelScale,
				1.0f);

			XMFLOAT3 fittedPosition{};
			XMStoreFloat3(&fittedPosition, position);
			minBounds.x = std::min(minBounds.x, fittedPosition.x);
			minBounds.y = std::min(minBounds.y, fittedPosition.y);
			minBounds.z = std::min(minBounds.z, fittedPosition.z);
			maxBounds.x = std::max(maxBounds.x, fittedPosition.x);
			maxBounds.y = std::max(maxBounds.y, fittedPosition.y);
			maxBounds.z = std::max(maxBounds.z, fittedPosition.z);
			hasVertex = true;
		}
	}

	m_animatedBoundsCenterLocal = hasVertex
		? XMFLOAT3
		{
			(minBounds.x + maxBounds.x) * 0.5f,
			(minBounds.y + maxBounds.y) * 0.5f,
			(minBounds.z + maxBounds.z) * 0.5f
		}
	: XMFLOAT3{};
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
