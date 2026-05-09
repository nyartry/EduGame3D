#include "ModelLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <algorithm>
#include <cctype>
#include <filesystem>

namespace
{
	TexturedVertex MakeTexturedVertex(const aiMesh* mesh, unsigned int vertexIndex)
	{
		TexturedVertex vertex
		{
			{ mesh->mVertices[vertexIndex].x, mesh->mVertices[vertexIndex].y, mesh->mVertices[vertexIndex].z },
			{ 0.0f, 1.0f, 0.0f },
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

	std::string ToLower(std::string text)
	{
		std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c)
		{
			return static_cast<char>(std::tolower(c));
		});
		return text;
	}

	std::string FindExistingTexturePath(const std::filesystem::path& modelDirectory, const std::string& texturePath)
	{
		if (texturePath.empty() || texturePath[0] == '*')
		{
			return {};
		}

		const std::filesystem::path sourcePath = texturePath;
		const std::filesystem::path directPath = sourcePath.is_absolute()
			? sourcePath
			: modelDirectory / sourcePath;
		if (std::filesystem::exists(directPath))
		{
			return directPath.string();
		}

		const std::filesystem::path textureDirectory = modelDirectory / "textures";
		const std::filesystem::path fileName = sourcePath.filename();
		const std::filesystem::path siblingPath = textureDirectory / fileName;
		if (std::filesystem::exists(siblingPath))
		{
			return siblingPath.string();
		}

		if (std::filesystem::exists(textureDirectory))
		{
			const std::string lowerFileName = ToLower(fileName.string());
			for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(textureDirectory))
			{
				if (ToLower(entry.path().filename().string()) == lowerFileName)
				{
					return entry.path().string();
				}
			}
		}

		return {};
	}

	std::string GetMaterialTexturePath(
		const aiScene* scene,
		unsigned int materialIndex,
		const std::filesystem::path& modelDirectory,
		const std::vector<aiTextureType>& textureTypes)
	{
		if (scene == nullptr || materialIndex >= scene->mNumMaterials)
		{
			return {};
		}

		const aiMaterial* material = scene->mMaterials[materialIndex];
		for (const aiTextureType textureType : textureTypes)
		{
			if (material->GetTextureCount(textureType) == 0)
			{
				continue;
			}

			aiString texturePath;
			if (AI_SUCCESS == material->GetTexture(textureType, 0, &texturePath))
			{
				const std::string resolvedPath = FindExistingTexturePath(modelDirectory, texturePath.C_Str());
				if (!resolvedPath.empty())
				{
					return resolvedPath;
				}
			}
		}

		return {};
	}
}

bool ModelLoader::Load(const std::string& filePath, ModelData& modelData)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(
		filePath,
		aiProcess_Triangulate |
			aiProcess_JoinIdenticalVertices |
			aiProcess_ConvertToLeftHanded |
			aiProcess_PreTransformVertices |
			aiProcess_GenSmoothNormals);

	if (scene == nullptr)
	{
		m_lastError = importer.GetErrorString();
		return false;
	}

	modelData.texturedMeshes.clear();

	const std::filesystem::path modelDirectory = std::filesystem::path(filePath).parent_path();

	for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
	{
		const aiMesh* mesh = scene->mMeshes[meshIndex];
		TexturedMeshData texturedMesh;
		texturedMesh.baseColorTexturePath = GetMaterialTexturePath(
			scene,
			mesh->mMaterialIndex,
			modelDirectory,
			{ aiTextureType_BASE_COLOR, aiTextureType_DIFFUSE });
		texturedMesh.opacityTexturePath = GetMaterialTexturePath(
			scene,
			mesh->mMaterialIndex,
			modelDirectory,
			{ aiTextureType_OPACITY });

		for (unsigned int faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex)
		{
			const aiFace& face = mesh->mFaces[faceIndex];
			if (face.mNumIndices != 3)
			{
				continue;
			}

			for (unsigned int index = 0; index < face.mNumIndices; ++index)
			{
				const unsigned int vertexIndex = face.mIndices[index];
				texturedMesh.vertices.push_back(MakeTexturedVertex(mesh, vertexIndex));
			}
		}

		if (!texturedMesh.vertices.empty())
		{
			modelData.texturedMeshes.push_back(std::move(texturedMesh));
		}
	}

	if (modelData.texturedMeshes.empty())
	{
		m_lastError = "Model has no drawable triangle vertices.";
		return false;
	}

	m_lastError.clear();
	return true;
}

std::string ModelLoader::GetLastError() const
{
	return m_lastError;
}
