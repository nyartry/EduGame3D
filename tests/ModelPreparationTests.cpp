#include "Framework/Assets/AssetPathResolver.h"
#include "Framework/Assets/ImageLoader.h"
#include "Framework/Models/ModelAssetCache.h"
#include "Framework/Models/SkinnedModel.h"
#include "Framework/Models/StaticModel.h"
#include "Framework/Rendering/Core/IRenderDevice.h"
#include "Framework/Scene/Input/InputWriter.h"
#include "Game/Gameplay/Player.h"

#include <atomic>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <future>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <utility>

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
			std::filesystem::remove(EventPath(), error);
			std::filesystem::remove(path, error);
			std::filesystem::remove(directory, error);
		}
		std::string Utf8() const { return AssetPathResolver::ToUtf8(path); }
		std::filesystem::path EventPath() const
		{
			auto eventPath = path;
			return eventPath.replace_extension(".anim_events.json");
		}
	};

	class TestPlayer final : public Player
	{
	public:
		explicit TestPlayer(std::string path, bool hasLocomotion = true) : m_path(std::move(path))
		{
			m_definition.mesh.modelPath = m_path;
			m_definition.mesh.skinningMode = SkinningMode::Gpu;
			m_definition.mesh.rootMotion.mode = RootMotionMode::Ignore;
			if (hasLocomotion)
			{
				m_definition.mesh.idleAnimationPath = m_path;
				m_definition.joggingAnimationPath = m_path;
			}
			m_definition.attackAnimationPath = m_path;
			m_definition.verticalMotion.gravityEnabled = false;
		}
		SkinnedModel& Model() { return GetModel(); }
		std::string_view CurrentClip() const
		{
			return GetModel().GetModelData().animations.at(GetModel().GetCurrentAnimationIndex()).name;
		}
	protected:
		const PlayerDefinition& GetPlayerDefinition() const override { return m_definition; }
	private:
		std::string m_path;
		PlayerDefinition m_definition;
	};

	struct ComboFixture
	{
		TemporaryModel file;
		TestPlayer player;
		NoGpuDevice device;
		Input input;
		float duration{};
		ComboFixture(double openFraction = 0.25, double closeFraction = 0.75, bool hasWindow = true, bool hasLocomotion = true)
			: player(file.Utf8(), hasLocomotion)
		{
			ModelAssetCache assets;
			const auto imported = assets.LoadSkinned(file.Utf8());
			Require(!imported->animations.empty(), "Combo fixture has an animation");
			const auto& clip = imported->animations.front();
			const double clipDuration = GetAnimationDurationSeconds(clip);
			Require(std::isfinite(clipDuration) && clipDuration > 0, "Combo fixture has a finite positive duration");
			AnimationEvents::FileData events;
			events.sourceFbx = file.Utf8();
			if (hasWindow)
			{
				events.events.push_back({ clipDuration * openFraction, clip.name, "ComboWindowOpen", "combo_open" });
				events.events.push_back({ clipDuration * closeFraction, clip.name, "ComboWindowClose", "combo_close" });
			}
			events.events.push_back({ clipDuration, clip.name, "PlaySE", "attack_end", "", "attack_end" });
			std::string error;
			Require(AnimationEvents::Save(file.EventPath(), events, error), "Write editable combo-window events");
			player.Prepare(assets);
			player.Initialize(device);
			duration = player.Model().GetAnimationDurationSeconds("Attack");
			Require(duration > 0 && (!hasLocomotion || player.CurrentClip() == "Idle"), "Combo player prepares its configured animations");
		}
		void Frame(float durationFraction, bool attackDown, bool moveDown = false)
		{
			InputWriter::BeginFrame(input);
			InputWriter::SetKey(input, InputKey::X, attackDown);
			InputWriter::SetKey(input, InputKey::W, moveDown);
			player.Update(duration * durationFraction, input);
		}
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

	void ExplicitModelPlaybackRequests()
	{
		TemporaryModel file;
		ModelAssetCache assets;
		SkinnedModel model;
		NoGpuDevice device;
		model.Prepare(assets, file.Utf8(), ModelScaleSettings::OriginalSize());
		model.PrepareAnimation(assets, "PlaybackAlias", file.Utf8());
		model.Activate(device, SkinningMode::Gpu);
		Require(model.PlayAnimationByIndex(0, { AnimationPlaybackMode::Loop, true }), "Existing clip accepts indexed loop playback");
		const std::string originalName = model.GetModelData().animations.front().name;
		const float duration = model.GetCurrentAnimationDurationSeconds();
		Require(std::isfinite(duration) && duration > 0, "Playback fixture has a finite positive duration");
		const float step = duration * 0.125f;
		model.Update(step);
		const float firstTime = model.GetAnimationTimeSeconds();
		Require(firstTime > 0, "Playback advances before requesting the same clip");
		Require(model.PlayAnimation(originalName) && model.PlayAnimationByIndex(0), "Default requests accept the current clip");
		Require(model.GetAnimationTimeSeconds() == firstTime && !model.IsAnimationFinished(),
			"Default same-clip requests preserve looping progress");
		model.Update(step);
		Require(model.GetAnimationTimeSeconds() > firstTime, "Default same-clip playback continues advancing");
		Require(model.PlayAnimation(originalName, { AnimationPlaybackMode::Loop, true }) && model.GetAnimationTimeSeconds() == 0,
			"Explicit restart rewinds the same looping clip");
		model.Update(step);
		Require(model.PlayAnimationByIndex(0, { AnimationPlaybackMode::Once }) && model.GetAnimationTimeSeconds() == 0,
			"Changing playback mode restarts the current clip");
		model.Update(duration * 2);
		Require(model.IsAnimationFinished() && model.GetAnimationTimeSeconds() == duration,
			"One-shot playback finishes at the actual clip endpoint without wrapping");
		model.Update(step);
		Require(model.IsAnimationFinished() && model.GetAnimationTimeSeconds() == duration,
			"Finished one-shot playback remains at its endpoint");
		Require(!model.PlayAnimation("MissingPlaybackClip", { AnimationPlaybackMode::Loop, true }) &&
			!model.PlayAnimationByIndex(model.GetModelData().animations.size(), { AnimationPlaybackMode::Loop, true }),
			"Missing clip names and invalid indices reject playback requests");
		Require(model.GetCurrentAnimationIndex() == 0 && model.GetAnimationTimeSeconds() == duration && model.IsAnimationFinished(),
			"Rejected requests preserve the current clip, endpoint and completion state");
		Require(model.PlayAnimation(originalName, { AnimationPlaybackMode::Once, true }) &&
			model.GetAnimationTimeSeconds() == 0 && !model.IsAnimationFinished(),
			"Restarting a completed one-shot clears completion and returns to the first frame");
		model.Update(duration * 2);
		Require(model.PlayAnimation("PlaybackAlias", { AnimationPlaybackMode::Once }) &&
			model.GetCurrentAnimationIndex() != 0 && model.GetAnimationTimeSeconds() == 0 && !model.IsAnimationFinished(),
			"Switching clips clears completion and starts the new clip");
		model.Update(duration * 2);
		Require(model.PlayAnimation("PlaybackAlias") && model.GetAnimationTimeSeconds() == 0 && !model.IsAnimationFinished(),
			"Returning to default loop mode clears one-shot completion");
	}

	void OneShotModelEventsFireOnce()
	{
		TemporaryModel file;
		ModelAssetCache assets;
		const auto imported = assets.LoadSkinned(file.Utf8());
		Require(!imported->animations.empty(), "Event fixture has an animation");
		const auto& clip = imported->animations.front();
		const double duration = GetAnimationDurationSeconds(clip);
		Require(std::isfinite(duration) && duration > 0, "Event fixture has a finite positive duration");
		AnimationEvents::FileData events;
		events.sourceFbx = file.Utf8();
		events.events = {
			{ 0, clip.name, "Custom", "start" },
			{ duration * 0.5, clip.name, "Custom", "middle" },
			{ duration, clip.name, "Custom", "end" }
		};
		std::string error;
		Require(AnimationEvents::Save(file.EventPath(), events, error), "Write the disposable model event sidecar");
		SkinnedModel model;
		NoGpuDevice device;
		model.Prepare(assets, file.Utf8(), ModelScaleSettings::OriginalSize());
		model.Activate(device, SkinningMode::Gpu);
		Require(model.PlayAnimationByIndex(0, { AnimationPlaybackMode::Once, true }), "One-shot event playback starts");
		model.Update(static_cast<float>(duration * 0.75));
		const float beforeRejectedRequest = model.GetAnimationTimeSeconds();
		Require(!model.PlayAnimation("MissingEventClip", { AnimationPlaybackMode::Loop, true }), "Reject a missing clip during playback");
		Require(model.GetAnimationTimeSeconds() == beforeRejectedRequest && !model.IsAnimationFinished(),
			"A rejected request preserves in-progress playback");
		Require(model.GetAnimationEvents().size() == 1 && model.GetAnimationEvents().front().event.name == "middle",
			"Gameplay can inspect animation events without consuming them");
		auto fired = model.ConsumeAnimationEvents();
		Require(fired.size() == 1 && fired.front().event.name == "middle", "A rejected request preserves pending animation events");
		Require(model.GetAnimationEvents().empty(), "The non-consuming event view reflects external consumption");
		Require(model.ConsumeAnimationEvents().empty(), "Animation events are consumed once");
		model.Update(static_cast<float>(duration));
		fired = model.ConsumeAnimationEvents();
		Require(model.IsAnimationFinished() && fired.size() == 1 && fired.front().event.name == "end",
			"Crossing the one-shot endpoint emits the end event without a loop-start event");
		model.Update(static_cast<float>(duration));
		Require(model.ConsumeAnimationEvents().empty(), "An already finished clip does not emit its end event again");
		Require(model.PlayAnimationByIndex(0, { AnimationPlaybackMode::Once, true }), "Completed event playback can restart");
		model.Update(static_cast<float>(duration * 2));
		fired = model.ConsumeAnimationEvents();
		Require(fired.size() == 2 && fired[0].event.name == "middle" && fired[1].event.name == "end",
			"A restarted one-shot emits each crossed event once even when a frame exceeds its duration");
	}

	void PlayerRejectsOutOfWindowAndHeldInput()
	{
		for (const float pressTime : { 0.125f, 0.875f })
		{
			ComboFixture fixture;
			fixture.Frame(pressTime, true);
			fixture.Frame(0, false);
			fixture.Frame(0, true);
			fixture.Frame(2, false);
			Require(fixture.player.CurrentClip() == "Attack" && fixture.player.Model().IsAnimationFinished(),
				"The completed attack remains selected through its final simulation frame");
			fixture.Frame(0, false);
			Require(fixture.player.CurrentClip() == "Idle", "Attack presses before opening or after closing do not queue a combo");
		}
		ComboFixture held;
		held.Frame(0.5f, true);
		held.Frame(0.1f, true);
		held.Frame(2, true);
		held.Frame(0, true, true);
		Require(held.player.CurrentClip() == "Jogging", "Holding attack does not queue a combo and movement resumes on the next update");
	}

	void PlayerQueuesOneComboAndPreservesFinalEvents()
	{
		ComboFixture fixture;
		fixture.Frame(0.4f, true);
		const auto opened = fixture.player.ConsumeAnimationEvents();
		Require(opened.size() == 1 && opened.front().event.type == "ComboWindowOpen",
			"Player window handling preserves events for the scene consumer");
		const float attackTime = fixture.player.Model().GetAnimationTimeSeconds();
		fixture.Frame(0, false);
		fixture.Frame(0, true);
		fixture.Frame(0, false);
		fixture.Frame(0, true);
		Require(fixture.player.Model().GetAnimationTimeSeconds() == attackTime,
			"Repeated valid combo inputs do not restart the current attack");
		fixture.Frame(2, false);
		Require(fixture.player.CurrentClip() == "Attack" && fixture.player.Model().IsAnimationFinished(),
			"A queued combo waits until the update after the attack finishes");
		const auto finished = fixture.player.ConsumeAnimationEvents();
		Require(finished.size() == 2 && finished[0].event.type == "ComboWindowClose" && finished[1].event.name == "attack_end",
			"The scene can still consume window-close and end events from the finishing attack");
		fixture.Frame(0, false);
		Require(fixture.player.CurrentClip() == "Attack" && !fixture.player.Model().IsAnimationFinished() &&
			fixture.player.Model().GetAnimationTimeSeconds() == 0,
			"A queued attack begins at its first frame on the following update");
		fixture.Frame(2, false);
		fixture.Frame(0, false);
		Require(fixture.player.CurrentClip() == "Idle", "Multiple presses in one window reserve only one following attack");
	}

	void PlayerUsesEditedComboWindowEvents()
	{
		for (const bool hasWindow : { true, false })
		{
			ComboFixture fixture(0.6, 0.9, hasWindow);
			fixture.Frame(0.4f, true);
			fixture.Frame(0, false);
			fixture.Frame(0, true);
			fixture.Frame(2, false);
			fixture.Frame(0, false);
			Require(fixture.player.CurrentClip() == "Idle",
				"Moving the open event later or omitting window events prevents previously accepted combo timing");
		}
	}

	void PlayerWithoutLocomotionClipsCanLeaveAttack()
	{
		ComboFixture fixture(0.25, 0.75, true, false);
		fixture.Frame(2, true);
		Require(fixture.player.Model().IsAnimationFinished(), "An attack finishes without locomotion animation assets");
		const float beforeMovement = fixture.player.GetPosition().z;
		fixture.Frame(0.1f, false, true);
		Require(fixture.player.GetPosition().z > beforeMovement,
			"Missing optional Idle and Jogging clips cannot trap the player in the finished attack state");
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
	run("explicit model playback and rejected requests", ExplicitModelPlaybackRequests);
	run("one-shot model events fire once", OneShotModelEventsFireOnce);
	run("player rejects early, late and held combo input", PlayerRejectsOutOfWindowAndHeldInput);
	run("player queues one combo and preserves final-frame events", PlayerQueuesOneComboAndPreservesFinalEvents);
	run("player uses edited combo-window events", PlayerUsesEditedComboWindowEvents);
	run("player without locomotion clips exits attack", PlayerWithoutLocomotionClipsCanLeaveAttack);
	return failures == 0 ? 0 : 1;
}
