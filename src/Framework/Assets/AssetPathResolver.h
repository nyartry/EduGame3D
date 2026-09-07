#pragma once

#include <filesystem>
#include <string>
#include <string_view>

// Shared policy for locating source-tree assets from the repository, build
// directory, or executable directory. This keeps asset consumers independent
// from the launcher's current working directory.
class AssetPathResolver final
{
public:
	static std::filesystem::path FromUtf8(std::string_view text);
	static std::string ToUtf8(const std::filesystem::path& path);
	static std::filesystem::path Resolve(std::string_view assetPath);
	static std::string ResolveUtf8(std::string_view assetPath);
};
