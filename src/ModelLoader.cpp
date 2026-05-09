#include "ModelLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>

namespace
{
	struct TriangleVertex
	{
		TexturedVertex vertex;
	};

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

	void ApplyTriangleTangents(TriangleVertex (&triangleVertices)[3])
	{
		const TexturedVertex& v0 = triangleVertices[0].vertex;
		const TexturedVertex& v1 = triangleVertices[1].vertex;
		const TexturedVertex& v2 = triangleVertices[2].vertex;

		const float edge1[] =
		{
			v1.position[0] - v0.position[0],
			v1.position[1] - v0.position[1],
			v1.position[2] - v0.position[2],
		};
		const float edge2[] =
		{
			v2.position[0] - v0.position[0],
			v2.position[1] - v0.position[1],
			v2.position[2] - v0.position[2],
		};

		const float deltaUv1[] = { v1.uv[0] - v0.uv[0], v1.uv[1] - v0.uv[1] };
		const float deltaUv2[] = { v2.uv[0] - v0.uv[0], v2.uv[1] - v0.uv[1] };
		const float denominator = deltaUv1[0] * deltaUv2[1] - deltaUv2[0] * deltaUv1[1];
		if (std::abs(denominator) < 0.000001f)
		{
			return;
		}

		const float scale = 1.0f / denominator;
		const float tangent[] =
		{
			(edge1[0] * deltaUv2[1] - edge2[0] * deltaUv1[1]) * scale,
			(edge1[1] * deltaUv2[1] - edge2[1] * deltaUv1[1]) * scale,
			(edge1[2] * deltaUv2[1] - edge2[2] * deltaUv1[1]) * scale,
		};

		for (TriangleVertex& triangleVertex : triangleVertices)
		{
			triangleVertex.vertex.tangent[0] = tangent[0];
			triangleVertex.vertex.tangent[1] = tangent[1];
			triangleVertex.vertex.tangent[2] = tangent[2];
		}
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
		texturedMesh.normalTexturePath = GetMaterialTexturePath(
			scene,
			mesh->mMaterialIndex,
			modelDirectory,
			{ aiTextureType_NORMALS, aiTextureType_NORMAL_CAMERA, aiTextureType_HEIGHT });

		for (unsigned int faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex)
		{
			const aiFace& face = mesh->mFaces[faceIndex];
			if (face.mNumIndices != 3)
			{
				continue;
			}

			TriangleVertex triangleVertices[3]{};
			for (unsigned int index = 0; index < face.mNumIndices; ++index)
			{
				const unsigned int vertexIndex = face.mIndices[index];
				triangleVertices[index].vertex = MakeTexturedVertex(mesh, vertexIndex);
			}

			ApplyTriangleTangents(triangleVertices);
			for (const TriangleVertex& triangleVertex : triangleVertices)
			{
				texturedMesh.vertices.push_back(triangleVertex.vertex);
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
