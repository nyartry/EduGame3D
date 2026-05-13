#pragma once

#include <assimp/material.h>

#include <filesystem>
#include <string>
#include <vector>

class ModelTextureResolver
{
public:
	explicit ModelTextureResolver(const std::filesystem::path& modelDirectory);

	std::string FindTexture(const aiMaterial* material, const std::vector<aiTextureType>& textureTypes) const;

private:
	std::string ResolveTexturePath(const std::string& texturePath) const;

	std::filesystem::path m_modelDirectory;
};
