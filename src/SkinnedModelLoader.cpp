#include "SkinnedModelLoader.h"

#include "MeshTangentCalculator.h"
#include "ModelTextureResolver.h"
#include "RootMotionPolicy.h"

#include <assimp/Importer.hpp>
#include <assimp/anim.h>
#include <assimp/material.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <DirectXMath.h>

#include <filesystem>
#include <string>
#include <unordered_map>

using namespace DirectX;

namespace
{
	XMFLOAT4X4 ToFloat4x4(const aiMatrix4x4& matrix)
	{
		return XMFLOAT4X4
		{
			matrix.a1, matrix.b1, matrix.c1, matrix.d1,
			matrix.a2, matrix.b2, matrix.c2, matrix.d2,
			matrix.a3, matrix.b3, matrix.c3, matrix.d3,
			matrix.a4, matrix.b4, matrix.c4, matrix.d4
		};
	}

	TexturedVertex MakeTexturedVertex(const aiMesh* mesh, unsigned int vertexIndex)
	{
		TexturedVertex vertex
		{
			XMFLOAT3{ mesh->mVertices[vertexIndex].x, mesh->mVertices[vertexIndex].y, mesh->mVertices[vertexIndex].z },
			XMFLOAT3{ 0.0f, 1.0f, 0.0f },
			XMFLOAT3{ 1.0f, 0.0f, 0.0f },
			XMFLOAT2{ 0.0f, 0.0f }
		};

		if (mesh->HasNormals())
		{
			vertex.normal = XMFLOAT3
			{
				mesh->mNormals[vertexIndex].x,
				mesh->mNormals[vertexIndex].y,
				mesh->mNormals[vertexIndex].z
			};
		}

		if (mesh->HasTextureCoords(0))
		{
			vertex.uv = XMFLOAT2
			{
				mesh->mTextureCoords[0][vertexIndex].x,
				1.0f - mesh->mTextureCoords[0][vertexIndex].y
			};
		}

		return vertex;
	}

	void AddBoneWeight(SkinnedVertex& vertex, int boneIndex, float weight)
	{
		for (int slot = 0; slot < 4; ++slot)
		{
			if (vertex.boneWeights[slot] == 0.0f)
			{
				vertex.boneIndices[slot] = boneIndex;
				vertex.boneWeights[slot] = weight;
				return;
			}
		}
	}

	int FindBoneIndex(const std::unordered_map<std::string, int>& boneMap, const std::string& name)
	{
		const auto found = boneMap.find(name);
		return found == boneMap.end() ? -1 : found->second;
	}

	int AddBoneRecursive(
		const aiNode* node,
		int parentIndex,
		std::vector<BoneData>& bones,
		std::unordered_map<std::string, int>& boneMap)
	{
		const std::string boneName = node->mName.C_Str();
		int boneIndex = FindBoneIndex(boneMap, boneName);
		if (boneIndex < 0)
		{
			boneIndex = static_cast<int>(bones.size());
			boneMap[boneName] = boneIndex;
			BoneData bone;
			bone.name = boneName;
			bone.parentIndex = parentIndex;
			bone.offsetMatrix = ToFloat4x4(aiMatrix4x4{});
			bone.localBindTransform = ToFloat4x4(node->mTransformation);
			bones.push_back(bone);
		}
		else
		{
			bones[boneIndex].parentIndex = parentIndex;
			bones[boneIndex].localBindTransform = ToFloat4x4(node->mTransformation);
		}

		for (unsigned int childIndex = 0; childIndex < node->mNumChildren; ++childIndex)
		{
			AddBoneRecursive(node->mChildren[childIndex], boneIndex, bones, boneMap);
		}

		return boneIndex;
	}

	void AddPositionKeys(const aiNodeAnim* channel, std::vector<VectorAnimationKey>& keys)
	{
		keys.reserve(channel->mNumPositionKeys);
		for (unsigned int keyIndex = 0; keyIndex < channel->mNumPositionKeys; ++keyIndex)
		{
			const aiVectorKey& sourceKey = channel->mPositionKeys[keyIndex];
			keys.push_back(VectorAnimationKey
				{
					sourceKey.mTime,
					XMFLOAT3{ sourceKey.mValue.x, sourceKey.mValue.y, sourceKey.mValue.z }
				});
		}
	}

	void AddRotationKeys(const aiNodeAnim* channel, std::vector<QuaternionAnimationKey>& keys)
	{
		keys.reserve(channel->mNumRotationKeys);
		for (unsigned int keyIndex = 0; keyIndex < channel->mNumRotationKeys; ++keyIndex)
		{
			const aiQuatKey& sourceKey = channel->mRotationKeys[keyIndex];
			keys.push_back(QuaternionAnimationKey
				{
					sourceKey.mTime,
					XMFLOAT4{ sourceKey.mValue.x, sourceKey.mValue.y, sourceKey.mValue.z, sourceKey.mValue.w }
				});
		}
	}

	void AddScaleKeys(const aiNodeAnim* channel, std::vector<VectorAnimationKey>& keys)
	{
		keys.reserve(channel->mNumScalingKeys);
		for (unsigned int keyIndex = 0; keyIndex < channel->mNumScalingKeys; ++keyIndex)
		{
			const aiVectorKey& sourceKey = channel->mScalingKeys[keyIndex];
			keys.push_back(VectorAnimationKey
				{
					sourceKey.mTime,
					XMFLOAT3{ sourceKey.mValue.x, sourceKey.mValue.y, sourceKey.mValue.z }
				});
		}
	}

	std::unordered_map<std::string, int> BuildBoneIndexMap(const std::vector<BoneData>& bones)
	{
		std::unordered_map<std::string, int> boneMap;
		for (size_t boneIndex = 0; boneIndex < bones.size(); ++boneIndex)
		{
			boneMap[bones[boneIndex].name] = static_cast<int>(boneIndex);
		}

		return boneMap;
	}

	AnimationClip BuildAnimationClip(
		const aiAnimation* aiAnimation,
		const std::string& fallbackName,
		const std::unordered_map<std::string, int>& boneMap,
		const std::vector<BoneData>& bones)
	{
		AnimationClip clip;
		clip.name = fallbackName;
		if (clip.name.empty())
		{
			clip.name = aiAnimation->mName.C_Str();
		}
		clip.durationTicks = aiAnimation->mDuration;
		clip.ticksPerSecond = aiAnimation->mTicksPerSecond == 0.0 ? 30.0 : aiAnimation->mTicksPerSecond;
		clip.boneAnimationIndicesByBone.assign(bones.size(), -1);

		for (unsigned int channelIndex = 0; channelIndex < aiAnimation->mNumChannels; ++channelIndex)
		{
			const aiNodeAnim* channel = aiAnimation->mChannels[channelIndex];
			const std::string boneName = channel->mNodeName.C_Str();
			BoneAnimation boneAnimation;
			boneAnimation.boneName = boneName;
			boneAnimation.boneIndex = FindBoneIndex(boneMap, boneName);
			if (boneAnimation.boneIndex < 0)
			{
				continue;
			}

			AddPositionKeys(channel, boneAnimation.translations);
			AddRotationKeys(channel, boneAnimation.rotations);
			AddScaleKeys(channel, boneAnimation.scales);
			boneAnimation.lockTranslationToBindPose = RootMotionPolicy::ShouldLockTranslationToBindPose(
				boneName,
				boneAnimation.boneIndex,
				bones);

			clip.boneAnimationIndicesByBone[boneAnimation.boneIndex] = static_cast<int>(clip.boneAnimations.size());
			clip.boneAnimations.push_back(std::move(boneAnimation));
		}

		return clip;
	}
}

bool SkinnedModelLoader::Load(const std::string& filePath, SkinnedModelData& modelData)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(
		filePath,
			aiProcess_Triangulate |
			aiProcess_JoinIdenticalVertices |
			aiProcess_LimitBoneWeights |
			aiProcess_ConvertToLeftHanded |
			aiProcess_GenSmoothNormals);

	if (scene == nullptr)
	{
		m_lastError = importer.GetErrorString();
		return false;
	}

	modelData = SkinnedModelData{};
	aiMatrix4x4 rootInverseTransform = scene->mRootNode->mTransformation;
	rootInverseTransform.Inverse();
	modelData.rootInverseTransform = ToFloat4x4(rootInverseTransform);

	std::unordered_map<std::string, int> boneMap;
	AddBoneRecursive(scene->mRootNode, -1, modelData.bones, boneMap);

	const std::filesystem::path modelDirectory = std::filesystem::path(filePath).parent_path();
	const ModelTextureResolver textureResolver(modelDirectory);

	for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
	{
		const aiMesh* mesh = scene->mMeshes[meshIndex];
		const aiMaterial* material = mesh->mMaterialIndex < scene->mNumMaterials
			? scene->mMaterials[mesh->mMaterialIndex]
			: nullptr;

		SkinnedMeshData meshData;
		meshData.baseColorTexturePath = textureResolver.FindTexture(material, { aiTextureType_BASE_COLOR, aiTextureType_DIFFUSE });
		meshData.opacityTexturePath = textureResolver.FindTexture(material, { aiTextureType_OPACITY });
		meshData.normalTexturePath = textureResolver.FindTexture(material, { aiTextureType_NORMALS, aiTextureType_NORMAL_CAMERA, aiTextureType_HEIGHT });

		std::vector<SkinnedVertex> sourceVertices(mesh->mNumVertices);
		for (unsigned int vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex)
		{
			sourceVertices[vertexIndex].vertex = MakeTexturedVertex(mesh, vertexIndex);
		}

		for (unsigned int meshBoneIndex = 0; meshBoneIndex < mesh->mNumBones; ++meshBoneIndex)
		{
			const aiBone* meshBone = mesh->mBones[meshBoneIndex];
			const std::string boneName = meshBone->mName.C_Str();
			const int boneIndex = FindBoneIndex(boneMap, boneName);
			if (boneIndex < 0)
			{
				continue;
			}

			modelData.bones[boneIndex].offsetMatrix = ToFloat4x4(meshBone->mOffsetMatrix);
			for (unsigned int weightIndex = 0; weightIndex < meshBone->mNumWeights; ++weightIndex)
			{
				const aiVertexWeight& weight = meshBone->mWeights[weightIndex];
				if (weight.mVertexId < sourceVertices.size())
				{
					AddBoneWeight(sourceVertices[weight.mVertexId], boneIndex, weight.mWeight);
				}
			}
		}

		for (unsigned int faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex)
		{
			const aiFace& face = mesh->mFaces[faceIndex];
			if (face.mNumIndices != 3)
			{
				continue;
			}

			TexturedVertex triangleVertices[3]{};
			for (unsigned int index = 0; index < face.mNumIndices; ++index)
			{
				triangleVertices[index] = sourceVertices[face.mIndices[index]].vertex;
			}
			MeshTangentCalculator::ApplyToTriangle(triangleVertices);

			for (unsigned int index = 0; index < face.mNumIndices; ++index)
			{
				SkinnedVertex vertex = sourceVertices[face.mIndices[index]];
				vertex.vertex.tangent = triangleVertices[index].tangent;
				meshData.vertices.push_back(vertex);
			}
		}

		modelData.meshes.push_back(std::move(meshData));
	}

	for (unsigned int animationIndex = 0; animationIndex < scene->mNumAnimations; ++animationIndex)
	{
		const aiAnimation* aiAnimation = scene->mAnimations[animationIndex];
		modelData.animations.push_back(BuildAnimationClip(aiAnimation, aiAnimation->mName.C_Str(), boneMap, modelData.bones));
	}

	if (modelData.meshes.empty())
	{
		m_lastError = "Skinned model has no drawable meshes.";
		return false;
	}

	m_lastError.clear();
	return true;
}

bool SkinnedModelLoader::LoadAnimation(const std::string& filePath, const std::string& animationName, SkinnedModelData& modelData)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(
		filePath,
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_LimitBoneWeights |
		aiProcess_ConvertToLeftHanded |
		aiProcess_GenSmoothNormals);

	if (scene == nullptr)
	{
		m_lastError = importer.GetErrorString();
		return false;
	}

	if (scene->mNumAnimations == 0)
	{
		m_lastError = "Model has no animations: " + filePath;
		return false;
	}

	const std::unordered_map<std::string, int> boneMap = BuildBoneIndexMap(modelData.bones);
	const size_t animationCountBeforeLoad = modelData.animations.size();
	for (unsigned int animationIndex = 0; animationIndex < scene->mNumAnimations; ++animationIndex)
	{
		const aiAnimation* aiAnimation = scene->mAnimations[animationIndex];
		AnimationClip clip = BuildAnimationClip(aiAnimation, animationName, boneMap, modelData.bones);
		if (!clip.boneAnimations.empty())
		{
			modelData.animations.push_back(std::move(clip));
		}
	}

	if (modelData.animations.size() == animationCountBeforeLoad)
	{
		m_lastError = "Animation bones did not match the loaded model skeleton: " + filePath;
		return false;
	}

	m_lastError.clear();
	return true;
}

std::string SkinnedModelLoader::GetLastError() const
{
	return m_lastError;
}
