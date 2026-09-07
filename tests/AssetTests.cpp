#include "Framework/Assets/AssetPathResolver.h"
#include "Framework/Assets/ImageLoader.h"
#include "Framework/Common/Common.h"
#include "Framework/Rendering/Core/RenderResourceAccess.h"
#include "Framework/Rendering/Materials/TexturedMaterial.h"
#include "Framework/Rendering/Materials/Texture2D.h"
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>

using Microsoft::WRL::ComPtr;

void Require(bool value, const char* message)
{
	if (!value) throw std::runtime_error(message);
}

int main(int argc, char** argv)
{
	try
	{
		// Keep test artifacts beside this executable under x64, regardless of cwd.
		wchar_t executable[32768]{};
		const auto length = GetModuleFileNameW(nullptr, executable, 32768);
		Require(length && length < 32768, "Locate asset test executable");
		const auto directory = std::filesystem::path(executable).parent_path() / L"asset-fixtures";
		std::filesystem::create_directories(directory);
		const auto path = directory / L"\u65e5\u672c\u8a9e.bmp";
		const unsigned char bmp[] = {
			0x42,0x4d,58,0,0,0,0,0,0,0,54,0,0,0,
			40,0,0,0,1,0,0,0,1,0,0,0,1,0,24,0,
			0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			30,20,10,0};
		{ std::ofstream file(path, std::ios::binary); file.write(reinterpret_cast<const char*>(bmp), sizeof(bmp)); }
		const auto utf8 = AssetPathResolver::ToUtf8(path);
		Require(AssetPathResolver::FromUtf8(utf8) == path, "UTF-8 filesystem round trip");
		Require(AssetPathResolver::FromUtf8({}).empty(), "Empty UTF-8 path");
		const auto before = ImageLoader::GetDecodeCount();
		auto worker = std::async(std::launch::async, [&] { return ImageLoader::Load(utf8); });
		const auto prepared = worker.get();
		Require(!prepared->isFallback && prepared->width == 1 && prepared->height == 1, "Worker image decode");
		Require(prepared->pixels == std::vector<std::uint8_t>({10,20,30,255}), "WIC RGBA pixel conversion");
		Require(ImageLoader::Load(utf8) == prepared && ImageLoader::GetDecodeCount() == before + 1,
			"Prepare and activation must share decoded pixels");
		std::string diagnostic;
		Diagnostics::SetSink([&](std::string_view text) { diagnostic = text; });
		const auto missing = ImageLoader::Load(AssetPathResolver::ToUtf8(directory / L"missing.png"));
		Require(missing->isFallback && missing->pixels.size() == 16 && diagnostic.find("missing.png") != std::string::npos,
			"Missing image must retain path diagnostics and fallback");
		try { ThrowIfFailed(E_FAIL, "Asset test operation"); }
		catch (const std::runtime_error& error)
		{
			Require(std::string(error.what()).find("80004005") != std::string::npos &&
				diagnostic.find("Asset test operation") != std::string::npos, "HRESULT diagnostic detail");
		}
		Diagnostics::SetSink({});
		std::cout << "Asset CPU path, decode cache and diagnostic tests passed.\n";

		if (argc > 1 && std::string_view(argv[1]) == "--gpu")
		{
			ComPtr<IDXGIFactory4> factory;
			ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
			ComPtr<IDXGIAdapter> adapter;
			ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));
			ComPtr<ID3D12Device> device;
			ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)));
			const auto uploads = Texture2D::GetUploadCount();
			TexturedMaterial first, duplicate, differentRole;
			RenderResourceAccess::Initialize(first, device.Get(), utf8, utf8, "");
			Require(Texture2D::GetUploadCount() == uploads + 3, "Same path with sRGB/linear must create distinct textures");
			RenderResourceAccess::Initialize(duplicate, device.Get(), utf8, utf8, "");
			Require(Texture2D::GetUploadCount() == uploads + 3, "Identical materials must reuse GPU resources");
			RenderResourceAccess::Initialize(differentRole, device.Get(), utf8, "", "");
			Require(Texture2D::GetUploadCount() == uploads + 4, "Different materials must share common textures");
			Require(ImageLoader::GetDecodeCount() == before + 2, "GPU activation must not decode prepared pixels again");
			ComPtr<ID3D12Device> otherDevice;
			ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&otherDevice)));
			TexturedMaterial other;
			RenderResourceAccess::Initialize(other, otherDevice.Get(), utf8, utf8, "");
			// D3D12 may return the existing device for this adapter. Cache identity
			// follows the actual device, independent of how often it was requested.
			const auto expectedUploads = device.Get() == otherDevice.Get() ? 4 : 7;
			Require(Texture2D::GetUploadCount() == uploads + expectedUploads, "GPU cache must follow device identity");
			std::cout << "Asset WARP texture/material sharing tests passed.\n";
		}
		return 0;
	}
	catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
