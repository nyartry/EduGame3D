#include "ModelLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

bool ModelLoader::Load(const std::string& filePath)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(
		filePath,
		aiProcess_Triangulate |
			aiProcess_JoinIdenticalVertices |
			aiProcess_ConvertToLeftHanded |
			aiProcess_GenNormals);

	if (scene == nullptr)
	{
		m_lastError = importer.GetErrorString();
		return false;
	}

	m_lastError.clear();
	return true;
}

std::string ModelLoader::GetLastError() const
{
	return m_lastError;
}
