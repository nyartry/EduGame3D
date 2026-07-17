#include "Framework/Models/SkinnedModelLoader.h"

#include "Framework/Animation/MeshTangentCalculator.h"
#include "Framework/Assets/AssetPathResolver.h"
#include "Framework/Models/ModelTextureResolver.h"
#include "Framework/Animation/RootMotionPolicy.h"

#include <assimp/Importer.hpp>
#include <assimp/anim.h>
#include <assimp/material.h>
#include <assimp/mesh.h>
#include <assimp/metadata.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <DirectXMath.h>
#include <Windows.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_map>

using namespace DirectX;

namespace
{
	void WriteDebugLog(const std::string& message)
	{
		OutputDebugStringA(message.c_str());
		OutputDebugStringA("\n");
	}

	std::string MetadataValueToString(const aiMetadataEntry& entry)
	{
		if (entry.mData == nullptr)
		{
			return "<null>";
		}

		std::ostringstream stream;
		switch (entry.mType)
		{
		case AI_BOOL:
			stream << (*static_cast<bool*>(entry.mData) ? "true" : "false");
			break;
		case AI_INT32:
			stream << *static_cast<int32_t*>(entry.mData);
			break;
		case AI_UINT32:
			stream << *static_cast<uint32_t*>(entry.mData);
			break;
		case AI_INT64:
			stream << *static_cast<int64_t*>(entry.mData);
			break;
		case AI_UINT64:
			stream << *static_cast<uint64_t*>(entry.mData);
			break;
		case AI_FLOAT:
			stream << *static_cast<float*>(entry.mData);
			break;
		case AI_DOUBLE:
			stream << *static_cast<double*>(entry.mData);
			break;
		case AI_AISTRING:
			stream << static_cast<aiString*>(entry.mData)->C_Str();
			break;
		case AI_AIVECTOR3D:
		{
			const aiVector3D& value = *static_cast<aiVector3D*>(entry.mData);
			stream << "(" << value.x << ", " << value.y << ", " << value.z << ")";
			break;
		}
		default:
			stream << "<type " << entry.mType << ">";
			break;
		}
		return stream.str();
	}

	bool IsNearlyOne(float value)
	{
		return std::fabs(value - 1.0f) < 0.001f;
	}

	bool HasNonUnitScale(const aiVector3D& scale)
	{
		return !IsNearlyOne(scale.x) || !IsNearlyOne(scale.y) || !IsNearlyOne(scale.z);
	}

	void LogNodeScaleDiagnostics(const aiNode* node, int depth = 0)
	{
		if (node == nullptr)
		{
			return;
		}

		aiVector3D scale;
		aiVector3D rotation;
		aiVector3D translation;
		node->mTransformation.Decompose(scale, rotation, translation);
		if (HasNonUnitScale(scale))
		{
			std::ostringstream stream;
			stream << "[SkinnedModelLoader] nodeScale depth=" << depth
				<< " name=" << node->mName.C_Str()
				<< " scale=(" << scale.x << ", " << scale.y << ", " << scale.z << ")"
				<< " translation=(" << translation.x << ", " << translation.y << ", " << translation.z << ")";
			WriteDebugLog(stream.str());
		}

		for (unsigned int childIndex = 0; childIndex < node->mNumChildren; ++childIndex)
		{
			LogNodeScaleDiagnostics(node->mChildren[childIndex], depth + 1);
		}
	}

	void LogAnimationScaleDiagnostics(const aiScene* scene)
	{
		if (scene == nullptr)
		{
			return;
		}

		for (unsigned int animationIndex = 0; animationIndex < scene->mNumAnimations; ++animationIndex)
		{
			const aiAnimation* animation = scene->mAnimations[animationIndex];
			for (unsigned int channelIndex = 0; channelIndex < animation->mNumChannels; ++channelIndex)
			{
				const aiNodeAnim* channel = animation->mChannels[channelIndex];
				if (channel == nullptr || channel->mNumScalingKeys == 0)
				{
					continue;
				}

				aiVector3D minScale{
					std::numeric_limits<float>::max(),
					std::numeric_limits<float>::max(),
					std::numeric_limits<float>::max()
				};
				aiVector3D maxScale{
					std::numeric_limits<float>::lowest(),
					std::numeric_limits<float>::lowest(),
					std::numeric_limits<float>::lowest()
				};

				for (unsigned int keyIndex = 0; keyIndex < channel->mNumScalingKeys; ++keyIndex)
				{
					const aiVector3D& scale = channel->mScalingKeys[keyIndex].mValue;
					minScale.x = std::min(minScale.x, scale.x);
					minScale.y = std::min(minScale.y, scale.y);
					minScale.z = std::min(minScale.z, scale.z);
					maxScale.x = std::max(maxScale.x, scale.x);
					maxScale.y = std::max(maxScale.y, scale.y);
					maxScale.z = std::max(maxScale.z, scale.z);
				}

				if (HasNonUnitScale(minScale) || HasNonUnitScale(maxScale))
				{
					std::ostringstream stream;
					stream << "[SkinnedModelLoader] animationScale animation=" << animationIndex
						<< " channel=" << channel->mNodeName.C_Str()
						<< " keys=" << channel->mNumScalingKeys
						<< " min=(" << minScale.x << ", " << minScale.y << ", " << minScale.z << ")"
						<< " max=(" << maxScale.x << ", " << maxScale.y << ", " << maxScale.z << ")";
					WriteDebugLog(stream.str());
				}
			}
		}
	}

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

	void LogSceneScaleDiagnostics(const std::string& filePath, const aiScene* scene)
	{
		if (scene == nullptr)
		{
			return;
		}

		std::ostringstream stream;
		stream << "[SkinnedModelLoader] file=" << filePath
			<< " meshes=" << scene->mNumMeshes
			<< " animations=" << scene->mNumAnimations;
		WriteDebugLog(stream.str());

		if (scene->mRootNode != nullptr)
		{
			const aiMatrix4x4& transform = scene->mRootNode->mTransformation;
			std::ostringstream rootStream;
			rootStream << "[SkinnedModelLoader] rootTransform="
				<< "[" << transform.a1 << ", " << transform.a2 << ", " << transform.a3 << ", " << transform.a4 << "] "
				<< "[" << transform.b1 << ", " << transform.b2 << ", " << transform.b3 << ", " << transform.b4 << "] "
				<< "[" << transform.c1 << ", " << transform.c2 << ", " << transform.c3 << ", " << transform.c4 << "] "
				<< "[" << transform.d1 << ", " << transform.d2 << ", " << transform.d3 << ", " << transform.d4 << "]";
			WriteDebugLog(rootStream.str());
			LogNodeScaleDiagnostics(scene->mRootNode);
		}

		if (scene->mMetaData != nullptr)
		{
			for (unsigned int metadataIndex = 0; metadataIndex < scene->mMetaData->mNumProperties; ++metadataIndex)
			{
				std::ostringstream metadataStream;
				metadataStream << "[SkinnedModelLoader] metadata "
					<< scene->mMetaData->mKeys[metadataIndex].C_Str()
					<< "="
					<< MetadataValueToString(scene->mMetaData->mValues[metadataIndex]);
				WriteDebugLog(metadataStream.str());
			}
		}

		for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
		{
			const aiMesh* mesh = scene->mMeshes[meshIndex];
			if (mesh == nullptr || mesh->mNumVertices == 0)
			{
				continue;
			}

			aiVector3D minPosition{
				std::numeric_limits<float>::max(),
				std::numeric_limits<float>::max(),
				std::numeric_limits<float>::max()
			};
			aiVector3D maxPosition{
				std::numeric_limits<float>::lowest(),
				std::numeric_limits<float>::lowest(),
				std::numeric_limits<float>::lowest()
			};

			for (unsigned int vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex)
			{
				const aiVector3D& position = mesh->mVertices[vertexIndex];
				minPosition.x = std::min(minPosition.x, position.x);
				minPosition.y = std::min(minPosition.y, position.y);
				minPosition.z = std::min(minPosition.z, position.z);
				maxPosition.x = std::max(maxPosition.x, position.x);
				maxPosition.y = std::max(maxPosition.y, position.y);
				maxPosition.z = std::max(maxPosition.z, position.z);
			}

			std::ostringstream meshStream;
			meshStream << "[SkinnedModelLoader] mesh=" << meshIndex
				<< " name=" << mesh->mName.C_Str()
				<< " vertices=" << mesh->mNumVertices
				<< " min=(" << minPosition.x << ", " << minPosition.y << ", " << minPosition.z << ")"
				<< " max=(" << maxPosition.x << ", " << maxPosition.y << ", " << maxPosition.z << ")"
				<< " size=("
				<< maxPosition.x - minPosition.x << ", "
				<< maxPosition.y - minPosition.y << ", "
				<< maxPosition.z - minPosition.z << ")";
			WriteDebugLog(meshStream.str());
		}

		LogAnimationScaleDiagnostics(scene);
	}

	XMFLOAT4 GetMaterialColor(const aiMaterial* material)
	{
		if (material == nullptr)
		{
			return XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f };
		}

		aiColor4D color;
		if (AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_BASE_COLOR, &color) ||
			AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &color))
		{
			return XMFLOAT4{ color.r, color.g, color.b, color.a };
		}

		return XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f };
	}

	TexturedVertex MakeTexturedVertex(const aiMesh* mesh, unsigned int vertexIndex, const XMFLOAT4& materialColor)
	{
		TexturedVertex vertex
		{
			XMFLOAT3{ mesh->mVertices[vertexIndex].x, mesh->mVertices[vertexIndex].y, mesh->mVertices[vertexIndex].z },
			XMFLOAT3{ 0.0f, 1.0f, 0.0f },
			XMFLOAT3{ 1.0f, 0.0f, 0.0f },
			XMFLOAT2{ 0.0f, 0.0f },
			materialColor
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
				mesh->mTextureCoords[0][vertexIndex].y
			};
		}

		if (mesh->HasVertexColors(0))
		{
			vertex.color = XMFLOAT4
			{
				materialColor.x * mesh->mColors[0][vertexIndex].r,
				materialColor.y * mesh->mColors[0][vertexIndex].g,
				materialColor.z * mesh->mColors[0][vertexIndex].b,
				materialColor.w * mesh->mColors[0][vertexIndex].a
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
		int rootMotionPriority = 0;

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
			const int boneRootMotionPriority = RootMotionPolicy::GetRootMotionPriority(
				boneName,
				boneAnimation.boneIndex,
				bones);
			boneAnimation.lockTranslationToBindPose = boneRootMotionPriority > 0;

			clip.boneAnimationIndicesByBone[boneAnimation.boneIndex] = static_cast<int>(clip.boneAnimations.size());
			if (boneRootMotionPriority > rootMotionPriority)
			{
				rootMotionPriority = boneRootMotionPriority;
				clip.rootMotionBoneAnimationIndex = static_cast<int>(clip.boneAnimations.size());
			}
			clip.boneAnimations.push_back(std::move(boneAnimation));
		}

		return clip;
	}
}

bool SkinnedModelLoader::Load(const std::string& filePath, SkinnedModelData& modelData)
{
	const std::filesystem::path resolvedFilePath = AssetPathResolver::Resolve(filePath);
	const std::string resolvedPath = AssetPathResolver::ResolveUtf8(filePath);
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(
		resolvedPath,
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

	LogSceneScaleDiagnostics(resolvedPath, scene);

	modelData = SkinnedModelData{};
	aiMatrix4x4 rootInverseTransform = scene->mRootNode->mTransformation;
	rootInverseTransform.Inverse();
	modelData.rootInverseTransform = ToFloat4x4(rootInverseTransform);

	std::unordered_map<std::string, int> boneMap;
	AddBoneRecursive(scene->mRootNode, -1, modelData.bones, boneMap);

	const std::filesystem::path modelDirectory = resolvedFilePath.parent_path();
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
		const XMFLOAT4 materialColor = GetMaterialColor(material);

		std::vector<SkinnedVertex> sourceVertices(mesh->mNumVertices);
		for (unsigned int vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex)
		{
			sourceVertices[vertexIndex].vertex = MakeTexturedVertex(mesh, vertexIndex, materialColor);
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

		if (!meshData.vertices.empty())
		{
			modelData.meshes.push_back(std::move(meshData));
		}
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
	const std::string resolvedPath = AssetPathResolver::ResolveUtf8(filePath);
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(
		resolvedPath,
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
