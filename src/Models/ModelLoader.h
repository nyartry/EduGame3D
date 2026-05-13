#pragma once

#include "Rendering/TexturedVertex.h"

#include <string>
#include <vector>

struct TexturedMeshData
{
	std::vector<TexturedVertex> vertices;
	std::string baseColorTexturePath;
	std::string opacityTexturePath;
	std::string normalTexturePath;
};

struct ModelData
{
	std::vector<TexturedMeshData> texturedMeshes;
};

class ModelLoader
{
public:
	bool Load(const std::string& filePath, ModelData& modelData);
	std::string GetLastError() const;

private:
	std::string m_lastError;
};
