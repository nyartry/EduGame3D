#include "TestSupport.h"
#include "Framework/Animation/RootMotion.h"
#include "Framework/Animation/RootMotionExtractor.h"
#include "Framework/Animation/AnimationSampler.h"
#include "Framework/Rendering/Core/IRenderDevice.h"
#include "Game/Gameplay/CharacterGrounding.h"
#include "Game/Gameplay/Cube.h"
#include "Game/Gameplay/Ground.h"

#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

using namespace DirectX;

void TestAnimationEvents();

namespace
{
	constexpr float Step = 1.0f / 60.0f;
	constexpr std::array Modes{ RootMotionMode::Ignore, RootMotionMode::Blend, RootMotionMode::Apply };

	void Require(bool condition, const char* message)
	{
		if (!condition)
		{
			throw std::runtime_error(message);
		}
	}

	void Near(float actual, float expected, const char* message)
	{
		if (!std::isfinite(actual) || std::abs(actual - expected) > 0.0001f)
		{
			throw std::runtime_error(std::string(message) + ": expected " +
				std::to_string(expected) + ", got " + std::to_string(actual));
		}
	}

	void Near(const XMFLOAT3& actual, const XMFLOAT3& expected, const char* message)
	{
		Near(actual.x, expected.x, message);
		Near(actual.y, expected.y, message);
		Near(actual.z, expected.z, message);
	}

	// Geometry and collision stay real; only GPU resource creation is skipped.
	class NoGpuDevice final : public IRenderDevice
	{
	public:
		void CreateVertexBuffer(VertexBuffer&, const std::vector<Vertex>&) override {}
		void CreateTexturedVertexBuffer(TexturedVertexBuffer&, const std::vector<TexturedVertex>&) override {}
		void CreateSkinnedVertexBuffer(SkinnedVertexBuffer&, const std::vector<SkinnedVertex>&) override {}
		void CreateSpriteVertexBuffer(SpriteVertexBuffer&, std::uint32_t) override {}
		void CreateSolidColorSpriteMaterial(SpriteMaterial&, std::uint8_t, std::uint8_t, std::uint8_t, std::uint8_t) override {}
		void CreateTextureSpriteMaterial(SpriteMaterial&, const std::string&, bool) override {}
		void CreatePixelSpriteMaterial(SpriteMaterial&, const std::vector<std::uint8_t>&, std::uint32_t, std::uint32_t, bool) override {}
		void CreateTexturedMaterial(TexturedMaterial&, const std::string&, const std::string&, const std::string&) override {}
	};

	void BlendWeightsAndYaw()
	{
		const XMFLOAT3 program{ 2.0f, 3.0f, 4.0f };
		const RootMotionDelta root{ { 10.0f, 20.0f, 30.0f } };
		RootMotionSettings settings;
		Require(settings.verticalMode == RootMotionVerticalMode::Ignore, "Animation Y must be opt-in");
		settings.mode = RootMotionMode::Ignore;
		Near(ResolveRootMotionDisplacement(program, root, 0.0f, settings), program, "Ignore preserves program movement");
		settings.mode = RootMotionMode::Apply;
		Near(ResolveRootMotionDisplacement(program, root, 0.0f, settings), { 10.0f, 3.0f, 30.0f }, "Apply replaces XZ only");
		settings.mode = RootMotionMode::Blend;
		for (const float weight : { -2.0f, 0.0f, 0.5f, 1.0f, 2.0f })
		{
			settings.blendWeight = weight;
			const float expectedWeight = weight < 0.0f ? 0.0f : (weight > 1.0f ? 1.0f : weight);
			Near(ResolveRootMotionDisplacement(program, root, 0.0f, settings),
				{ 2.0f + 8.0f * expectedWeight, 3.0f, 4.0f + 26.0f * expectedWeight }, "Blend and clamped endpoints");
		}
		for (const float weight : { std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity() })
		{
			settings.blendWeight = weight;
			Near(ResolveRootMotionDisplacement(program, root, 0.0f, settings), program, "Invalid blend weight is ignored");
		}
		settings.blendWeight = 0.5f;
		settings.verticalMode = RootMotionVerticalMode::Apply;
		Near(ResolveRootMotionDisplacement(program, root, 0.0f, settings), { 6.0f, 13.0f, 17.0f }, "Animation Y is additive");
		settings.mode = RootMotionMode::Apply;
		Near(ResolveRootMotionDisplacement(program, root, XM_PIDIV2, settings), { 30.0f, 23.0f, -10.0f }, "Model-local root rotates into world space");
		settings.mode = RootMotionMode::Ignore;
		Near(ResolveRootMotionDisplacement(program, root, XM_PIDIV2, settings), program, "Ignore also ignores opted-in animation Y");
	}

	void RootExtractionAcrossLoops()
	{
		AnimationClip clip;
		clip.durationTicks = 30.0;
		clip.rootMotionBoneAnimationIndex = 0;
		BoneAnimation animation;
		animation.boneIndex = 0;
		animation.translations = { { 0.0, { 3.0f, 1.0f, 2.0f } }, { 30.0, { 4.0f, 3.0f, 5.0f } } };
		clip.boneAnimations.push_back(animation);
		std::vector<BoneData> bones(1);
		XMStoreFloat4x4(&bones[0].localBindTransform, XMMatrixIdentity());
		for (const double start : { 0.0, 0.25, 0.99 })
		{
			for (const double delta : { 0.0, 0.02, 1.0, 2.25, 10.0 })
			{
				AnimationPlayback playback;
				playback.Seek(start, 1.0);
				const auto interval = playback.Advance(delta, 1.0);
				const auto root = ExtractRootMotionDelta(clip, bones, interval, 2.0f);
				Near(root.translation, { static_cast<float>(2.0 * delta), static_cast<float>(4.0 * delta), static_cast<float>(6.0 * delta) },
					"Every whole and partial loop must contribute scaled XYZ motion");
				Require(interval.completedLoops == static_cast<std::uint64_t>(std::floor(start + delta)), "Complete loop count");
				Near(static_cast<float>(playback.GetLocalTimeSeconds()), static_cast<float>(std::fmod(start + delta, 1.0)), "Shared pose endpoint");
			}
		}
		AnimationPlayback playback;
		clip.ticksPerSecond = 300.0; // Duration 0.1 is not exactly representable.
		Near(ExtractRootMotionDelta(clip, bones, playback.Advance(1.0, GetAnimationDurationSeconds(clip))).translation,
			{ 10.0f, 20.0f, 30.0f }, "Remainder and quotient agree for decimal clip duration");
		clip.ticksPerSecond = 30.0;
		playback.Seek(0.9, 1.0);
		// Clip endpoint is midway through the second key, not its value.
		clip.boneAnimations[0].translations.back().time = 60.0;
		Near(ExtractRootMotionDelta(clip, bones, playback.Advance(2.2, 1.0)).translation,
			{ 1.1f, 2.2f, 3.3f }, "Cycle displacement samples actual duration");
		clip.boneAnimations[0].translations.clear();
		Near(ExtractRootMotionDelta(clip, bones, playback.Advance(3.0, 1.0)).translation, {}, "Bind pose has no root motion");
		clip.boneAnimations[0].boneIndex = -1;
		Near(ExtractRootMotionDelta(clip, bones, playback.Advance(1.0, 1.0)).translation, {}, "Invalid bone is ignored");
	}

	void PlaybackSeekAndInvalidTime()
	{
		AnimationPlayback playback;
		playback.Seek(1234567890.25, 1.0);
		Near(static_cast<float>(playback.GetLocalTimeSeconds()), 0.25f, "Seek stores local time without long-running float drift");
		for (const double delta : { 0.0, -1.0, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN(), 1e30 })
		{
			Require(!playback.Advance(delta, 1.0).advanced, "Invalid advance emits no interval");
			Near(static_cast<float>(playback.GetLocalTimeSeconds()), 0.25f, "Invalid advance preserves time");
		}
		playback.Seek(1.0, 1.0);
		Near(static_cast<float>(playback.GetLocalTimeSeconds()), 0.0f, "Seek to exact duration selects first pose");
		Require(!playback.Advance(1.0, 0.0).advanced, "Zero duration is stationary");
		AnimationClip clip;
		clip.durationTicks = 1.0;
		clip.ticksPerSecond = 0.0;
		Near(static_cast<float>(GetAnimationDurationSeconds(clip)), 0.0f, "Invalid tick rate has zero duration");
	}

	void OncePlaybackStopsAndRestarts()
	{
		AnimationPlayback playback;
		Require(playback.GetMode() == AnimationPlaybackMode::Loop && !playback.IsFinished(), "Default playback keeps looping");
		for (const double finalDelta : { 0.75, 2.0, (std::numeric_limits<double>::max)() })
		{
			playback.Reset(AnimationPlaybackMode::Once);
			Require(playback.GetMode() == AnimationPlaybackMode::Once && !playback.IsFinished() &&
				playback.GetLocalTimeSeconds() == 0.0, "Once reset clears the old completion and time");
			const auto partial = playback.Advance(0.25, 1.0);
			Require(partial.advanced && partial.fromSeconds == 0.0 && partial.toSeconds == 0.25 &&
				partial.completedLoops == 0 && !playback.IsFinished(), "Once advances through the clip without wrapping");
			for (const double invalidDelta : { 0.0, -1.0, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN() })
			{
				const auto stationary = playback.Advance(invalidDelta, 1.0);
				Require(!stationary.advanced && stationary.completedLoops == 0 &&
					stationary.fromSeconds == 0.25 && stationary.toSeconds == 0.25 && !playback.IsFinished(),
					"Invalid Once delta preserves playback and completion state");
			}
			const auto terminal = playback.Advance(finalDelta, 1.0);
			Require(terminal.advanced && terminal.fromSeconds == 0.25 && terminal.toSeconds == 1.0 &&
				terminal.completedLoops == 0 && playback.IsFinished() && playback.GetLocalTimeSeconds() == 1.0,
				"Exact and overshooting Once advances stop at the final pose");
			const auto finished = playback.Advance(10.0, 1.0);
			Require(!finished.advanced && finished.completedLoops == 0 && finished.fromSeconds == 1.0 &&
				finished.toSeconds == 1.0 && playback.IsFinished(), "Finished Once playback remains stationary");
		}

		playback.Seek(-1.0, 1.0);
		Require(playback.GetMode() == AnimationPlaybackMode::Once && playback.GetLocalTimeSeconds() == 0.0 &&
			!playback.IsFinished(), "Once seek clamps negative time and retains its mode");
		playback.Seek(2.0, 1.0);
		Require(playback.GetLocalTimeSeconds() == 1.0 && playback.IsFinished(), "Once seek beyond the end finishes without wrapping");
		playback.Seek(0.5, 1.0);
		Require(playback.GetLocalTimeSeconds() == 0.5 && !playback.IsFinished(), "Seeking inside Once allows playback to continue");
		playback.Seek(std::numeric_limits<double>::quiet_NaN(), 1.0);
		Require(playback.GetLocalTimeSeconds() == 0.0 && !playback.IsFinished(), "Invalid Once seek time resets to zero");
		for (const double invalidDuration : { 0.0, -1.0, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN() })
		{
			playback.Reset(AnimationPlaybackMode::Once);
			Require(!playback.Advance(0.0, invalidDuration).advanced && playback.IsFinished(), "Invalid Once duration finishes immediately");
			Require(!playback.Advance(1.0, 1.0).advanced && playback.IsFinished(), "Invalid-duration completion persists until an explicit restart");
			playback.Reset(AnimationPlaybackMode::Once);
			playback.Seek(0.25, invalidDuration);
			Require(playback.GetLocalTimeSeconds() == 0.0 && playback.IsFinished(), "Seeking an invalid Once duration also finishes");
		}
		playback.Reset();
		Require(playback.GetMode() == AnimationPlaybackMode::Loop && !playback.IsFinished(), "Default reset restores looping");
		const auto loop = playback.Advance(1.25, 1.0);
		Require(loop.completedLoops == 1 && loop.toSeconds == 0.25 && !playback.IsFinished(), "Returning to Loop retains existing wrap behavior");
	}

	void OnceRootMotionStopsAtClipEnd()
	{
		AnimationClip clip;
		clip.durationTicks = 30.0;
		clip.rootMotionBoneAnimationIndex = 0;
		BoneAnimation animation;
		animation.boneIndex = 0;
		// The final key extends beyond the clip, so extraction must sample its actual endpoint.
		animation.translations = { { 0.0, { 3.0f, 1.0f, 2.0f } }, { 60.0, { 5.0f, 5.0f, 8.0f } } };
		clip.boneAnimations.push_back(animation);
		std::vector<BoneData> bones(1);
		XMStoreFloat4x4(&bones[0].localBindTransform, XMMatrixIdentity());
		AnimationPlayback playback;
		for (int play = 0; play < 2; ++play)
		{
			playback.Reset(AnimationPlaybackMode::Once);
			XMFLOAT3 total{};
			for (const double delta : { 0.25, 0.5, 5.0, 5.0 })
			{
				const auto interval = playback.Advance(delta, 1.0);
				const auto root = ExtractRootMotionDelta(clip, bones, interval, 2.0f);
				total.x += root.translation.x;
				total.y += root.translation.y;
				total.z += root.translation.z;
				Require(interval.completedLoops == 0, "Once root extraction must never include another cycle");
			}
			Near(total, { 2.0f, 4.0f, 6.0f }, "Overshoot and finished updates contribute only one clip of scaled root motion on every replay");
			Near(ExtractRootMotionDelta(clip, bones, playback.Advance(1.0, 1.0), 2.0f).translation,
				{}, "Finished Once playback emits no more root displacement");
		}
	}

	void DefaultModesPreserveJump()
	{
		Ground ground;
		CharacterGroundProbe probe;
		probe.SetGround(&ground);
		for (const RootMotionMode mode : Modes)
		{
			RootMotionSettings settings;
			settings.mode = mode;
			CharacterVerticalMotion baseline;
			CharacterVerticalMotion candidate;
			XMFLOAT3 reference{};
			XMFLOAT3 position{};
			baseline.Update(Step, false, reference, probe);
			candidate.Update(Step, false, position, probe);
			float peak = 0.0f;
			bool landed = false;
			for (int frame = 0; frame < 120; ++frame)
			{
				const auto movement = ResolveRootMotionDisplacement(
					{ 0.01f, 0.0f, 0.0f }, { { 0.02f, -5.0f, 0.01f } }, 0.0f, settings);
				position.x += movement.x;
				position.z += movement.z;
				const bool jumped = candidate.Update(Step, frame == 0, position, probe, movement.y);
				const bool referenceJumped = baseline.Update(Step, frame == 0, reference, probe);
				Require(jumped == referenceJumped && jumped == (frame == 0), "Jump is accepted exactly once in every root mode");
				Near(position.y, reference.y, "Root mode must preserve program jump trajectory");
				Near(candidate.GetVerticalVelocity(), baseline.GetVerticalVelocity(), "Gravity is identical in every root mode");
				Require(candidate.IsGrounded() == baseline.IsGrounded(), "Landing time must be unchanged");
				if (position.y > peak) peak = position.y;
				if (frame > 0 && candidate.IsGrounded()) landed = true;
			}
			Require(peak > 1.0f && landed, "Character must rise and land in every root mode");
		}
	}

	void ExplicitAnimationYPreservesWholeJump()
	{
		Ground ground;
		CharacterGroundProbe probe;
		probe.SetGround(&ground);
		CharacterVerticalMotion baseline;
		CharacterVerticalMotion candidate;
		XMFLOAT3 reference{};
		XMFLOAT3 position{};
		baseline.Update(Step, false, reference, probe);
		candidate.Update(Step, false, position, probe);
		bool sawAscent = false;
		bool sawDescent = false;
		bool landed = false;
		for (int frame = 0; frame < 120; ++frame)
		{
			const bool wantsJump = frame == 0 || frame == 2;
			const float animationY = candidate.GetVerticalVelocity() >= 0.0f ? -5.0f : 5.0f;
			const bool jumped = candidate.Update(Step, wantsJump, position, probe, animationY);
			baseline.Update(Step, wantsJump, reference, probe);
			Require(jumped == (frame == 0), "Airborne jump request must be rejected");
			Near(position.y, reference.y, "Animation Y must not override ascent or descent of program jump");
			Near(candidate.GetVerticalVelocity(), baseline.GetVerticalVelocity(), "Animation Y must not alter jump velocity");
			sawAscent |= candidate.GetVerticalVelocity() > 0.0f;
			sawDescent |= candidate.GetVerticalVelocity() < 0.0f;
			if (frame > 0 && candidate.IsGrounded())
			{
				landed = true;
				break;
			}
		}
		Require(sawAscent && sawDescent && landed, "Test must exercise a complete jump");
		candidate.Update(Step, false, position, probe, 0.25f);
		Require(position.y > 0.2f, "Animation Y must resume after landing");
	}

	void RootOnlyMotionWithoutGravity()
	{
		Ground ground;
		CharacterGroundProbe probe;
		probe.SetGround(&ground);
		CharacterVerticalMotion motion;
		XMFLOAT3 position{};
		motion.Update(Step, false, position, probe);
		motion.SetGravityEnabled(false);
		for (int frame = 0; frame < 20; ++frame)
		{
			Require(!motion.Update(Step, true, position, probe, 0.1f), "Gravity-disabled movement must not report a program jump");
			Near(position.y, static_cast<float>(frame + 1) * 0.1f, "Animation Y must continue with gravity disabled");
			Near(motion.GetVerticalVelocity(), 0.0f, "Gravity-disabled movement must not gain vertical velocity");
		}

		for (const bool useSetSettings : { false, true })
		{
			CharacterVerticalMotion jumping;
			XMFLOAT3 airborne{};
			jumping.Update(Step, false, airborne, probe);
			Require(jumping.Update(Step, true, airborne, probe), "Initial program jump must be accepted");
			if (useSetSettings)
			{
				CharacterVerticalMotionSettings settings;
				settings.gravityEnabled = false;
				jumping.SetSettings(settings);
			}
			else
			{
				jumping.SetGravityEnabled(false);
			}
			const float previousY = airborne.y;
			Require(!jumping.Update(Step, true, airborne, probe, 0.25f), "Disabling gravity during a jump must clear program jump state");
			Near(airborne.y, previousY + 0.25f, "Animation Y must resume when gravity is disabled during a jump");
		}
	}

	void DownwardAnimationLandsOnPlatform()
	{
		NoGpuDevice device;
		Cube platform;
		platform.Initialize(device);
		platform.SetPosition(0.0f, 1.0f, 0.0f);
		CharacterGroundProbe probe;
		probe.AddLandingSurface(&platform);
		for (const bool gravityEnabled : { false, true })
		{
			CharacterVerticalMotion motion;
			motion.SetGravityEnabled(gravityEnabled);
			XMFLOAT3 position{ 0.0f, 3.0f, 0.0f };
			Require(!motion.Update(Step, false, position, probe, -2.0f), "Root-only descent must not report a program jump");
			Near(position.y, 1.5f, "Root descent must retain previous Y when crossing a platform");
			Require(motion.IsGrounded(), "Root descent must land on the platform with gravity on or off");
			Near(motion.GetVerticalVelocity(), 0.0f, "Landing must stop vertical velocity");
		}
	}
}

#define ROOTMOTIONTESTS_CASES(TEST) \
	TEST(BlendWeightsAndYaw, "blend weights, Y policy, and yaw", Cpu) \
	TEST(RootExtractionAcrossLoops, "root extraction across complete and partial loops", Cpu) \
	TEST(PlaybackSeekAndInvalidTime, "playback seeks and invalid time", Cpu) \
	TEST(OncePlaybackStopsAndRestarts, "once playback stops, seeks, and restarts", Cpu) \
	TEST(OnceRootMotionStopsAtClipEnd, "once root motion stops at clip end", Cpu) \
	TEST(TestAnimationEvents, "animation events and shared editor/runtime JSON", Cpu) \
	TEST(DefaultModesPreserveJump, "default modes preserve jump", Cpu) \
	TEST(ExplicitAnimationYPreservesWholeJump, "explicit animation Y preserves whole jump", Cpu) \
	TEST(RootOnlyMotionWithoutGravity, "root-only motion without gravity", Cpu) \
	TEST(DownwardAnimationLandsOnPlatform, "downward animation lands on platform", Cpu)

GAME_TEST_SUITE(RootMotionTests, ROOTMOTIONTESTS_CASES)
