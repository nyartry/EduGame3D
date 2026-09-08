#include "Framework/Scene/Core/SceneManager.h"
#include "Framework/Rendering/Core/IRenderDevice.h"
#include "Framework/Rendering/Core/IRenderResourceLifetime.h"
#include "Framework/Rendering/Core/RenderResourceAccess.h"

#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
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
		void CreateVertexBuffer(VertexBuffer&, const std::vector<Vertex>&) override {}
		void CreateTexturedVertexBuffer(TexturedVertexBuffer&, const std::vector<TexturedVertex>&) override {}
		void CreateSkinnedVertexBuffer(SkinnedVertexBuffer&, const std::vector<SkinnedVertex>&) override {}
		void CreateSpriteVertexBuffer(SpriteVertexBuffer& buffer, std::uint32_t capacity) override
		{
			// Sprite storage is CPU-backed until a real renderer submits the draw.
			RenderResourceAccess::Initialize(buffer, nullptr, capacity);
		}
		void CreateSolidColorSpriteMaterial(SpriteMaterial&, std::uint8_t, std::uint8_t, std::uint8_t, std::uint8_t) override {}
		void CreateTextureSpriteMaterial(SpriteMaterial&, const std::string&, bool) override {}
		void CreatePixelSpriteMaterial(SpriteMaterial&, const std::vector<std::uint8_t>&, std::uint32_t, std::uint32_t, bool) override {}
		void CreateTexturedMaterial(TexturedMaterial&, const std::string&, const std::string&, const std::string&) override {}
	};

	class DeferredLifetime final : public IRenderResourceLifetime
	{
	public:
		void DeferRelease(std::function<void()> release) override { pending.push_back(std::move(release)); }
		void CompleteGpuWork()
		{
			auto completed = std::move(pending);
			pending.clear();
			for (auto& release : completed) release();
		}
		std::vector<std::function<void()>> pending;
	};

	struct State
	{
		std::atomic<int> prepared{}, activated{}, unloaded{}, destroyed{}, updates{}, frames{}, failedLoads{};
		bool failPrepare{}, failActivate{};
		std::thread::id prepareThread, activateThread;
		std::promise<void> prepareEntered;
		std::shared_future<void> allowPrepare;
		std::string request;
		bool requestAsync{};
	};

	class TestScene final : public IScene
	{
	public:
		explicit TestScene(std::shared_ptr<State> state) : m_state(std::move(state)) {}
		~TestScene() override { ++m_state->destroyed; }
		void Prepare() override
		{
			m_state->prepareThread = std::this_thread::get_id();
			++m_state->prepared;
			m_state->prepareEntered.set_value();
			if (m_state->allowPrepare.valid()) m_state->allowPrepare.wait();
			if (m_state->failPrepare) throw std::runtime_error("prepare failure");
		}
		void Activate() override
		{
			m_state->activateThread = std::this_thread::get_id();
			++m_state->activated;
			if (m_state->failActivate) throw std::runtime_error("activate failure");
		}
		void Unload() override { ++m_state->unloaded; }
		void Update(float, const Input&) override { ++m_state->updates; }
		void UpdateFrame(float, const Input&) override { ++m_state->frames; }
		RenderView GetRenderView() const override { return {}; }
		std::string GetRequestedSceneName() const override { return m_state->request; }
		bool ShouldLoadRequestedSceneAsync() const override { return m_state->requestAsync; }
		void OnSceneLoadFailed() override { ++m_state->failedLoads; m_state->request.clear(); }
	private:
		std::shared_ptr<State> m_state;
	};

	struct Fixture
	{
		NoGpuDevice device;
		DeferredLifetime lifetime;
		Input input;
		SceneManager manager;
		Fixture() { manager.Initialize(device, lifetime, 640, 480); }
		void Register(const char* name, const std::shared_ptr<State>& state)
		{
			manager.RegisterScene(name, [state]() { return std::make_unique<TestScene>(state); });
		}
		void FinishTransition()
		{
			const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
			while (manager.IsLoading() && std::chrono::steady_clock::now() < deadline)
			{
				manager.UpdateFrame(0.25f, input);
				std::this_thread::yield();
			}
			Require(!manager.IsLoading(), "Async transition must finish within five seconds");
		}
	};

	void SynchronousFailureKeepsOldScene()
	{
		for (int failure = 0; failure < 4; ++failure)
		{
			Fixture fixture;
			auto old = std::make_shared<State>();
			fixture.Register("old", old);
			Require(fixture.manager.LoadScene("old"), "Initial scene loads synchronously");
			auto next = std::make_shared<State>();
			next->failPrepare = failure == 2;
			next->failActivate = failure == 3;
			if (failure == 0) fixture.manager.RegisterScene("next", []() -> std::unique_ptr<IScene> { throw std::runtime_error("factory failure"); });
			else if (failure == 1) fixture.manager.RegisterScene("next", []() -> std::unique_ptr<IScene> { return nullptr; });
			else fixture.Register("next", next);
			Require(!fixture.manager.LoadScene("next"), "Factory/null/Prepare/Activate failure returns false");
			Require(!fixture.manager.GetLastLoadError().empty() && !fixture.manager.IsLoading(), "Failed load provides error and resets transition");
			Require(old->unloaded == 0 && old->destroyed == 0 && fixture.lifetime.pending.empty(), "Failed replacement does not retire current scene");
			Require(old->failedLoads == 1, "Old scene notified once to clear its request");
			fixture.manager.Update(1.0f / 60.0f, fixture.input);
			fixture.manager.UpdateFrame(1.0f / 144.0f, fixture.input);
			Require(old->updates == 1 && old->frames == 1, "Old scene resumes simulation and presentation");
			if (failure >= 2) Require(next->destroyed == 1, "Failed candidate is destroyed");
		}
	}

	void DeferredRetirementAndAdditiveLoad()
	{
		Fixture fixture;
		auto old = std::make_shared<State>();
		auto added = std::make_shared<State>();
		auto replacement = std::make_shared<State>();
		fixture.Register("old", old);
		fixture.Register("added", added);
		fixture.Register("replacement", replacement);
		Require(fixture.manager.LoadScene("old"), "Old scene loads");
		Require(fixture.manager.LoadScene("added", SceneLoadType::Synchronous, SceneLoadMode::Additive), "Additive load succeeds");
		fixture.manager.Update(0.016f, fixture.input);
		Require(old->updates == 1 && added->updates == 1 && old->unloaded == 0, "Additive scenes both simulate");
		Require(fixture.manager.LoadScene("replacement"), "Replacement activates before commit");
		Require(replacement->activated == 1 && fixture.lifetime.pending.size() == 2, "Replaced scenes retire only after successful activation");
		Require(old->unloaded == 0 && added->unloaded == 0, "GPU-in-flight scenes remain loaded");
		fixture.manager.Update(0.016f, fixture.input);
		Require(old->updates == 1 && added->updates == 1 && replacement->updates == 1, "Only committed scene simulates");
		fixture.lifetime.CompleteGpuWork();
		Require(old->unloaded == 1 && old->destroyed == 1 && added->unloaded == 1 && added->destroyed == 1,
			"GPU completion unloads and destroys each retired scene once");
	}

	void AsynchronousPrepareKeepsOldScene()
	{
		for (const bool failPrepare : { false, true })
		{
			Fixture fixture;
			auto old = std::make_shared<State>();
			auto next = std::make_shared<State>();
			next->failPrepare = failPrepare;
			std::promise<void> releasePrepare;
			next->allowPrepare = releasePrepare.get_future().share();
			auto entered = next->prepareEntered.get_future();
			fixture.Register("old", old);
			fixture.Register("next", next);
			Require(fixture.manager.LoadScene("old"), "Old scene loads");
			Require(fixture.manager.LoadScene("next", SceneLoadType::Asynchronous), "Async request accepted");
			Require(!fixture.manager.LoadScene("next"), "Concurrent transition rejected");
			fixture.manager.Update(0.016f, fixture.input);
			fixture.manager.UpdateFrame(0.5f, fixture.input);
			const bool workerStarted = entered.wait_for(std::chrono::seconds(5)) == std::future_status::ready;
			// Always release the worker before any throwing assertion/destructor.
			const bool oldRetained = old->unloaded == 0 && old->destroyed == 0 && fixture.lifetime.pending.empty();
			fixture.manager.UpdateFrame(1.0f, fixture.input);
			const bool notActivatedEarly = next->activated == 0;
			releasePrepare.set_value();
			Require(workerStarted && oldRetained && notActivatedEarly, "Pending CPU work retains old scene and delays activation");
			Require(old->updates == 0 && old->frames == 0, "Fade out and loading pause old simulation and scene UI");
			fixture.FinishTransition();
			Require(next->prepareThread != std::this_thread::get_id(), "Async Prepare uses worker thread");
			fixture.manager.Update(0.016f, fixture.input);
			if (failPrepare)
			{
				Require(next->activated == 0 && next->destroyed == 1, "Failed Prepare never activates");
				Require(old->updates == 1 && old->unloaded == 0 && old->failedLoads == 1, "Failed async load resumes old scene");
			}
			else
			{
				Require(next->activateThread == std::this_thread::get_id(), "Async candidate activates on main thread");
				Require(next->updates == 1 && old->updates == 0 && old->unloaded == 0, "Committed scene runs while old GPU resources remain alive");
				fixture.lifetime.CompleteGpuWork();
				Require(old->unloaded == 1 && old->destroyed == 1, "Async replacement retires old scene after GPU completion");
			}
		}
	}

	void AsyncFactoryAndActivationFailures()
	{
		for (int failure = 0; failure < 3; ++failure)
		{
			Fixture fixture;
			auto old = std::make_shared<State>();
			fixture.Register("old", old);
			Require(fixture.manager.LoadScene("old"), "Old scene loads");
			if (failure == 0) fixture.manager.RegisterScene("next", []() -> std::unique_ptr<IScene> { return nullptr; });
			else if (failure == 1) fixture.manager.RegisterScene("next", []() -> std::unique_ptr<IScene> { throw 42; });
			else { auto next = std::make_shared<State>(); next->failActivate = true; fixture.Register("next", next); }
			Require(fixture.manager.LoadScene("next", SceneLoadType::Asynchronous), "Async request accepted");
			fixture.FinishTransition();
			Require(!fixture.manager.GetLastLoadError().empty() && old->failedLoads == 1 && old->unloaded == 0,
				"Async null/factory/activation failure reports and preserves current scene");
		}
	}

	void PresentationAndSimulationAreIndependent()
	{
		Fixture fixture;
		auto old = std::make_shared<State>();
		fixture.Register("old", old);
		Require(fixture.manager.LoadScene("old"), "Old scene loads");
		fixture.manager.UpdateFrame(1.0f / 144.0f, fixture.input);
		Require(old->frames == 1 && old->updates == 0, "A zero-step frame updates presentation only");
		fixture.manager.Update(1.0f / 60.0f, fixture.input);
		fixture.manager.Update(1.0f / 60.0f, fixture.input);
		Require(old->frames == 1 && old->updates == 2, "Catch-up simulation does not duplicate UI updates");
		fixture.manager.RegisterScene("bad", []() -> std::unique_ptr<IScene> { return nullptr; });
		old->request = "bad";
		fixture.manager.UpdateFrame(0.016f, fixture.input);
		Require(old->failedLoads == 1 && old->request.empty(), "Scene request failure callback resets request");
		fixture.manager.UpdateFrame(0.016f, fixture.input);
		Require(old->failedLoads == 1, "Failed scene request is not repeated every frame");
	}

	void UnregisteredSceneRequestKeepsActiveScene()
	{
		for (const bool requestAsync : { false, true })
		{
			Fixture fixture;
			auto active = std::make_shared<State>();
			fixture.Register("active", active);
			Require(fixture.manager.LoadScene("active"), "Initial active scene loads");
			const std::string missingName = "unregistered_scene";
			active->request = missingName;
			active->requestAsync = requestAsync;
			fixture.manager.UpdateFrame(0.016f, fixture.input);
			Require(active->request.empty() && active->failedLoads == 1,
				"An unregistered synchronous or asynchronous scene request is cleared and reports failure once");
			Require(fixture.manager.GetLastLoadError().find(missingName) != std::string::npos && !fixture.manager.IsLoading(),
				"The load error names the unregistered scene without starting a transition");
			Require(active->unloaded == 0 && active->destroyed == 0 && fixture.lifetime.pending.empty(),
				"An unregistered scene request preserves the active scene and its resources");
			fixture.manager.Update(0.016f, fixture.input);
			fixture.manager.UpdateFrame(0.016f, fixture.input);
			Require(active->updates == 1 && active->frames == 2, "The active scene continues simulation and frame updates after rejection");
			Require(active->failedLoads == 1 && active->request.empty(), "The rejected scene request does not notify again on the next frame");
		}
	}
}

int main()
{
	int failures = 0;
	const auto run = [&](const char* name, void (*test)())
	{
		try { test(); std::cout << "PASS " << name << '\n'; }
		catch (const std::exception& error) { ++failures; std::cerr << "FAIL " << name << ": " << error.what() << '\n'; }
	};
	run("sync load failures preserve active scene", SynchronousFailureKeepsOldScene);
	run("additive load and fence-deferred retirement", DeferredRetirementAndAdditiveLoad);
	run("async Prepare retention, affinity and recovery", AsynchronousPrepareKeepsOldScene);
	run("async factory and activation failure recovery", AsyncFactoryAndActivationFailures);
	run("presentation and fixed simulation contract", PresentationAndSimulationAreIndependent);
	run("unregistered scene requests preserve active scene", UnregisteredSceneRequestKeepsActiveScene);
	return failures == 0 ? 0 : 1;
}
