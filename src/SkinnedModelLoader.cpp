#include "SkinnedModelLoader.h"

#include "MeshTangentCalculator.h"
#include "ModelTextureResolver.h"

#include <assimp/Importer.hpp>
#include <assimp/anim.h>
#include <assimp/material.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <DirectXMath.h>

#include <filesystem>
#include <unordered_map>

using namespace DirectX;

namespace
{
	XMFLOAT4X4 ToFloat4x4(const aiMatrix4x4& matrix)
	{
		return XMFLOAT4X4
		{
			matrix.a1, matrix.a2, matrix.a3, matrix.a4,
			matrix.b1, matrix.b2, matrix.b3, matrix.b4,
			matrix.c1, matrix.c2, matrix.c3, matrix.c4,
			matrix.d1, matrix.d2, matrix.d3, matrix.d4
		};
	}

	TexturedVertex MakeTexturedVertex(const aiMesh* mesh, unsigned int vertexIndex)
	{
		TexturedVertex vertex
		{
			{ mesh->mVertices[vertexIndex].x, mesh->mVertices[vertexIndex].y, mesh->mVertices[vertexIndex].z },
			{ 0.0f, 1.0f, 0.0f },
			{ 1.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f }
		};

		if (mesh->HasNormals())
		{
			vertex.normal[0] = mesh->mNormals[vertexIndex].x;
			vertex.normal[1] = mesh->mNormals[vertexIndex].y;
			vertex.normal[2] = mesh->mNormals[vertexIndex].z;
		}

		if (mesh->HasTextureCoords(0))
		{
			vertex.uv[0] = mesh->mTextureCoords[0][vertexIndex].x;
			vertex.uv[1] = 1.0f - mesh->mTextureCoords[0][vertexIndex].y;
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

	AnimationKey BuildKey(const aiNodeAnim* channel, unsigned int keyIndex)
	{
		AnimationKey key;
		key.time = channel->mPositionKeys[keyIndex].mTime;

		const aiVector3D position = channel->mPositionKeys[keyIndex].mValue;
		key.translation = XMFLOAT3{ position.x, position.y, position.z };

		if (keyIndex < channel->mNumRotationKeys)
		{
			const aiQuaternion rotation = channel->mRotationKeys[keyIndex].mValue;
			key.rotation = XMFLOAT4{ rotation.x, rotation.y, rotation.z, rotation.w };
		}

		if (keyIndex < channel->mNumScalingKeys)
		{
			const aiVector3D scale = channel->mScalingKeys[keyIndex].mValue;
			key.scale = XMFLOAT3{ scale.x, scale.y, scale.z };
		}

		return key;
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
				vertex.vertex.tangent[0] = triangleVertices[index].tangent[0];
				vertex.vertex.tangent[1] = triangleVertices[index].tangent[1];
				vertex.vertex.tangent[2] = triangleVertices[index].tangent[2];
				meshData.vertices.push_back(vertex);
			}
		}

		modelData.meshes.push_back(std::move(meshData));
	}

	for (unsigned int animationIndex = 0; animationIndex < scene->mNumAnimations; ++animationIndex)
	{
		const aiAnimation* aiAnimation = scene->mAnimations[animationIndex];
		AnimationClip clip;
		clip.durationTicks = aiAnimation->mDuration;
		clip.ticksPerSecond = aiAnimation->mTicksPerSecond == 0.0 ? 30.0 : aiAnimation->mTicksPerSecond;

		for (unsigned int channelIndex = 0; channelIndex < aiAnimation->mNumChannels; ++channelIndex)
		{
			const aiNodeAnim* channel = aiAnimation->mChannels[channelIndex];
			BoneAnimation boneAnimation;
			boneAnimation.boneIndex = FindBoneIndex(boneMap, channel->mNodeName.C_Str());
			if (boneAnimation.boneIndex < 0)
			{
				continue;
			}

			const unsigned int keyCount = channel->mNumPositionKeys;
			for (unsigned int keyIndex = 0; keyIndex < keyCount; ++keyIndex)
			{
				boneAnimation.keys.push_back(BuildKey(channel, keyIndex));
			}

			clip.boneAnimations.push_back(std::move(boneAnimation));
		}

		modelData.animations.push_back(std::move(clip));
	}

	if (modelData.meshes.empty())
	{
		m_lastError = "Skinned model has no drawable meshes.";
		return false;
	}

	m_lastError.clear();
	return true;
}

std::string SkinnedModelLoader::GetLastError() const
{
	return m_lastError;
}
