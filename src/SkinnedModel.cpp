#include "SkinnedModel.h"

#include "Dx12Renderer.h"
#include "SkinnedModelLoader.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <stdexcept>
#include <unordered_map>

using namespace DirectX;

namespace
{
	constexpr float ModelHeight = 1.8f;
	constexpr const char* FallbackTexturePath = "";

	XMMATRIX LoadMatrix(const XMFLOAT4X4& matrix)
	{
		return XMLoadFloat4x4(&matrix);
	}

	XMMATRIX BuildTransform(const XMFLOAT3& translation, const XMFLOAT4& rotation, const XMFLOAT3& scale)
	{
		const XMVECTOR rotationQuaternion = XMLoadFloat4(&rotation);
		return XMMatrixScaling(scale.x, scale.y, scale.z) *
			XMMatrixRotationQuaternion(rotationQuaternion) *
			XMMatrixTranslation(translation.x, translation.y, translation.z);
	}

	XMVECTOR NormalizeOrDefault(XMVECTOR vector, XMVECTOR defaultVector)
	{
		const XMVECTOR length = XMVector3LengthSq(vector);
		if (XMVectorGetX(length) <= 0.0f)
		{
			return defaultVector;
		}

		return XMVector3Normalize(vector);
	}

	float GetInterpolationAmount(double fromTime, double toTime, double animationTimeTicks)
	{
		const double duration = toTime - fromTime;
		if (duration <= 0.0)
		{
			return 0.0f;
		}

		return static_cast<float>((animationTimeTicks - fromTime) / duration);
	}

	XMVECTOR SampleVectorKey(const std::vector<VectorAnimationKey>& keys, double animationTimeTicks, XMVECTOR fallback)
	{
		if (keys.empty())
		{
			return fallback;
		}

		if (keys.size() == 1 || animationTimeTicks <= keys.front().time)
		{
			return XMLoadFloat3(&keys.front().value);
		}

		for (size_t keyIndex = 1; keyIndex < keys.size(); ++keyIndex)
		{
			if (animationTimeTicks <= keys[keyIndex].time)
			{
				const XMVECTOR from = XMLoadFloat3(&keys[keyIndex - 1].value);
				const XMVECTOR to = XMLoadFloat3(&keys[keyIndex].value);
				return XMVectorLerp(from, to, GetInterpolationAmount(keys[keyIndex - 1].time, keys[keyIndex].time, animationTimeTicks));
			}
		}

		return XMLoadFloat3(&keys.back().value);
	}

	XMVECTOR SampleQuaternionKey(const std::vector<QuaternionAnimationKey>& keys, double animationTimeTicks, XMVECTOR fallback)
	{
		if (keys.empty())
		{
			return fallback;
		}

		if (keys.size() == 1 || animationTimeTicks <= keys.front().time)
		{
			return XMQuaternionNormalize(XMLoadFloat4(&keys.front().value));
		}

		for (size_t keyIndex = 1; keyIndex < keys.size(); ++keyIndex)
		{
			if (animationTimeTicks <= keys[keyIndex].time)
			{
				const XMVECTOR from = XMLoadFloat4(&keys[keyIndex - 1].value);
				const XMVECTOR to = XMLoadFloat4(&keys[keyIndex].value);
				return XMQuaternionNormalize(XMQuaternionSlerp(from, to, GetInterpolationAmount(keys[keyIndex - 1].time, keys[keyIndex].time, animationTimeTicks)));
			}
		}

		return XMQuaternionNormalize(XMLoadFloat4(&keys.back().value));
	}
}

void SkinnedModel::Initialize(ID3D12Device* device, const std::string& modelPath)
{
	SkinnedModelLoader loader;
	if (!loader.Load(modelPath, m_modelData))
	{
		throw std::runtime_error("Failed to load skinned model: " + loader.GetLastError());
	}

	FitModelToHeight();
	m_boneMatrices.resize(m_modelData.bones.size());

	std::unordered_map<std::string, std::shared_ptr<TexturedMaterial>> materialCache;
	m_meshParts.clear();
	m_meshParts.reserve(m_modelData.meshes.size());

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

		MeshPart meshPart;
		meshPart.sourceVertices = meshData.vertices;
		meshPart.skinnedVertices.resize(meshData.vertices.size());
		for (size_t index = 0; index < meshData.vertices.size(); ++index)
		{
			meshPart.skinnedVertices[index] = meshData.vertices[index].vertex;
		}

		meshPart.vertexBuffer.Initialize(device, meshPart.skinnedVertices);
		meshPart.material = material;
		m_meshParts.push_back(std::move(meshPart));
	}

	UpdateBoneMatrices();
	SkinMeshes();
}

void SkinnedModel::Update(float deltaTime)
{
	m_animationTimeSeconds += deltaTime;
	UpdateBoneMatrices();
	SkinMeshes();
}

void SkinnedModel::Draw(Dx12Renderer& renderer) const
{
	const XMMATRIX world = XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
	for (const MeshPart& meshPart : m_meshParts)
	{
		renderer.DrawTextured(meshPart.vertexBuffer, *meshPart.material, world);
	}
}

void SkinnedModel::SetPosition(float x, float y, float z)
{
	m_position = XMFLOAT3{ x, y, z };
}

void SkinnedModel::FitModelToHeight()
{
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
			minX = std::min(minX, vertex.vertex.position[0]);
			minY = std::min(minY, vertex.vertex.position[1]);
			minZ = std::min(minZ, vertex.vertex.position[2]);
			maxX = std::max(maxX, vertex.vertex.position[0]);
			maxY = std::max(maxY, vertex.vertex.position[1]);
			maxZ = std::max(maxZ, vertex.vertex.position[2]);
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
	m_modelScale = ModelHeight / height;
}

void SkinnedModel::UpdateBoneMatrices()
{
	std::vector<XMMATRIX> globalTransforms(m_modelData.bones.size(), XMMatrixIdentity());

	for (size_t boneIndex = 0; boneIndex < m_modelData.bones.size(); ++boneIndex)
	{
		const BoneData& bone = m_modelData.bones[boneIndex];
		XMMATRIX localTransform = GetLocalTransform(bone);

		if (!m_modelData.animations.empty())
		{
			const AnimationClip& clip = m_modelData.animations.front();
			for (const BoneAnimation& boneAnimation : clip.boneAnimations)
			{
				if (boneAnimation.boneIndex == static_cast<int>(boneIndex))
				{
					localTransform = GetAnimatedLocalTransform(clip, boneAnimation);
					break;
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

void SkinnedModel::SkinMeshes()
{
	for (MeshPart& meshPart : m_meshParts)
	{
		for (size_t vertexIndex = 0; vertexIndex < meshPart.sourceVertices.size(); ++vertexIndex)
		{
			const SkinnedVertex& sourceVertex = meshPart.sourceVertices[vertexIndex];
			TexturedVertex skinnedVertex = sourceVertex.vertex;

			XMVECTOR position = XMVectorZero();
			XMVECTOR normal = XMVectorZero();
			XMVECTOR tangent = XMVectorZero();
			float totalWeight = 0.0f;

			const XMVECTOR sourcePosition = XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(sourceVertex.vertex.position));
			const XMVECTOR sourceNormal = XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(sourceVertex.vertex.normal));
			const XMVECTOR sourceTangent = XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(sourceVertex.vertex.tangent));

			for (int slot = 0; slot < 4; ++slot)
			{
				const int boneIndex = sourceVertex.boneIndices[slot];
				const float weight = sourceVertex.boneWeights[slot];
				if (boneIndex < 0 || weight == 0.0f || boneIndex >= static_cast<int>(m_boneMatrices.size()))
				{
					continue;
				}

				const XMMATRIX boneMatrix = XMLoadFloat4x4(&m_boneMatrices[boneIndex]);
				position += XMVector3TransformCoord(sourcePosition, boneMatrix) * weight;
				normal += XMVector3TransformNormal(sourceNormal, boneMatrix) * weight;
				tangent += XMVector3TransformNormal(sourceTangent, boneMatrix) * weight;
				totalWeight += weight;
			}

			if (totalWeight == 0.0f)
			{
				position = sourcePosition;
				normal = sourceNormal;
				tangent = sourceTangent;
			}
			else if (totalWeight != 1.0f)
			{
				position /= totalWeight;
				normal /= totalWeight;
				tangent /= totalWeight;
			}

			position = XMVectorSet(
				(XMVectorGetX(position) - m_modelCenterX) * m_modelScale,
				(XMVectorGetY(position) - m_modelMinY) * m_modelScale,
				(XMVectorGetZ(position) - m_modelCenterZ) * m_modelScale,
				1.0f);

			XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(skinnedVertex.position), position);
			XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(skinnedVertex.normal), NormalizeOrDefault(normal, sourceNormal));
			XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(skinnedVertex.tangent), NormalizeOrDefault(tangent, sourceTangent));
			meshPart.skinnedVertices[vertexIndex] = skinnedVertex;
		}

		meshPart.vertexBuffer.Update(meshPart.skinnedVertices);
	}
}

XMMATRIX SkinnedModel::GetLocalTransform(const BoneData& bone) const
{
	return LoadMatrix(bone.localBindTransform);
}

XMMATRIX SkinnedModel::GetAnimatedLocalTransform(const AnimationClip& clip, const BoneAnimation& boneAnimation) const
{
	const double durationSeconds = clip.durationTicks / clip.ticksPerSecond;
	const double animationTime = durationSeconds > 0.0
		? std::fmod(m_animationTimeSeconds, durationSeconds) * clip.ticksPerSecond
		: 0.0;

	XMVECTOR bindScale = XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);
	XMVECTOR bindRotation = XMQuaternionIdentity();
	XMVECTOR bindTranslation = XMVectorZero();
	XMMatrixDecompose(&bindScale, &bindRotation, &bindTranslation, LoadMatrix(m_modelData.bones[boneAnimation.boneIndex].localBindTransform));

	const XMVECTOR translation = SampleVectorKey(boneAnimation.translations, animationTime, bindTranslation);
	const XMVECTOR rotation = SampleQuaternionKey(boneAnimation.rotations, animationTime, bindRotation);
	const XMVECTOR scale = SampleVectorKey(boneAnimation.scales, animationTime, bindScale);

	XMFLOAT3 translationFloat{};
	XMFLOAT4 rotationFloat{};
	XMFLOAT3 scaleFloat{};
	XMStoreFloat3(&translationFloat, translation);
	XMStoreFloat4(&rotationFloat, rotation);
	XMStoreFloat3(&scaleFloat, scale);
	return BuildTransform(translationFloat, rotationFloat, scaleFloat);
}
