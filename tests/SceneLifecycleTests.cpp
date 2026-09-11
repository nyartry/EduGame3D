#include "TestSupport.h"
#include "Framework/Scene/Core/SceneManager.h"
#include "Framework/Rendering/Core/IRenderDevice.h"
#include "Framework/Rendering/Core/IRenderResourceLifetime.h"
#include "Framework/Rendering/Core/RenderResourceAccess.h"
#include "Framework/Core/Diagnostics/Diagnostics.h"

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
		std::atomic<int> prepared{}, resized{}, activated{}, unloaded{}, destroyed{}, updates{}, frames{}, failedLoads{};
		bool failPrepare{}, failResize{}, failActivate{}, failNotification{}, failUnload{};
		std::thread::id prepareThread, resizeThread, activateThread;
		std::uint32_t width{}, height{}, activationWidth{}, activationHeight{};
		std::vector<std::string> lifecycle;
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
			m_state->lifecycle.push_back("prepare");
			m_state->prepareEntered.set_value();
			if (m_state->allowPrepare.valid()) m_state->allowPrepare.wait();
			if (m_state->failPrepare) throw std::runtime_error("prepare failure");
		}
		void OnResize(std::uint32_t width, std::uint32_t height) override
		{
			m_state->resizeThread = std::this_thread::get_id();
			m_state->width = width;
			m_state->height = height;
			m_state->lifecycle.push_back("resize");
			++m_state->resized;
			if (m_state->failResize) throw std::runtime_error("resize failure");
		}
		void Activate() override
		{
			m_state->activateThread = std::this_thread::get_id();
			m_state->activationWidth = m_state->width;
			m_state->activationHeight = m_state->height;
			m_state->lifecycle.push_back("activate");
			++m_state->activated;
			if (m_state->failActivate) throw std::runtime_error("activate failure");
		}
		void Unload() override
		{
			++m_state->unloaded;
			if (m_state->failUnload) throw std::runtime_error("unload failure");
		}
		void Update(float, const Input&) override { ++m_state->updates; }
		void UpdateFrame(float, const Input&) override { ++m_state->frames; }
		RenderView GetRenderView() const override { return {}; }
		std::string GetRequestedSceneName() const override { return m_state->request; }
		bool ShouldLoadRequestedSceneAsync() const override { return m_state->requestAsync; }
		void OnSceneLoadFailed() override
		{
			++m_state->failedLoads;
			m_state->request.clear();
			if (m_state->failNotification) throw std::runtime_error("notification failure");
		}
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

	struct DiagnosticCapture
	{
		std::vector<std::string> messages;
		DiagnosticCapture()
		{
			Diagnostics::SetSink([this](std::string_view message) { messages.emplace_back(message); });
		}
		~DiagnosticCapture() { Diagnostics::SetSink({}); }
	};

	struct PreparationGate
	{
		std::promise<void> signal;
		~PreparationGate() { Open(); }
		void Open()
		{
			if (opened) return;
			signal.set_value();
			opened = true;
		}
	private:
		bool opened{};
	};

	struct CopyFailingFactory
	{
		std::shared_ptr<bool> failCopy;
		explicit CopyFailingFactory(std::shared_ptr<bool> failure) : failCopy(std::move(failure)) {}
		CopyFailingFactory(const CopyFailingFactory& other) : failCopy(other.failCopy)
		{
			if (*failCopy) throw std::runtime_error("factory copy failure");
		}
		std::unique_ptr<IScene> operator()() const { return std::make_unique<TestScene>(std::make_shared<State>()); }
	};

	void ResizePrecedesActivationOnMainThread()
	{
		Fixture fixture;
		auto initial = std::make_shared<State>();
		fixture.Register("initial", initial);
		Require(fixture.manager.LoadScene("initial"), "The initial scene loads");
		Require(initial->lifecycle == std::vector<std::string>{ "prepare", "resize", "activate" },
			"Initial resize runs after CPU preparation and before resource activation");
		Require(initial->resizeThread == std::this_thread::get_id() && initial->activateThread == std::this_thread::get_id(),
			"Initial viewport notification and activation run on the main thread");
		Require(initial->activationWidth == 640 && initial->activationHeight == 480 && initial->resized == 1,
			"The first Activate observes the initialized client size exactly once");

		fixture.manager.Resize(1200, 700);
		auto replacement = std::make_shared<State>();
		fixture.Register("replacement", replacement);
		Require(fixture.manager.LoadScene("replacement"), "A replacement loads after the window resizes");
		Require(replacement->activationWidth == 1200 && replacement->activationHeight == 700 && replacement->resized == 1,
			"A newly created scene receives the current size before Activate");
	}

	void ResizeNotifiesAllActiveScenesAndIgnoresInvalidOrUnchangedSizes()
	{
		Fixture fixture;
		auto first = std::make_shared<State>();
		auto second = std::make_shared<State>();
		fixture.Register("first", first);
		fixture.Register("second", second);
		Require(fixture.manager.LoadScene("first") &&
			fixture.manager.LoadScene("second", SceneLoadType::Synchronous, SceneLoadMode::Additive), "Both active scenes load");
		fixture.manager.Resize(640, 480);
		fixture.manager.Resize(0, 480);
		fixture.manager.Resize(640, 0);
		fixture.manager.Resize(0, 0);
		Require(first->resized == 1 && second->resized == 1 && first->width == 640 && first->height == 480,
			"Unchanged and zero dimensions preserve the last usable scene viewport without notification");
		fixture.manager.Resize(960, 540);
		Require(first->resized == 2 && second->resized == 2 && first->width == 960 && first->height == 540 &&
			second->width == 960 && second->height == 540,
			"Every active additive scene receives the changed dimensions");
		Require(first->resizeThread == std::this_thread::get_id() && second->resizeThread == std::this_thread::get_id(),
			"All active viewport notifications run on the calling main thread");
		fixture.manager.Resize(0, 0);
		fixture.manager.Resize(960, 540);
		Require(first->resized == 2 && second->resized == 2,
			"Minimization followed by the same client size needs no repeated resource update");
	}

	void AsyncResizeWaitsForPreparationAndUsesLatestSize()
	{
		Fixture fixture;
		auto old = std::make_shared<State>();
		auto next = std::make_shared<State>();
		// Destroyed before the fixture, so any throwing assertion or frame update
		// releases Prepare before SceneManager joins its worker during destruction.
		PreparationGate gate;
		next->allowPrepare = gate.signal.get_future().share();
		auto entered = next->prepareEntered.get_future();
		fixture.Register("old", old);
		fixture.Register("next", next);
		Require(fixture.manager.LoadScene("old"), "The current scene loads");
		Require(fixture.manager.LoadScene("next", SceneLoadType::Asynchronous), "An asynchronous transition starts");
		fixture.manager.Resize(800, 600);
		Require(old->resized == 2 && next->resized == 0, "Fade-out resizes only the existing scene before worker preparation");
		fixture.manager.UpdateFrame(0.5f, fixture.input);
		const bool workerStarted = entered.wait_for(std::chrono::seconds(5)) == std::future_status::ready;
		fixture.manager.Resize(1024, 720);
		fixture.manager.Resize(1200, 700);
		fixture.manager.UpdateFrame(1.0f, fixture.input);
		const bool candidateUntouched = next->resized == 0 && next->activated == 0;
		const bool oldResized = old->resized == 4 && old->width == 1200 && old->height == 700 && old->unloaded == 0;
		gate.Open();
		Require(workerStarted && candidateUntouched && oldResized,
			"Resizing during CPU preparation updates retained scenes without touching the candidate or activating early");
		const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
		while (next->activated == 0 && fixture.manager.IsLoading() && std::chrono::steady_clock::now() < deadline)
		{
			fixture.manager.UpdateFrame(0.25f, fixture.input);
			std::this_thread::yield();
		}
		Require(next->activated == 1 && fixture.manager.IsLoading(), "Successful preparation enters fade-in");
		Require(next->lifecycle == std::vector<std::string>{ "prepare", "resize", "activate" } && next->resized == 1,
			"The candidate receives one size notification after Prepare and before Activate");
		Require(next->prepareThread != std::this_thread::get_id() && next->resizeThread == std::this_thread::get_id() &&
			next->activateThread == std::this_thread::get_id(), "Only CPU preparation runs on the worker");
		Require(next->activationWidth == 1200 && next->activationHeight == 700,
			"Activation uses the latest resize during preparation instead of the originally queued dimensions");
		fixture.manager.Resize(900, 600);
		Require(next->resized == 2 && next->width == 900 && next->height == 600 && old->resized == 4,
			"Fade-in resizes the newly active scene without touching the retired scene");
		fixture.FinishTransition();
	}

	void CandidateResizeFailureKeepsOldSceneAndCanRetry()
	{
		for (const auto type : { SceneLoadType::Synchronous, SceneLoadType::Asynchronous })
		{
			Fixture fixture;
			auto old = std::make_shared<State>();
			auto failed = std::make_shared<State>();
			failed->failResize = true;
			fixture.Register("old", old);
			fixture.Register("next", failed);
			Require(fixture.manager.LoadScene("old"), "The current scene loads");
			fixture.manager.Resize(1000, 700);
			DiagnosticCapture diagnostics;
			Require(fixture.manager.LoadScene("next", type) == (type == SceneLoadType::Asynchronous),
				"A synchronous resize failure rejects the load; asynchronous failure follows an accepted request");
			fixture.FinishTransition();
			const auto& error = fixture.manager.GetLastLoadError();
			Require(error.find("next") != std::string::npos && error.find("] resize:") != std::string::npos &&
				error.find("resize failure") != std::string::npos, "The load error identifies scene, resize phase and cause");
			Require(diagnostics.messages.size() == 1 && diagnostics.messages.front().find(error) != std::string::npos,
				"A failed candidate viewport notification is diagnosed exactly once");
			Require(failed->prepared == 1 && failed->resized == 1 && failed->activated == 0 && failed->destroyed == 1,
				"A resize failure destroys the prepared candidate before activation");
			Require(old->failedLoads == 1 && old->unloaded == 0 && old->destroyed == 0 && fixture.lifetime.pending.empty(),
				"Resize failure retains the current scene and notifies it without retiring its GPU resources");
			fixture.manager.Update(0.016f, fixture.input);
			Require(old->updates == 1 && old->width == 1000 && old->height == 700,
				"The retained scene resumes with the updated viewport after failure");
			fixture.manager.Resize(1100, 800);
			auto retry = std::make_shared<State>();
			fixture.Register("next", retry);
			Require(fixture.manager.LoadScene("next", type), "A failed resize permits retry with a new candidate");
			fixture.FinishTransition();
			Require(retry->activationWidth == 1100 && retry->activationHeight == 800 && retry->activated == 1 &&
				fixture.manager.GetLastLoadError().empty() && diagnostics.messages.size() == 1,
				"A successful retry activates at the latest size and clears the old error without repeating its diagnostic");
		}
	}

	void ActiveResizeFailurePropagatesToCaller()
	{
		Fixture fixture;
		auto active = std::make_shared<State>();
		fixture.Register("active", active);
		Require(fixture.manager.LoadScene("active"), "The active scene loads before resize failure");
		active->failResize = true;
		bool originalFailureCaught = false;
		try { fixture.manager.Resize(1000, 700); }
		catch (const std::runtime_error& error) { originalFailureCaught = std::string_view(error.what()) == "resize failure"; }
		Require(originalFailureCaught, "An active-scene resize failure reaches the caller to stop rendering");
		Require(active->failedLoads == 0 && fixture.manager.GetLastLoadError().empty() && fixture.lifetime.pending.empty(),
			"An active resize failure is not mistaken for a recoverable scene replacement failure");
	}

	void AsyncFactoryCopyFailureRecovers()
	{
		for (const bool failAtStart : { false, true })
		{
			Fixture fixture;
			auto old = std::make_shared<State>();
			fixture.Register("old", old);
			Require(fixture.manager.LoadScene("old"), "Initial scene loads");
			auto failCopy = std::make_shared<bool>(false);
			fixture.manager.RegisterScene("copy_failure", CopyFailingFactory(failCopy));
			if (failAtStart)
			{
				Require(fixture.manager.LoadScene("copy_failure", SceneLoadType::Asynchronous), "Request queues before copy fails");
				*failCopy = true;
				fixture.manager.UpdateFrame(0.5f, fixture.input);
			}
			else
			{
				*failCopy = true;
				Require(!fixture.manager.LoadScene("copy_failure", SceneLoadType::Asynchronous), "Request copy failure returns false");
			}
			Require(!fixture.manager.IsLoading() && old->failedLoads == 1, "Copy failure resets transition and notifies once");
			Require(fixture.manager.GetLastLoadError().find("factory copy failure") != std::string::npos,
				"Copy failure preserves original cause");
			fixture.manager.Update(0.016f, fixture.input);
			Require(old->updates == 1 && old->unloaded == 0 && fixture.lifetime.pending.empty(), "Copy failure retains active scene");
			*failCopy = false;
			Require(fixture.manager.LoadScene("copy_failure", SceneLoadType::Asynchronous), "Failed request can be retried");
			fixture.FinishTransition();
			Require(fixture.manager.GetLastLoadError().empty(), "Successful retry clears previous failure");
		}
	}

	void LoadFailuresIncludeContextAndReportOnce()
	{
		for (const auto type : { SceneLoadType::Synchronous, SceneLoadType::Asynchronous })
		{
			for (const bool failActivate : { false, true })
			{
				Fixture fixture;
				auto next = std::make_shared<State>();
				next->failPrepare = !failActivate;
				next->failActivate = failActivate;
				fixture.Register("broken_scene", next);
				DiagnosticCapture diagnostics;
				const bool accepted = fixture.manager.LoadScene("broken_scene", type);
				Require(accepted == (type == SceneLoadType::Asynchronous), "Sync failure rejects; async failure follows accepted request");
				fixture.FinishTransition();
				const auto& error = fixture.manager.GetLastLoadError();
				Require(error.find("broken_scene") != std::string::npos &&
					error.find(failActivate ? "activation" : "preparation") != std::string::npos &&
					error.find(failActivate ? "activate failure" : "prepare failure") != std::string::npos,
					"Failure identifies scene, operation and original cause");
				Require(diagnostics.messages.size() == 1 && diagnostics.messages.front().find(error) != std::string::npos,
					"Recovery reports each load failure exactly once");
			}
		}
	}

	void FailingNotificationDoesNotReplaceLoadFailure()
	{
		for (const auto type : { SceneLoadType::Synchronous, SceneLoadType::Asynchronous })
		{
			Fixture fixture;
			auto first = std::make_shared<State>();
			auto second = std::make_shared<State>();
			first->failNotification = true;
			fixture.Register("first", first);
			fixture.Register("second", second);
			Require(fixture.manager.LoadScene("first") &&
				fixture.manager.LoadScene("second", SceneLoadType::Synchronous, SceneLoadMode::Additive), "Both active scenes load");
			fixture.manager.RegisterScene("bad", []() -> std::unique_ptr<IScene> { throw std::runtime_error("original failure"); });
			DiagnosticCapture diagnostics;
			fixture.manager.LoadScene("bad", type);
			fixture.FinishTransition();
			Require(first->failedLoads == 1 && second->failedLoads == 1, "Every active scene is notified despite a throwing observer");
			Require(fixture.manager.GetLastLoadError().find("original failure") != std::string::npos,
				"Observer error does not replace original load error");
			Require(diagnostics.messages.size() == 2 && diagnostics.messages.back().find("notification failure") != std::string::npos,
				"Observer failure is separately diagnosed");
			fixture.manager.Update(0.016f, fixture.input);
			Require(first->updates == 1 && second->updates == 1, "All retained scenes resume after observer failure");
		}
	}

	void InitialLoadFailuresLeaveNoActiveSceneAndCanRetry()
	{
		const char* causes[] = { "not registered", "factory failure", "returned null", "prepare failure", "activate failure" };
		for (int failure = 0; failure < 5; ++failure)
		{
			Fixture fixture;
			DiagnosticCapture diagnostics;
			auto failed = std::make_shared<State>();
			failed->failPrepare = failure == 3;
			failed->failActivate = failure == 4;
			if (failure == 1) fixture.manager.RegisterScene("initial", []() -> std::unique_ptr<IScene> { throw std::runtime_error("factory failure"); });
			else if (failure == 2) fixture.manager.RegisterScene("initial", []() -> std::unique_ptr<IScene> { return nullptr; });
			else if (failure >= 3) fixture.Register("initial", failed);

			Require(!fixture.manager.LoadScene("initial", SceneLoadType::Synchronous, SceneLoadMode::Single),
				"An initial load failure must return false even when there is no old scene");
			const auto& error = fixture.manager.GetLastLoadError();
			Require(error.find("initial") != std::string::npos && error.find(causes[failure]) != std::string::npos,
				"The startup caller must receive the failed scene name and original cause");
			if (failure > 0) Require(error.find(failure == 4 ? "activation" : "preparation") != std::string::npos,
				"The startup error must identify the failed loading phase");
			Require(diagnostics.messages.size() == 1 && diagnostics.messages.front().find(error) != std::string::npos,
				"The load failure must already be diagnosed once for the startup caller");
			Require(!fixture.manager.IsLoading() && fixture.lifetime.pending.empty(),
				"A failed initial load must leave no pending transition or retired scene");
			if (failure >= 3) Require(failed->destroyed == 1, "A failed initial candidate must be destroyed");
			fixture.manager.Update(0.016f, fixture.input);
			fixture.manager.UpdateFrame(0.016f, fixture.input);
			Require(failed->updates == 0 && failed->frames == 0 && failed->failedLoads == 0,
				"A failed initial candidate must never become an active scene");

			auto recovered = std::make_shared<State>();
			fixture.Register("initial", recovered);
			Require(fixture.manager.LoadScene("initial") && fixture.manager.GetLastLoadError().empty(),
				"SceneManager must still allow a successful retry and clear the old error");
			fixture.manager.Update(0.016f, fixture.input);
			fixture.manager.UpdateFrame(0.016f, fixture.input);
			Require(recovered->activated == 1 && recovered->updates == 1 && recovered->frames == 1 && diagnostics.messages.size() == 1,
				"Only the successful retry must run, without repeating the failure diagnostic");
		}
	}

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

	void DeferredUnloadFailureDoesNotSkipLaterCleanup()
	{
		Fixture fixture;
		DiagnosticCapture diagnostics;
		auto failing = std::make_shared<State>();
		auto later = std::make_shared<State>();
		auto replacement = std::make_shared<State>();
		failing->failUnload = true;
		fixture.Register("failing", failing);
		fixture.Register("later", later);
		fixture.Register("replacement", replacement);
		Require(fixture.manager.LoadScene("failing") &&
			fixture.manager.LoadScene("later", SceneLoadType::Synchronous, SceneLoadMode::Additive), "Both retiring scenes load");
		Require(fixture.manager.LoadScene("replacement"), "A successful replacement retires both scenes");
		Require(failing->unloaded == 0 && later->unloaded == 0 && fixture.lifetime.pending.size() == 2,
			"Even a failing Unload waits until the existing GPU retirement boundary");
		int laterReleaseCount = 0;
		fixture.lifetime.DeferRelease([&laterReleaseCount]() { ++laterReleaseCount; });
		fixture.lifetime.CompleteGpuWork();
		Require(failing->unloaded == 1 && failing->destroyed == 1 && later->unloaded == 1 && later->destroyed == 1,
			"A failing retired Unload must not skip another scene or repeat cleanup");
		Require(laterReleaseCount == 1 && fixture.lifetime.pending.empty(),
			"Deferred release continues through callbacks following a failed scene Unload");
		Require(diagnostics.messages.size() == 1 && diagnostics.messages.front().find("unload failure") != std::string::npos,
			"The failed deferred Unload reports its original cause once");
		fixture.lifetime.CompleteGpuWork();
		Require(failing->unloaded == 1 && later->unloaded == 1 && laterReleaseCount == 1,
			"Completing GPU work again must not repeat scene cleanup or callbacks");
	}

	void ActiveUnloadFailurePreservesOriginalException()
	{
		for (const bool failSink : { false, true })
		{
			DiagnosticCapture diagnostics;
			if (failSink)
			{
				Diagnostics::SetSink([&diagnostics](std::string_view message)
				{
					diagnostics.messages.emplace_back(message);
					throw std::runtime_error("diagnostic sink failure");
				});
			}
			auto failing = std::make_shared<State>();
			auto later = std::make_shared<State>();
			failing->failUnload = true;
			bool originalExceptionCaught = false;
			try
			{
				Fixture fixture;
				fixture.Register("failing", failing);
				fixture.Register("later", later);
				Require(fixture.manager.LoadScene("failing") &&
					fixture.manager.LoadScene("later", SceneLoadType::Synchronous, SceneLoadMode::Additive), "Both active scenes load");
				throw std::runtime_error("original update failure");
			}
			catch (const std::runtime_error& error)
			{
				originalExceptionCaught = std::string_view(error.what()) == "original update failure";
			}
			Require(originalExceptionCaught, "Unload or diagnostic sink failures must not replace the exception that caused shutdown");
			Require(failing->unloaded == 1 && failing->destroyed == 1 && later->unloaded == 1 && later->destroyed == 1,
				"Destruction attempts Unload and destroys each active scene once despite an earlier failure");
			Require(diagnostics.messages.size() == 1 && diagnostics.messages.front().find("unload failure") != std::string::npos,
				"The failed active Unload reports its original cause once, including with a throwing sink");
		}
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

#define SCENELIFECYCLETESTS_CASES(TEST) \
	TEST(ResizePrecedesActivationOnMainThread, "scene resize precedes activation on the main thread", Cpu) \
	TEST(ResizeNotifiesAllActiveScenesAndIgnoresInvalidOrUnchangedSizes, "resize updates additive scenes and ignores zero or unchanged dimensions", Cpu) \
	TEST(AsyncResizeWaitsForPreparationAndUsesLatestSize, "async resize waits for CPU preparation and activates with the latest size", Cpu) \
	TEST(CandidateResizeFailureKeepsOldSceneAndCanRetry, "candidate resize failure preserves active scene and permits retry", Cpu) \
	TEST(ActiveResizeFailurePropagatesToCaller, "active resize failure propagates to stop mismatched rendering", Cpu) \
	TEST(InitialLoadFailuresLeaveNoActiveSceneAndCanRetry, "initial load failures preserve cause and permit retry", Cpu) \
	TEST(SynchronousFailureKeepsOldScene, "sync load failures preserve active scene", Cpu) \
	TEST(DeferredRetirementAndAdditiveLoad, "additive load and fence-deferred retirement", Cpu) \
	TEST(DeferredUnloadFailureDoesNotSkipLaterCleanup, "deferred unload failures preserve later cleanup", Cpu) \
	TEST(ActiveUnloadFailurePreservesOriginalException, "active unload and sink failures preserve the original exception", Cpu) \
	TEST(AsynchronousPrepareKeepsOldScene, "async Prepare retention, affinity and recovery", Cpu) \
	TEST(AsyncFactoryAndActivationFailures, "async factory and activation failure recovery", Cpu) \
	TEST(PresentationAndSimulationAreIndependent, "presentation and fixed simulation contract", Cpu) \
	TEST(UnregisteredSceneRequestKeepsActiveScene, "unregistered scene requests preserve active scene", Cpu) \
	TEST(AsyncFactoryCopyFailureRecovers, "async factory copy failures recover and retry", Cpu) \
	TEST(LoadFailuresIncludeContextAndReportOnce, "load failures carry context and report once", Cpu) \
	TEST(FailingNotificationDoesNotReplaceLoadFailure, "notification failures preserve load error and other observers", Cpu)

GAME_TEST_SUITE(SceneLifecycleTests, SCENELIFECYCLETESTS_CASES)
