#include "Framework/Assets/ImageLoader.h"
#include "Framework/Assets/AssetPathResolver.h"
#include "Framework/Common/Common.h"

#include <wincodec.h>
#include <wrl/client.h>
#include <atomic>
#include <limits>
#include <map>
#include <mutex>

using Microsoft::WRL::ComPtr;

namespace
{
	std::mutex cacheMutex;
	std::map<std::filesystem::path, std::weak_ptr<const ImageData>> cache;
	std::atomic<std::uint64_t> decodeCount{};

	struct ComScope
	{
		HRESULT result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		ComScope() { if (result != RPC_E_CHANGED_MODE) ThrowIfFailed(result, "Initialize image decoder COM"); }
		~ComScope() { if (SUCCEEDED(result)) CoUninitialize(); }
	};

	void Decode(const std::filesystem::path& path, ImageData& image)
	{
		ComScope com;
		ComPtr<IWICImagingFactory2> factory;
		ThrowIfFailed(CoCreateInstance(CLSID_WICImagingFactory2, nullptr, CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&factory)), "Create WIC image factory");
		ComPtr<IWICBitmapDecoder> decoder;
		ThrowIfFailed(factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ,
			WICDecodeMetadataCacheOnLoad, &decoder), "Open image file");
		ComPtr<IWICBitmapFrameDecode> frame;
		ThrowIfFailed(decoder->GetFrame(0, &frame), "Read image frame");
		UINT width{}, height{};
		ThrowIfFailed(frame->GetSize(&width, &height), "Read image dimensions");
		const auto maxBytes = (std::numeric_limits<UINT>::max)();
		if (!width || !height || width > maxBytes / 4 || height > maxBytes / (width * 4))
			throw std::runtime_error("Image dimensions exceed WIC pixel buffer limits");
		ComPtr<IWICFormatConverter> converter;
		ThrowIfFailed(factory->CreateFormatConverter(&converter), "Create RGBA converter");
		ThrowIfFailed(converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppRGBA,
			WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom), "Convert image to RGBA");
		image.width = width;
		image.height = height;
		image.pixels.resize(static_cast<std::size_t>(width) * height * 4);
		ThrowIfFailed(converter->CopyPixels(nullptr, width * 4, static_cast<UINT>(image.pixels.size()),
			image.pixels.data()), "Decode RGBA pixels");
	}
}

std::shared_ptr<const ImageData> ImageLoader::Load(std::string_view path)
{
	const auto resolved = AssetPathResolver::Resolve(path);
	std::unique_lock lock(cacheMutex);
	if (const auto found = cache.find(resolved); found != cache.end())
		if (auto existing = found->second.lock()) return existing;
	std::erase_if(cache, [](const auto& entry) { return entry.second.expired(); });
	auto image = std::make_shared<ImageData>();
	++decodeCount;
	try { Decode(resolved, *image); }
	catch (const std::runtime_error& error)
	{
		image->width = image->height = 2;
		image->pixels = {255,255,255,255, 180,180,180,255, 180,180,180,255, 255,255,255,255};
		image->isFallback = true;
		// Failed decodes remain retryable while existing consumers use their
		// fallback. Application diagnostic sinks may themselves load images.
		lock.unlock();
		Diagnostics::Write("Image fallback [" + AssetPathResolver::ToUtf8(resolved) + "]: " + error.what());
		return image;
	}
	cache[resolved] = image;
	return image;
}

std::uint64_t ImageLoader::GetDecodeCount() { return decodeCount.load(); }
