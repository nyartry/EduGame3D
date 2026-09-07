#include "Framework/Assets/AssetPathResolver.h"
#include "Framework/Assets/ImageLoader.h"
#include "Framework/Models/ModelAssetCache.h"
#include "Framework/Models/SkinnedModel.h"
#include "Framework/Models/StaticModel.h"
#include "Framework/Rendering/Core/IRenderDevice.h"

#include <atomic>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <future>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace
{
	void Require(bool condition, const char* message)
	{
		if (!condition) throw std::runtime_error(message);
	}

	class NoGpuDevice final : public IRenderDevice
	{
	public:
		std::thread::id activationThread{ std::this_thread::get_id() };
		int resources{};
		std::vector<TexturedVertex> staticVertices;
		void Resource()
		{
			Require(std::this_thread::get_id() == activationThread, "GPU resources must be created on the activation thread");
			++resources;
		}
		void CreateVertexBuffer(VertexBuffer&, const std::vector<Vertex>&) override { Resource(); }
		void CreateTexturedVertexBuffer(TexturedVertexBuffer&, const std::vector<TexturedVertex>& vertices) override
		{
			Resource(); staticVertices = vertices;
		}
		void CreateSkinnedVertexBuffer(SkinnedVertexBuffer&, const std::vector<SkinnedVertex>&) override { Resource(); }
		void CreateSpriteVertexBuffer(SpriteVertexBuffer&, std::uint32_t) override { Resource(); }
		void CreateSolidColorSpriteMaterial(SpriteMaterial&, std::uint8_t, std::uint8_t, std::uint8_t, std::uint8_t) override { Resource(); }
		void CreateTextureSpriteMaterial(SpriteMaterial&, const std::string&, bool) override { Resource(); }
		void CreatePixelSpriteMaterial(SpriteMaterial&, const std::vector<std::uint8_t>&, std::uint32_t, std::uint32_t, bool) override { Resource(); }
		void CreateTexturedMaterial(TexturedMaterial&, const std::string&, const std::string&, const std::string&) override { Resource(); }
	};

	struct TemporaryModel
	{
		std::filesystem::path directory;
		std::filesystem::path path;
		TemporaryModel()
		{
			static std::atomic<unsigned> sequence{};
			const auto unique = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "_" + std::to_string(sequence++);
			directory = AssetPathResolver::Resolve("Content/Models/Untitled/Untitled.fbx").parent_path().parent_path().parent_path().parent_path() /
				"work" / ("model_preparation_test_" + unique);
			std::filesystem::create_directories(directory);
			path = directory / "model.fbx";
			CopySource();
		}
		void CopySource() const
		{
			std::filesystem::copy_file(AssetPathResolver::Resolve("Content/Models/Untitled/Untitled.fbx"), path,
				std::filesystem::copy_options::overwrite_existing);
		}
		~TemporaryModel()
		{
			std::error_code error;
			std::filesystem::remove(path, error);
			std::filesystem::remove(directory, error);
		}
		std::string Utf8() const { return AssetPathResolver::ToUtf8(path); }
	};

	void WorkerPrepareThenActivationWithoutSource()
	{
		TemporaryModel file;
		SkinnedModel first;
		SkinnedModel second;
		StaticModel staticModel;
		NoGpuDevice device;
		const auto mainThread = std::this_thread::get_id();
		std::vector<std::shared_ptr<const ImageData>> images;
		auto preparation = std::async(std::launch::async, [&]
		{
			Require(std::this_thread::get_id() != mainThread, "Test must run CPU preparation on a worker");
			ModelAssetCache assets;
			const auto imported = assets.LoadSkinned(file.Utf8());
			Require(!imported->meshes.empty() && !imported->animations.empty(), "Real FBX includes geometry and animation");
			Require(imported == assets.LoadSkinned(file.Utf8()), "Repeated model requests reuse immutable import data");
			const auto originalName = imported->animations.front().name;
			const auto idle = assets.LoadAnimation(file.Utf8(), file.Utf8(), "Idle");
			Require(idle == assets.LoadAnimation(file.Utf8(), file.Utf8(), "Idle"), "Repeated animation request reuses compiled clips");
			Require(idle->front().name == "Idle" && imported->animations.front().name == originalName,
				"Animation alias does not mutate the cached model");
			first.Prepare(assets, file.Utf8(), ModelScaleSettings::NormalizeToHeight(2));
			second.Prepare(assets, file.Utf8(), ModelScaleSettings::NormalizeToHeight(2));
			first.PrepareAnimation(assets, "Idle", file.Utf8());
			second.PrepareAnimation(assets, "Idle", file.Utf8());
			staticModel.Prepare(assets, file.Utf8(), ModelScaleSettings::NormalizeToHeight(2));
			images = assets.TakePreparedImages();
		});
		preparation.get();
		Require(device.resources == 0, "Preparation cannot create any GPU resources");
		// Only a test-created copy is removed. Activate must consume prepared data,
		// so a hidden second FBX import will fail rather than silently passing.
		Require(std::filesystem::remove(file.path), "Test source copy is removed after successful Prepare");
		first.Activate(device, SkinningMode::Gpu);
		second.Activate(device, SkinningMode::Gpu);
		staticModel.Activate(device);
		Require(device.resources > 0 && !device.staticVertices.empty(), "Activation creates buffers from prepared vertices");
		float minimumY = device.staticVertices.front().position.y;
		float maximumY = minimumY;
		for (const auto& vertex : device.staticVertices)
		{
			minimumY = (std::min)(minimumY, vertex.position.y);
			maximumY = (std::max)(maximumY, vertex.position.y);
		}
		Require(std::abs(minimumY) < 0.0001f && std::abs(maximumY - 2) < 0.0001f, "Static CPU preparation preserves foot origin and target height");
		first.PlayAnimation("Idle");
		second.PlayAnimation("Idle");
		first.Update(0.1f);
		Require(first.GetAnimationTimeSeconds() > 0 && second.GetAnimationTimeSeconds() == 0,
			"Cached models keep independent playback state");
	}

	void FailedImportsDoNotPoisonCache()
	{
		TemporaryModel file;
		Require(std::filesystem::remove(file.path), "Remove only the test-created copy");
		ModelAssetCache assets;
		bool failed = false;
		try { assets.LoadSkinned(file.Utf8()); }
		catch (const std::runtime_error&) { failed = true; }
		Require(failed, "Missing model failure reaches the scene preparation future");
		file.CopySource();
		Require(!assets.LoadSkinned(file.Utf8())->meshes.empty(), "A failed import remains retryable after the asset appears");
	}

	void CacheLifetimeAndSynchronousCompatibility()
	{
		TemporaryModel file;
		std::weak_ptr<const SkinnedModelData> reference;
		{
			ModelAssetCache assets;
			reference = assets.LoadSkinned(file.Utf8());
			Require(!reference.expired(), "Preparation cache retains its import until preparation ends");
		}
		Require(reference.expired(), "Scene preparation leaves no process-wide strong model cache");
		NoGpuDevice device;
		SkinnedModel skinned;
		skinned.Initialize(device, file.Utf8(), ModelScaleSettings::NormalizeToHeight(1), SkinningMode::Gpu);
		skinned.AddAnimation("Idle", file.Utf8());
		Require(skinned.GetAnimationDurationSeconds("Idle") > 0, "Existing synchronous model and AddAnimation APIs remain usable");
		StaticModel staticModel;
		staticModel.Initialize(device, file.Utf8(), ModelScaleSettings::NormalizeToHeight(1));
		Require(device.resources > 0, "Existing static Initialize still prepares and activates");
	}
}

int main()
{
	int failures = 0;
	const auto run = [&failures](const char* name, void (*test)())
	{
		try { test(); std::cout << "PASS " << name << '\n'; }
		catch (const std::exception& error) { ++failures; std::cerr << "FAIL " << name << ": " << error.what() << '\n'; }
	};
	run("worker preparation and activation without source", WorkerPrepareThenActivationWithoutSource);
	run("failed model import remains retryable", FailedImportsDoNotPoisonCache);
	run("cache lifetime and synchronous compatibility", CacheLifetimeAndSynchronousCompatibility);
	return failures == 0 ? 0 : 1;
}
