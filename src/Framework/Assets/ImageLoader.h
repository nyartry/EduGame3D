#pragma once

#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

struct ImageData
{
	std::uint32_t width{};
	std::uint32_t height{};
	std::vector<std::uint8_t> pixels;
	bool isFallback{};
};

// CPU-only, thread-safe decoding. Retaining the returned data during Prepare
// lets GPU activation use the same pixels without reading the file again.
class ImageLoader final
{
public:
	static std::shared_ptr<const ImageData> Load(std::string_view path);
	static std::uint64_t GetDecodeCount();
};
