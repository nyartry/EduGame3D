#include "Framework/Core/Math/Aabb.h"
#include "Framework/Core/Time/FixedStepClock.h"
#include "Framework/Models/ModelFit.h"
#include "Framework/Physics/CollisionWorld.h"
#include "Framework/Rendering/Core/IRenderDevice.h"
#include "Game/Gameplay/CharacterGrounding.h"
#include "Game/Gameplay/Cube.h"
#include "Game/Gameplay/Ground.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

using namespace DirectX;

namespace
{
	void Require(bool condition, const char* message)
	{
		if (!condition) throw std::runtime_error(message);
	}

	void Near(float actual, float expected, const char* message)
	{
		if (!std::isfinite(actual) || std::abs(actual - expected) > 0.0001f)
		{
			throw std::runtime_error(std::string(message) + ": expected " +
				std::to_string(expected) + ", got " + std::to_string(actual));
		}
	}

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

	class CountingSurface final : public ICollisionSurface
	{
	public:
		mutable int calls{};
		float height{ 1.0f };
		bool TryGetHeightAt(const XMFLOAT3&, float, float& result) const override
		{
			++calls;
			result = height;
			return true;
		}
	};

	void FloorSelectionAndRemoval()
	{
		NoGpuDevice device;
		Ground ground;
		Cube platform;
		platform.Initialize(device);
		platform.SetPosition(0, 1, 0);
		CollisionWorld world;
		world.RegisterSurface(ground);
		world.RegisterSurface(platform, &platform);
		FloorHit hit;
		Require(world.TryFindFloor({ { 0, -1, 0 }, 0.35f, 3, -1 }, hit), "Swept descent must hit a floor");
		Near(hit.height, 1.5f, "Highest crossed platform wins over ground");
		Require(hit.surface == &platform, "Floor hit reports its source surface");
		Require(world.TryFindFloor({ { 0, 1, 0 }, 0.35f, 0.5f, 1 }, hit), "Underlying ground remains queryable");
		Require(hit.surface == &ground, "Passing upward from below must not land on a platform");
		platform.SetSurfaceCollisionEnabled(false);
		Require(world.TryFindFloor({ { 0, 0, 0 }, 0.35f, 3, 0 }, hit) && hit.surface == &ground,
			"Disabled surface is skipped");
		platform.SetSurfaceCollisionEnabled(true);
		platform.SetPosition(4, 2, 0);
		Require(world.TryFindFloor({ { 4, 0, 0 }, 0.35f, 4, 0 }, hit), "Moved registered geometry is read live");
		Near(hit.height, 2.5f, "Moving a platform needs no re-registration");
		world.RemoveSurface(platform);
		Require(world.TryFindFloor({ { 4, 0, 0 }, 0.35f, 4, 0 }, hit) && hit.surface == &ground,
			"Removing a platform removes its floor");
		ground.SetCollisionEnabled(false);
		Require(!world.TryFindFloor({ {}, 0.35f, 0, 0 }, hit) && hit.surface == nullptr,
			"Missing floor clears the previous hit");
	}

	void RegistrationAndInvalidQueries()
	{
		CountingSurface surface;
		CollisionWorld world;
		world.RegisterSurface(surface);
		world.RegisterSurface(surface);
		FloorHit hit;
		Require(world.TryFindFloor({ {}, 0.35f, 2, 0 }, hit), "Registered abstract surface is queryable");
		Require(surface.calls == 1, "Duplicate registration must query each surface only once");
		Require(!world.TryFindFloor({ {}, 0.35f, 2, 0, 0.05f, &surface }, hit), "Self surface is excluded");
		Require(!world.TryFindFloor({ {}, -1, 2, 0 }, hit), "Negative radius is invalid");
		Require(!world.TryFindFloor({ {}, 1, std::numeric_limits<float>::quiet_NaN(), 0 }, hit), "NaN sweep is rejected");
		surface.height = std::numeric_limits<float>::infinity();
		Require(!world.TryFindFloor({ {}, 1, 2, 0 }, hit), "Nonfinite provider height is rejected");
		world.Clear();
		Require(!world.TryFindFloor({ {}, 1, 2, 0 }, hit), "Clear removes every registration");
	}

	void UnifiedSidesAndBoundaries()
	{
		NoGpuDevice device;
		Ground ground;
		Cube movable;
		Cube blocker;
		movable.Initialize(device);
		blocker.Initialize(device);
		movable.SetPosition(0, 0.5f, 0);
		blocker.SetPosition(0.5f, 0.5f, 0);
		CollisionWorld world;
		world.RegisterSurface(ground);
		world.RegisterSurface(blocker, &blocker);
		world.RegisterBody(movable);
		Require(world.ResolveBodyCollisions(movable), "World pushes a body away from registered blocker");
		Near(movable.GetPosition().x, 0.5f - 2 * movable.GetCollisionRadius(), "Side collision retains cylinder radius policy");
		world.RemoveSurface(blocker);
		movable.SetPosition(0.5f, 0.5f, 0);
		Require(!world.ResolveBodyCollisions(movable), "Removing a surface removes its paired side blocker and ignores self");
		movable.SetPosition(9, 0.5f, 0);
		Require(world.ResolveBodyCollisions(movable), "World also resolves arena bounds");
		Near(movable.GetPosition().x, 8 - movable.GetCollisionRadius(), "Boundary radius is maintained");
		XMFLOAT3 oversized{ 2, 0, 3 };
		Require(world.ResolveBoundaries(oversized, 20), "Oversized body resolves without invalid clamp bounds");
		Near(oversized.x, 0, "Oversized body is centered in X");
		Near(oversized.z, 0, "Oversized body is centered in Z");
	}

	void PrimitiveUsesSweptFloorsAndExcludesSelf()
	{
		NoGpuDevice device;
		Ground ground;
		Cube platform;
		Cube falling;
		platform.Initialize(device);
		falling.Initialize(device);
		platform.SetPosition(0, 1, 0);
		falling.SetPosition(0, 4, 0);
		CollisionWorld world;
		world.RegisterSurface(ground);
		world.RegisterSurface(platform, &platform);
		world.RegisterSurface(falling, &falling);
		falling.SetCollisionQuery(&world);
		for (int i = 0; i < 60; ++i) falling.Update(1.0f / 60.0f);
		Require(falling.IsGrounded(), "Primitive lands through the shared world query");
		Near(falling.GetCollisionBottomY(), platform.GetCollisionTopY(), "Primitive uses its bottom offset on platform landing");
		world.RemoveSurface(platform);
		for (int i = 0; i < 60; ++i) falling.Update(1.0f / 60.0f);
		Near(falling.GetCollisionBottomY(), 0, "Removing platform lets primitive fall to ground");
		world.RemoveSurface(ground);
		const float previousY = falling.GetPosition().y;
		falling.Update(1.0f / 60.0f);
		Require(!falling.IsGrounded() && falling.GetPosition().y < previousY, "Registered primitive cannot stand on itself");
	}

	void CharacterUsesAbstractWorld()
	{
		CountingSurface floor;
		CollisionWorld world;
		world.RegisterSurface(floor);
		CharacterGroundProbe probe;
		probe.SetCollisionQuery(&world);
		CharacterVerticalMotion motion;
		motion.SetGravityEnabled(false);
		XMFLOAT3 position{ 0, 3, 0 };
		motion.Update(1.0f / 60.0f, false, position, probe, -3);
		Require(motion.IsGrounded(), "Character can land on an abstract Framework surface");
		Near(position.y, 1, "Animation descent retains swept previous Y");
	}

	struct JumpResult { float peak{}; int landingStep{}; int jumps{}; };
	JumpResult SimulateJump(int framesPerSecond)
	{
		Ground ground;
		CollisionWorld world;
		world.RegisterSurface(ground);
		CharacterGroundProbe probe;
		probe.SetCollisionQuery(&world);
		CharacterVerticalMotion motion;
		FixedStepClock clock;
		XMFLOAT3 position{};
		motion.Update(clock.GetStepSeconds(), false, position, probe);
		JumpResult result;
		bool pendingJump = true;
		int simulationStep = 0;
		for (int frame = 0; frame < framesPerSecond * 2; ++frame)
		{
			const unsigned steps = clock.Advance(1.0 / framesPerSecond);
			for (unsigned step = 0; step < steps; ++step)
			{
				++simulationStep;
				result.jumps += motion.Update(clock.GetStepSeconds(), pendingJump, position, probe) ? 1 : 0;
				pendingJump = false;
				result.peak = (std::max)(result.peak, position.y);
				if (motion.IsGrounded() && result.landingStep == 0) result.landingStep = simulationStep;
			}
		}
		Require(simulationStep == 120, "Two seconds use identical simulation counts at every render rate");
		return result;
	}

	void FixedStepJumpAcrossFrameRatesAndStall()
	{
		const auto reference = SimulateJump(60);
		Require(reference.peak > 1 && reference.landingStep > 30 && reference.jumps == 1, "Reference jump completes");
		for (const int frameRate : { 30, 144 })
		{
			const auto result = SimulateJump(frameRate);
			Near(result.peak, reference.peak, "Render frame rate does not alter jump peak");
			Require(result.landingStep == reference.landingStep && result.jumps == 1, "Landing and jump acceptance are frame-rate independent");
		}
		Ground ground;
		CharacterGroundProbe probe;
		probe.SetGround(&ground);
		CharacterVerticalMotion motion;
		FixedStepClock clock;
		XMFLOAT3 position{};
		motion.Update(clock.GetStepSeconds(), false, position, probe);
		const unsigned steps = clock.Advance(0.5);
		Require(steps > 0 && steps <= 8, "Long stall bounds catch-up work");
		int jumps = 0;
		for (unsigned step = 0; step < steps; ++step)
		{
			jumps += motion.Update(clock.GetStepSeconds(), step == 0, position, probe) ? 1 : 0;
		}
		Require(jumps == 1 && !motion.IsGrounded() && position.y > 0.5f, "Half-second jump-start stall must not erase the jump");
	}

	void AabbAndModelFitContracts()
	{
		Aabb bounds;
		Near(bounds.Center().x, 0, "Empty center is zero");
		Require(bounds.IsEmpty(), "Default AABB is empty");
		Require(!bounds.AddPoint({ std::numeric_limits<float>::quiet_NaN(), 0, 0 }) && bounds.IsEmpty(), "Nonfinite points leave bounds empty");
		bounds.AddPoint({ -2, -3, -4 });
		bounds.AddPoint({ 4, 7, 6 });
		Near(bounds.Center().x, 1, "Center aggregates both points");
		Near(bounds.Size().y, 10, "Height aggregates both points");
		Require(!bounds.AddPoint({ 0, std::numeric_limits<float>::infinity(), 0 }), "Infinite point is rejected");
		Near(bounds.Max().y, 7, "Rejected point cannot corrupt extrema");
		ModelFit fit;
		Require(TryCreateModelFit(bounds, 2, fit), "Positive height can be fitted");
		Near(fit.Apply({ 1, -3, 1 }).y, 0, "Fit uses the feet as origin");
		Near(fit.Apply({ 1, 7, 1 }).y, 2, "Fit reaches target height");
		Near(fit.Apply({ 1, -3, 1 }).x, 0, "Fit centers the asset horizontally");
		Require(!TryCreateModelFit(bounds, -1, fit) && fit.scale == 1, "Invalid target gives identity fit");
		Aabb huge;
		const float maximum = (std::numeric_limits<float>::max)();
		huge.AddPoint({ maximum, -maximum, 0 });
		huge.AddPoint({ maximum, maximum, 0 });
		Require(std::isfinite(huge.Center().x) && std::isfinite(huge.Size().y), "Finite extreme points keep finite bounds results");
		Require(TryCreateModelFit(huge, 2, fit), "Wide finite span fits using double intermediates");
		Near(fit.Apply({ maximum, maximum, 0 }).y, 2, "Extreme fit avoids intermediate float overflow");
		PrimitiveObjectCollider collider;
		float height = 0;
		collider.RebuildFromVertices({});
		Require(!collider.TryGetTopSurfaceAt({}, {}, 1, height), "Empty collider must not create a phantom platform");
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
	run("floor selection and removal", FloorSelectionAndRemoval);
	run("registration and invalid queries", RegistrationAndInvalidQueries);
	run("unified sides and boundaries", UnifiedSidesAndBoundaries);
	run("primitive swept floors and self exclusion", PrimitiveUsesSweptFloorsAndExcludesSelf);
	run("character uses abstract world", CharacterUsesAbstractWorld);
	run("fixed-step jump at 30/60/144 FPS and stall", FixedStepJumpAcrossFrameRatesAndStall);
	run("AABB and model fit contracts", AabbAndModelFitContracts);
	return failures == 0 ? 0 : 1;
}
