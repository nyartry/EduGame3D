#include "ModelLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace
{
	constexpr float DefaultColor[] = { 0.62f, 0.78f, 0.95f, 1.0f };

	void CopyColor(float destination[4], const float source[4])
	{
		destination[0] = source[0];
		destination[1] = source[1];
		destination[2] = source[2];
		destination[3] = source[3];
	}

	void GetMaterialColor(const aiScene* scene, unsigned int materialIndex, float color[4])
	{
		CopyColor(color, DefaultColor);

		if (scene == nullptr || materialIndex >= scene->mNumMaterials)
		{
			return;
		}

		aiColor4D diffuseColor;
		if (AI_SUCCESS == scene->mMaterials[materialIndex]->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor))
		{
			color[0] = diffuseColor.r;
			color[1] = diffuseColor.g;
			color[2] = diffuseColor.b;
			color[3] = diffuseColor.a;

			const float brightness = color[0] + color[1] + color[2];
			if (brightness < 0.05f)
			{
				CopyColor(color, DefaultColor);
			}
		}
	}

	Vertex MakeVertex(const aiVector3D& position, const float color[4])
	{
		return Vertex
		{
			{ position.x, position.y, position.z },
			{ color[0], color[1], color[2], color[3] }
		};
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
			aiProcess_GenNormals);

	if (scene == nullptr)
	{
		m_lastError = importer.GetErrorString();
		return false;
	}

	modelData.vertices.clear();

	for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
	{
		const aiMesh* mesh = scene->mMeshes[meshIndex];
		float color[4]{};
		GetMaterialColor(scene, mesh->mMaterialIndex, color);

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
				modelData.vertices.push_back(MakeVertex(mesh->mVertices[vertexIndex], color));
			}
		}
	}

	if (modelData.vertices.empty())
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
