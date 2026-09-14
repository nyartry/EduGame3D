#include "TestSupport.h"
#include "Framework/Animation/BoneSkinning.h"
#include "Framework/Assets/AssetPathResolver.h"
#include "Framework/Assets/ImageLoader.h"
#include "Framework/Core/Math/Aabb.h"
#include "Framework/Models/ModelAssetCache.h"
#include "Framework/Models/SkinnedModel.h"
#include "Framework/Models/StaticModel.h"
#include "Framework/Rendering/Core/IRenderDevice.h"
#include "Framework/Scene/Input/InputWriter.h"
#include "Game/Gameplay/Player.h"

#include <array>
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

	constexpr std::array<const char*, 3> EduHumanModels{
		"Content/Models/EduHuman/EduHuman_Idle.fbx",
		"Content/Models/EduHuman/EduHuman_Jog.fbx",
		"Content/Models/EduHuman/EduHuman_Kick.fbx"
	};

	std::size_t RequireEduHumanBone(const SkinnedModelData& data, std::string_view name)
	{
		for (std::size_t index = 0; index < data.bones.size(); ++index)
			if (data.bones[index].name == name) return index;
		throw std::runtime_error("EduHuman is missing bone: " + std::string(name));
	}

	void RequireSameBindMatrix(const DirectX::XMFLOAT4X4& first, const DirectX::XMFLOAT4X4& second)
	{
		for (int row = 0; row < 4; ++row)
			for (int column = 0; column < 4; ++column)
				Require(std::isfinite(first.m[row][column]) && std::isfinite(second.m[row][column]) &&
					std::abs(first.m[row][column] - second.m[row][column]) < 0.001f,
					"EduHuman animation exports must retain the same finite bind pose and skin offsets");
	}

	void EduHumanSkeletonAndSkinWeights()
	{
		ModelAssetCache assets;
		const auto reference = assets.LoadSkinned(EduHumanModels[0]);
		for (const char* path : EduHumanModels)
		{
			const auto data = assets.LoadSkinned(path);
			Require(!data->meshes.empty() && data->animations.size() == 1,
				"Each EduHuman FBX must contain drawable geometry and exactly one animation");
			// Use the actual CPU/GPU skinning limit, not an assumed exporter bone count.
			Require(!data->bones.empty() && data->bones.size() <= BoneSkinning::MaxBones,
				"EduHuman must fit the engine's bone palette without truncation");
			for (std::size_t index = 0; index < data->bones.size(); ++index)
			{
				const int parent = data->bones[index].parentIndex;
				Require(parent >= -1 && parent < static_cast<int>(index),
					"Imported EduHuman parents must precede children, without cycles or invalid indices");
			}
			const auto requireChain = [&](std::initializer_list<const char*> names)
			{
				int previous = -1;
				for (const char* name : names)
				{
					const auto index = RequireEduHumanBone(*data, name);
					if (previous >= 0)
						Require(data->bones[index].parentIndex == previous,
							"EduHuman must retain connected torso, head, arm and leg chains");
					const auto referenceIndex = RequireEduHumanBone(*reference, name);
					RequireSameBindMatrix(data->bones[index].localBindTransform, reference->bones[referenceIndex].localBindTransform);
					previous = static_cast<int>(index);
				}
			};
			requireChain({ "Root", "Hips", "Spine", "Chest", "Neck", "Head" });
			requireChain({ "Chest", "Shoulder.L", "UpperArm.L", "Forearm.L", "Hand.L" });
			requireChain({ "Chest", "Shoulder.R", "UpperArm.R", "Forearm.R", "Hand.R" });
			requireChain({ "Hips", "Thigh.L", "Shin.L", "Foot.L", "Toe.L" });
			requireChain({ "Hips", "Thigh.R", "Shin.R", "Foot.R", "Toe.R" });

			Aabb geometryBounds;
			std::vector<bool> weightedBones(data->bones.size());
			for (const auto& mesh : data->meshes)
			{
				Require(!mesh.vertices.empty() && mesh.vertices.size() % 3 == 0,
					"EduHuman meshes must contain complete triangles");
				Require(mesh.baseColorTexturePath.empty() && mesh.opacityTexturePath.empty() && mesh.normalTexturePath.empty(),
					"EduHuman must not depend on omitted third-party image assets");
				for (const auto& vertex : mesh.vertices)
				{
					const auto& position = vertex.vertex.position;
					const auto& normal = vertex.vertex.normal;
					Require(std::isfinite(position.x) && std::isfinite(position.y) && std::isfinite(position.z) &&
						std::isfinite(normal.x) && std::isfinite(normal.y) && std::isfinite(normal.z),
						"EduHuman geometry must remain finite after Assimp import");
					geometryBounds.AddPoint(position);
					float totalWeight = 0;
					for (std::size_t slot = 0; slot < std::size(vertex.boneWeights); ++slot)
					{
						const float weight = vertex.boneWeights[slot];
						Require(std::isfinite(weight) && weight >= 0, "EduHuman skin weights must be finite and nonnegative");
						if (weight == 0) continue;
						const int boneIndex = vertex.boneIndices[slot];
						Require(boneIndex >= 0 && static_cast<std::size_t>(boneIndex) < data->bones.size(),
							"Every nonzero EduHuman skin weight must reference an imported bone");
						weightedBones[static_cast<std::size_t>(boneIndex)] = true;
						totalWeight += weight;
					}
					Require(std::abs(totalWeight - 1.0f) < 0.0001f,
						"Every EduHuman vertex must be weighted and normalized; rigid fallback is not sufficient");
				}
			}
			const auto geometrySize = geometryBounds.Size();
			Require(geometrySize.y > 1.7f && geometrySize.y < 1.9f && std::abs(geometryBounds.Min().y) < 0.02f &&
				geometrySize.x < 1.0f && geometrySize.z < 0.65f,
				"EduHuman mesh export must use meters, Y-up and feet at the origin, matching its skeleton");
			SkinnedModel pose;
			pose.Prepare(assets, path, ModelScaleSettings::OriginalSize());
			pose.SetAnimationTimeSeconds(0);
			for (const auto& [footName, toeName] : { std::pair{ "Foot.L", "Toe.L" }, std::pair{ "Foot.R", "Toe.R" } })
			{
				DirectX::XMFLOAT3 foot{}, toe{};
				Require(pose.TryGetBonePositionLocal(footName, foot) && pose.TryGetBonePositionLocal(toeName, toe) && toe.z < foot.z,
					"Both EduHuman feet must face model-local negative Z after FBX coordinate conversion");
			}
			for (const char* name : { "Head", "Chest", "UpperArm.L", "Forearm.L", "Hand.L", "UpperArm.R", "Forearm.R",
				"Hand.R", "Thigh.L", "Shin.L", "Foot.L", "Thigh.R", "Shin.R", "Foot.R" })
			{
				const auto index = RequireEduHumanBone(*data, name);
				Require(weightedBones[index], "EduHuman needs weighted geometry on its head, torso and all four limbs");
				const auto referenceIndex = RequireEduHumanBone(*reference, name);
				RequireSameBindMatrix(data->bones[index].offsetMatrix, reference->bones[referenceIndex].offsetMatrix);
			}
		}
	}

	void EduHumanClipsAnimateOneSkeleton()
	{
		ModelAssetCache assets;
		SkinnedModel model;
		NoGpuDevice device;
		model.Prepare(assets, EduHumanModels[0], ModelScaleSettings::NormalizeToHeight(1.8f));
		constexpr std::array<const char*, 3> names{ "Idle", "Jogging", "Attack" };
		for (std::size_t index = 0; index < names.size(); ++index)
			model.PrepareAnimation(assets, names[index], EduHumanModels[index]);
		Require(device.resources == 0, "EduHuman and all three clips must prepare without GPU resources");
		model.Activate(device, SkinningMode::Gpu);
		const auto requireMotion = [&](const char* clip, std::initializer_list<const char*> boneNames)
		{
			Require(model.PlayAnimation(clip, { AnimationPlaybackMode::Loop, true }),
				"Idle, jogging and attack exports must load and play on the same EduHuman skeleton");
			const float duration = model.GetCurrentAnimationDurationSeconds();
			Require(std::isfinite(duration) && duration > 0, "Each EduHuman clip must have a finite positive duration");
			for (const char* bone : boneNames)
			{
				model.SetAnimationTimeSeconds(0);
				DirectX::XMFLOAT3 first{};
				Require(model.TryGetBonePositionLocal(bone, first), "EduHuman animated joints must be queryable");
				float maximumDisplacementSquared = 0;
				for (const float fraction : { 0.125f, 0.25f, 0.375f, 0.5f, 0.625f, 0.75f, 0.875f })
				{
					model.SetAnimationTimeSeconds(duration * fraction);
					DirectX::XMFLOAT3 current{};
					Require(model.TryGetBonePositionLocal(bone, current) && std::isfinite(current.x) &&
						std::isfinite(current.y) && std::isfinite(current.z), "EduHuman playback must produce finite joint positions");
					const float dx = current.x - first.x, dy = current.y - first.y, dz = current.z - first.z;
					maximumDisplacementSquared = (std::max)(maximumDisplacementSquared, dx * dx + dy * dy + dz * dz);
				}
				Require(maximumDisplacementSquared > 0.000001f,
					"EduHuman clips must move the expected body parts, not merely contain static animation tracks");
			}
		};
		requireMotion("Idle", { "Head" });
		requireMotion("Jogging", { "Hand.L", "Hand.R", "Foot.L", "Foot.R" });
		requireMotion("Attack", { "Foot.R" });
	}

	void EduHumanKickLoadsAuthoredComboEvents()
	{
		ModelAssetCache assets;
		const auto kick = assets.LoadSkinned(EduHumanModels[2]);
		Require(kick->animations.size() == 1 && std::abs(GetAnimationDurationSeconds(kick->animations.front()) - 1.6) < 0.001,
			"The authored EduHuman kick must retain its 1.6-second duration");
		auto sidecar = AssetPathResolver::Resolve(EduHumanModels[2]);
		sidecar.replace_extension(".anim_events.json");
		std::string error;
		const auto events = AnimationEvents::Load(sidecar, error);
		Require(events && error.empty() && events->events.size() == 2,
			"The distributed EduHuman kick must include its authored combo-window sidecar");
		for (std::size_t index = 0; index < events->events.size(); ++index)
		{
			const auto& event = events->events[index];
			Require(event.animation == kick->animations.front().name &&
				event.type == (index == 0 ? "ComboWindowOpen" : "ComboWindowClose") &&
				std::abs(event.time - (index == 0 ? 0.5 : 1.2)) < 0.000001,
				"EduHuman combo events must target the exported kick clip at 0.5 and 1.2 seconds");
		}
		SkinnedModel model;
		NoGpuDevice device;
		model.Prepare(assets, EduHumanModels[0], ModelScaleSettings::OriginalSize());
		model.PrepareAnimation(assets, "Attack", EduHumanModels[2]);
		model.Activate(device, SkinningMode::Gpu);
		Require(model.PlayAnimation("Attack", { AnimationPlaybackMode::Once, true }), "The imported kick accepts the gameplay alias");
		model.Update(0.49f);
		Require(model.ConsumeAnimationEvents().empty(), "EduHuman combo input must remain closed before 0.5 seconds");
		model.Update(0.02f);
		auto fired = model.ConsumeAnimationEvents();
		Require(fired.size() == 1 && fired.front().event.type == "ComboWindowOpen" && fired.front().event.animation == "Attack",
			"The real kick sidecar must open the combo window through the runtime animation alias");
		model.Update(0.70f);
		fired = model.ConsumeAnimationEvents();
		Require(fired.size() == 1 && fired.front().event.type == "ComboWindowClose",
			"The real kick sidecar must close the combo window after 1.2 seconds");
		model.Update(1.0f);
		Require(model.IsAnimationFinished() && model.ConsumeAnimationEvents().empty(),
			"Finishing the EduHuman kick must not repeat its combo events");
	}

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

#define MODELPREPARATIONTESTS_CASES(TEST) \
	TEST(EduHumanSkeletonAndSkinWeights, "EduHuman complete skeleton and compatible skin weights", Cpu) \
	TEST(EduHumanClipsAnimateOneSkeleton, "EduHuman idle, jog and kick animate one skeleton", Cpu) \
	TEST(EduHumanKickLoadsAuthoredComboEvents, "EduHuman authored kick and combo-window sidecar", Cpu) \
	TEST(WorkerPrepareThenActivationWithoutSource, "worker preparation and activation without source", Cpu) \
	TEST(FailedImportsDoNotPoisonCache, "failed model import remains retryable", Cpu) \
	TEST(CacheLifetimeAndSynchronousCompatibility, "cache lifetime and synchronous compatibility", Cpu) \
	TEST(ExplicitModelPlaybackRequests, "explicit model playback and rejected requests", Cpu) \
	TEST(OneShotModelEventsFireOnce, "one-shot model events fire once", Cpu) \
	TEST(PlayerRejectsOutOfWindowAndHeldInput, "player rejects early, late and held combo input", Cpu) \
	TEST(PlayerQueuesOneComboAndPreservesFinalEvents, "player queues one combo and preserves final-frame events", Cpu) \
	TEST(PlayerUsesEditedComboWindowEvents, "player uses edited combo-window events", Cpu) \
	TEST(PlayerWithoutLocomotionClipsCanLeaveAttack, "player without locomotion clips exits attack", Cpu)

GAME_TEST_SUITE(ModelPreparationTests, MODELPREPARATIONTESTS_CASES)
