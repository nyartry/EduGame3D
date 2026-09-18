#include "Framework/Models/ModelTextureResolver.h"

#include "Framework/Assets/AssetPathResolver.h"

#include <algorithm>
#include <cctype>

namespace
{
	std::string ToLower(std::string text)
	{
		std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c)
		{
			return static_cast<char>(std::tolower(c));
		});
		return text;
	}
}

ModelTextureResolver::ModelTextureResolver(const std::filesystem::path& modelDirectory)
	: m_modelDirectory(modelDirectory)
{
}

std::string ModelTextureResolver::FindTexture(const aiMaterial* material, const std::vector<aiTextureType>& textureTypes) const
{
	if (material == nullptr)
	{
		return {};
	}

	for (const aiTextureType textureType : textureTypes)
	{
		if (material->GetTextureCount(textureType) == 0)
		{
			continue;
		}

		aiString texturePath;
		if (AI_SUCCESS == material->GetTexture(textureType, 0, &texturePath))
		{
			const std::string resolvedPath = ResolveTexturePath(texturePath.C_Str());
			if (!resolvedPath.empty())
			{
				return resolvedPath;
			}
		}
	}

	return {};
}

std::string ModelTextureResolver::ResolveTexturePath(const std::string& texturePath) const
{
	if (texturePath.empty() || texturePath[0] == '*')
	{
		return {};
	}

	const std::filesystem::path sourcePath = AssetPathResolver::FromUtf8(texturePath);
	const std::filesystem::path directPath = sourcePath.is_absolute()
		? sourcePath
		: m_modelDirectory / sourcePath;
	if (std::filesystem::exists(directPath))
	{
		return AssetPathResolver::ToUtf8(directPath);
	}

	const std::filesystem::path fileName = sourcePath.filename();
	const std::string lowerFileName = ToLower(AssetPathResolver::ToUtf8(fileName));
	const std::filesystem::path textureDirectories[] =
	{
		m_modelDirectory / "textures",
		m_modelDirectory / "tex"
	};

	for (const std::filesystem::path& textureDirectory : textureDirectories)
	{
		const std::filesystem::path siblingPath = textureDirectory / fileName;
		if (std::filesystem::exists(siblingPath))
		{
			return AssetPathResolver::ToUtf8(siblingPath);
		}

		if (!std::filesystem::exists(textureDirectory))
		{
			continue;
		}

		for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(textureDirectory))
		{
			if (entry.is_regular_file() && ToLower(AssetPathResolver::ToUtf8(entry.path().filename())) == lowerFileName)
			{
				return AssetPathResolver::ToUtf8(entry.path());
			}
		}
	}

	return {};
}
