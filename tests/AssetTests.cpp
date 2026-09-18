#include "Framework/Assets/AssetPathResolver.h"
#include "Framework/Assets/ImageLoader.h"
#include "Framework/Common/Common.h"
#include "Framework/Rendering/Core/RenderResourceAccess.h"
#include "Framework/Rendering/Materials/TexturedMaterial.h"
#include "Framework/Rendering/Materials/Texture2D.h"
#include "TestSupport.h"
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <thread>

namespace
{
using Microsoft::WRL::ComPtr;

void Require(bool value, const char* message)
{
	if (!value) throw std::runtime_error(message);
}

struct DiagnosticSinkScope
{
	explicit DiagnosticSinkScope(Diagnostics::Sink sink = {}) { Diagnostics::SetSink(std::move(sink)); }
	~DiagnosticSinkScope() { Diagnostics::SetSink({}); }
};

void WriteBmp(const std::filesystem::path& path)
{
	const unsigned char bmp[] = {
		0x42,0x4d,58,0,0,0,0,0,0,0,54,0,0,0,
		40,0,0,0,1,0,0,0,1,0,0,0,1,0,24,0,
		0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		30,20,10,0};
	std::ofstream file(path, std::ios::binary);
	file.write(reinterpret_cast<const char*>(bmp), sizeof(bmp));
	Require(static_cast<bool>(file), "Write asset BMP fixture");
}

struct AssetFixture
{
	DiagnosticSinkScope diagnosticSink;
	std::filesystem::path directory;
	std::filesystem::path path;
	std::string utf8;

	AssetFixture()
	{
		// Use the test module, since the native test host executable lives elsewhere.
		// Every invocation owns its files, so selected tests and reruns are independent.
		static std::atomic<unsigned> sequence{};
		directory = TestSupport::ModuleDirectory() / L"asset-fixtures" /
			(std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64()) +
				L"-" + std::to_wstring(sequence.fetch_add(1)));
		std::filesystem::create_directories(directory);
		path = directory / L"\u65e5\u672c\u8a9e.bmp";
		WriteBmp(path);
		utf8 = AssetPathResolver::ToUtf8(path);
	}

	~AssetFixture()
	{
		// Remove only files this fixture can create; never recursively remove a path.
		std::error_code ignored;
		std::filesystem::remove(path, ignored);
		std::filesystem::remove(directory / L"repairable.bmp", ignored);
		std::filesystem::remove(directory, ignored);
	}
};

void PathsDecodeAndCache()
{
	AssetFixture fixture;
	Require(AssetPathResolver::FromUtf8(fixture.utf8) == fixture.path, "UTF-8 filesystem round trip");
	Require(AssetPathResolver::FromUtf8({}).empty(), "Empty UTF-8 path");
	const auto before = ImageLoader::GetDecodeCount();
	auto worker = std::async(std::launch::async, [&] { return ImageLoader::Load(fixture.utf8); });
	const auto prepared = worker.get();
	Require(!prepared->isFallback && prepared->width == 1 && prepared->height == 1, "Worker image decode");
	Require(prepared->pixels == std::vector<std::uint8_t>({10,20,30,255}), "WIC RGBA pixel conversion");
	Require(ImageLoader::Load(fixture.utf8) == prepared && ImageLoader::GetDecodeCount() == before + 1,
		"Prepare and activation must share decoded pixels");
}

void ImageFallbackReportsOnceWithContext()
{
	AssetFixture fixture;
	std::vector<std::string> diagnostics;
	DiagnosticSinkScope sink([&](std::string_view text) { diagnostics.emplace_back(text); });
	const auto missing = ImageLoader::Load(AssetPathResolver::ToUtf8(fixture.directory / L"missing.png"));
	Require(missing->isFallback && missing->pixels.size() == 16, "Missing image must provide fallback pixels");
	Require(diagnostics.size() == 1 && diagnostics.front().find("missing.png") != std::string::npos &&
		diagnostics.front().find("Open image file") != std::string::npos &&
		diagnostics.front().find("HRESULT 0x") != std::string::npos,
		"Image fallback must report its path and original failure exactly once");
}

void HResultFailuresPropagateWithoutDuplicateReporting()
{
	std::vector<std::string> diagnostics;
	DiagnosticSinkScope sink([&](std::string_view text) { diagnostics.emplace_back(text); });
	ThrowIfFailed(S_OK, "Successful asset operation");
	std::string message;
	try { ThrowIfFailed(E_FAIL, "Asset test operation"); }
	catch (const std::runtime_error& error) { message = error.what(); }
	Require(message.find("80004005") != std::string::npos && message.find("Asset test operation") != std::string::npos &&
		message.find("AssetTests.cpp:") != std::string::npos, "HRESULT exception must retain code, operation and call site");
	Require(diagnostics.empty(), "HRESULT helpers must leave reporting to the recovery boundary");
}

void RepairedImageRetriesWhileFallbackRemainsInUse()
{
	AssetFixture fixture;
	const auto prepared = ImageLoader::Load(fixture.utf8);
	const auto repairPath = fixture.directory / L"repairable.bmp";
	{ std::ofstream file(repairPath, std::ios::binary); file << "broken image"; }
	DiagnosticSinkScope sink([](std::string_view) {});
	const auto repairUtf8 = AssetPathResolver::ToUtf8(repairPath);
	const auto fallback = ImageLoader::Load(repairUtf8);
	Require(fallback->isFallback, "Corrupt fixture produces fallback pixels");
	WriteBmp(repairPath);
	const auto repaired = ImageLoader::Load(repairUtf8);
	Require(!repaired->isFallback && repaired->pixels == prepared->pixels && fallback->isFallback,
		"Repairing an image must succeed without releasing consumers of its fallback");
	Require(ImageLoader::Load(repairUtf8) == repaired, "A successful retry must enter the shared decode cache");
}

void ImageDiagnosticsAllowAnotherImageLoad()
{
	AssetFixture fixture;
	const auto prepared = ImageLoader::Load(fixture.utf8);
	std::promise<void> loaded;
	auto completion = loaded.get_future();
	std::thread reader;
	bool completedDuringCallback = false;
	DiagnosticSinkScope sink([&](std::string_view)
	{
		if (reader.joinable()) return;
		reader = std::thread([&]
		{
			try { Require(ImageLoader::Load(fixture.utf8) == prepared, "Reentrant lookup retains cache sharing"); loaded.set_value(); }
			catch (...) { loaded.set_exception(std::current_exception()); }
		});
		// Return on timeout so the old mutex-held implementation can unwind
		// and the reader can be joined instead of hanging the test process.
		completedDuringCallback = completion.wait_for(std::chrono::seconds(2)) == std::future_status::ready;
	});
	const auto missing = ImageLoader::Load(AssetPathResolver::ToUtf8(fixture.directory / L"reentrant-missing.png"));
	Require(reader.joinable(), "A missing image invokes the diagnostic sink");
	reader.join();
	completion.get();
	Require(missing->isFallback && completedDuringCallback, "Diagnostic callbacks must run after the image cache lock is released");
}

void GpuTextureAndMaterialSharing()
{
	AssetFixture fixture;
	const auto prepared = ImageLoader::Load(fixture.utf8);
	Require(!prepared->isFallback, "Prepare the GPU test image independently");
	const auto beforeGpuDecodeCount = ImageLoader::GetDecodeCount();
	ComPtr<IDXGIFactory4> factory;
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
	ComPtr<IDXGIAdapter> adapter;
	ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));
	ComPtr<ID3D12Device> device;
	ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)));
	const auto uploads = Texture2D::GetUploadCount();
	TexturedMaterial first, duplicate, differentRole;
	RenderResourceAccess::Initialize(first, device.Get(), fixture.utf8, fixture.utf8, "");
	Require(Texture2D::GetUploadCount() == uploads + 3, "Same path with sRGB/linear must create distinct textures");
	RenderResourceAccess::Initialize(duplicate, device.Get(), fixture.utf8, fixture.utf8, "");
	Require(Texture2D::GetUploadCount() == uploads + 3, "Identical materials must reuse GPU resources");
	RenderResourceAccess::Initialize(differentRole, device.Get(), fixture.utf8, "", "");
	Require(Texture2D::GetUploadCount() == uploads + 4, "Different materials must share common textures");
	Require(ImageLoader::GetDecodeCount() == beforeGpuDecodeCount, "GPU activation must not decode prepared pixels again");
	ComPtr<ID3D12Device> otherDevice;
	ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&otherDevice)));
	TexturedMaterial other;
	RenderResourceAccess::Initialize(other, otherDevice.Get(), fixture.utf8, fixture.utf8, "");
	// D3D12 may return the existing device for this adapter. Cache identity
	// follows the actual device, independent of how often it was requested.
	const auto expectedUploads = device.Get() == otherDevice.Get() ? 4 : 7;
	Require(Texture2D::GetUploadCount() == uploads + expectedUploads, "GPU cache must follow device identity");
}
}

#define ASSET_TEST_CASES(TEST) \
	TEST(PathsDecodeAndCache, "asset paths, worker decode and shared cache", Cpu) \
	TEST(ImageFallbackReportsOnceWithContext, "image fallback reports once with context", Cpu) \
	TEST(HResultFailuresPropagateWithoutDuplicateReporting, "HRESULT failures propagate without duplicate reporting", Cpu) \
	TEST(RepairedImageRetriesWhileFallbackRemainsInUse, "repaired image retries while fallback remains in use", Cpu) \
	TEST(ImageDiagnosticsAllowAnotherImageLoad, "image diagnostics allow another image load", Cpu) \
	TEST(GpuTextureAndMaterialSharing, "WARP texture and material sharing", Gpu)

GAME_TEST_SUITE(AssetTests, ASSET_TEST_CASES)
