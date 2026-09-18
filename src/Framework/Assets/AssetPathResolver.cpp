#include "Framework/Assets/AssetPathResolver.h"

#include <Windows.h>

#include <array>
#include <system_error>
#include <vector>

namespace
{
	std::filesystem::path GetExecutableDirectory()
	{
		std::vector<wchar_t> buffer(512);
		for (;;)
		{
			const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
			if (length == 0)
			{
				return {};
			}
			if (length < buffer.size() - 1)
			{
				return std::filesystem::path(buffer.data(), buffer.data() + length).parent_path();
			}
			buffer.resize(buffer.size() * 2);
		}
	}

	bool TryResolve(const std::filesystem::path& candidate, std::filesystem::path& resolved)
	{
		std::error_code error;
		if (!std::filesystem::exists(candidate, error))
		{
			return false;
		}
		const std::filesystem::path absolute = std::filesystem::absolute(candidate, error);
		resolved = error ? candidate.lexically_normal() : absolute.lexically_normal();
		return true;
	}

}

std::filesystem::path AssetPathResolver::Resolve(std::string_view assetPath)
{
	const std::filesystem::path requestedPath = FromUtf8(assetPath);
	std::filesystem::path resolvedPath;
	if (requestedPath.is_absolute())
	{
		return TryResolve(requestedPath, resolvedPath) ? resolvedPath : requestedPath;
	}

	std::error_code error;
	const std::array roots = { std::filesystem::current_path(error), GetExecutableDirectory() };
	for (const std::filesystem::path& root : roots)
	{
		std::filesystem::path searchRoot = root;
		for (int depth = 0; depth < 8 && !searchRoot.empty(); ++depth)
		{
			if (TryResolve(searchRoot / requestedPath, resolvedPath))
			{
				return resolvedPath;
			}
			const std::filesystem::path parent = searchRoot.parent_path();
			if (parent == searchRoot)
			{
				break;
			}
			searchRoot = parent;
		}
	}
	return requestedPath;
}

std::string AssetPathResolver::ResolveUtf8(std::string_view assetPath)
{
	return ToUtf8(Resolve(assetPath));
}

std::filesystem::path AssetPathResolver::FromUtf8(std::string_view text)
{
	if (text.empty()) return {};
	return std::filesystem::path(std::u8string(
		reinterpret_cast<const char8_t*>(text.data()), text.size()));
}

std::string AssetPathResolver::ToUtf8(const std::filesystem::path& path)
{
	const auto utf8 = path.u8string();
	return std::string(reinterpret_cast<const char*>(utf8.data()), utf8.size());
}
