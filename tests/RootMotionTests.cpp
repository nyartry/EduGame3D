#include "Framework/Animation/RootMotion.h"
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

int main()
{
	int failures = 0;
	const auto run = [&failures](const char* name, void (*test)())
	{
		try
		{
			test();
			std::cout << "PASS " << name << '\n';
		}
		catch (const std::exception& error)
		{
			++failures;
			std::cerr << "FAIL " << name << ": " << error.what() << '\n';
		}
	};
	run("blend weights, Y policy, and yaw", BlendWeightsAndYaw);
	run("default modes preserve jump", DefaultModesPreserveJump);
	run("explicit animation Y preserves whole jump", ExplicitAnimationYPreservesWholeJump);
	run("root-only motion without gravity", RootOnlyMotionWithoutGravity);
	run("downward animation lands on platform", DownwardAnimationLandsOnPlatform);
	return failures == 0 ? 0 : 1;
}
