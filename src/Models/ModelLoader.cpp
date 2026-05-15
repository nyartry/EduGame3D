#include "Models/ModelLoader.h"

#include "Animation/MeshTangentCalculator.h"
#include "Models/ModelTextureResolver.h"

#include <assimp/Importer.hpp>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <DirectXMath.h>

#include <filesystem>

using namespace DirectX;

namespace
{
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
				mesh->mTextureCoords[0][vertexIndex].y
			};
		}

		return vertex;
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
	const ModelTextureResolver textureResolver(modelDirectory);

	for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
	{
		const aiMesh* mesh = scene->mMeshes[meshIndex];
		const aiMaterial* material = mesh->mMaterialIndex < scene->mNumMaterials
			? scene->mMaterials[mesh->mMaterialIndex]
			: nullptr;
		TexturedMeshData texturedMesh;
		texturedMesh.baseColorTexturePath = textureResolver.FindTexture(
			material,
			{ aiTextureType_BASE_COLOR, aiTextureType_DIFFUSE });
		texturedMesh.opacityTexturePath = textureResolver.FindTexture(
			material,
			{ aiTextureType_OPACITY });
		texturedMesh.normalTexturePath = textureResolver.FindTexture(
			material,
			{ aiTextureType_NORMALS, aiTextureType_NORMAL_CAMERA, aiTextureType_HEIGHT });

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
				const unsigned int vertexIndex = face.mIndices[index];
				triangleVertices[index] = MakeTexturedVertex(mesh, vertexIndex);
			}

			MeshTangentCalculator::ApplyToTriangle(triangleVertices);
			for (const TexturedVertex& vertex : triangleVertices)
			{
				texturedMesh.vertices.push_back(vertex);
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
