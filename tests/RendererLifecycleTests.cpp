#include "TestSupport.h"
#include "Framework/Common/Common.h"
#include "Framework/Core/Diagnostics/Diagnostics.h"
#include "Framework/Rendering/Core/Dx12Renderer.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <future>
#include <memory>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

namespace
{
	void Require(bool condition, const char* message)
	{
		if (!condition) throw std::runtime_error(message);
	}

	template<class Action>
	void RequireLogicError(Action action, const char* message)
	{
		try { action(); }
		catch (const std::logic_error&) { return; }
		throw std::runtime_error(message);
	}

	class HiddenWindow
	{
	public:
		HiddenWindow()
		{
			m_window = CreateWindowExW(0, L"STATIC", L"Renderer lifecycle test", WS_OVERLAPPEDWINDOW,
				0, 0, 64, 64, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
			Require(m_window != nullptr, "Hidden test window is created");
		}
		~HiddenWindow() { DestroyWindow(m_window); }
		HWND Get() const { return m_window; }
		HiddenWindow(const HiddenWindow&) = delete;
		HiddenWindow& operator=(const HiddenWindow&) = delete;
	private:
		HWND m_window{};
	};

	ComPtr<IDXGIAdapter> WarpAdapter()
	{
		ComPtr<IDXGIFactory4> factory;
		ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
		ComPtr<IDXGIAdapter> adapter;
		ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));
		return adapter;
	}

	struct Fixture
	{
		TestSupport::RepositoryDirectory directory;
		HiddenWindow window;
		ComPtr<IDXGIAdapter> adapter{ WarpAdapter() };
		Dx12Renderer renderer;
		Fixture() { renderer.Initialize(window.Get(), 64, 64, adapter.Get()); }
	};

	class DiagnosticCapture
	{
	public:
		DiagnosticCapture()
		{
			Diagnostics::SetSink([this](std::string_view message)
			{
				messages.emplace_back(message);
				if (observer) observer(message);
			});
		}
		~DiagnosticCapture() { Diagnostics::SetSink({}); }
		std::vector<std::string> messages;
		std::function<void(std::string_view)> observer;
	};

	void UninitializedShutdownIsIdempotentAndTerminal()
	{
		static_assert(std::is_nothrow_destructible_v<Dx12Renderer>);
		static_assert(std::is_nothrow_destructible_v<RenderShutdownGuard>);
		static_assert(noexcept(std::declval<Dx12Renderer&>().Shutdown()));
		Dx12Renderer renderer;
		renderer.Shutdown();
		renderer.Shutdown();
		RequireLogicError([&] { renderer.BeginFrame(XMMatrixIdentity()); }, "Shutdown renderer rejects a new frame");
		RequireLogicError([&] { renderer.EndFrame(); }, "Shutdown renderer rejects frame submission");
		RequireLogicError([&] { renderer.Resize(64, 64); }, "Shutdown renderer rejects resize");
		RequireLogicError([&] { renderer.Initialize(nullptr, 64, 64); }, "Shutdown renderer rejects reinitialization");
	}

	void PartialInitializationCanShutdown()
	{
		auto adapter = WarpAdapter();
		Dx12Renderer renderer;
		bool failed = false;
		try { renderer.Initialize(nullptr, 64, 64, adapter.Get()); }
		catch (const std::exception&) { failed = true; }
		Require(failed, "Invalid HWND fails renderer initialization");
		Require(renderer.GetDevice() != nullptr && renderer.GetCommandQueue() != nullptr,
			"Failure occurs after device and queue creation, before complete initialization");
		renderer.Shutdown();
		renderer.Shutdown();
	}

	void SubmittedFrameCanShutdownTwice()
	{
		Fixture fixture;
		fixture.renderer.BeginFrame(XMMatrixIdentity());
		fixture.renderer.EndFrame();
		fixture.renderer.Shutdown();
		fixture.renderer.Shutdown();
		RequireLogicError([&] { fixture.renderer.BeginFrame(XMMatrixIdentity()); }, "Submitted renderer remains terminal after shutdown");
	}

	void ResizedFramesKeepValidTargets()
	{
		Fixture fixture;
		// Exercise both swap-chain buffers, landscape/portrait, repeated sizes,
		// and the zero-sized notification produced by minimizing a window.
		for (const auto& size : { std::pair<UINT, UINT>{ 96, 48 }, { 48, 96 }, { 48, 96 }, { 64, 64 } })
		{
			fixture.renderer.Resize(size.first, size.second);
			Require(fixture.renderer.GetWidth() == size.first && fixture.renderer.GetHeight() == size.second,
				"Renderer uses the latest positive dimensions");
			fixture.renderer.Resize(0, size.second);
			fixture.renderer.Resize(size.first, 0);
			Require(fixture.renderer.GetWidth() == size.first && fixture.renderer.GetHeight() == size.second,
				"Zero dimensions preserve the last valid render targets");
			for (UINT frame = 0; frame <= Dx12Renderer::FrameCount; ++frame)
			{
				fixture.renderer.BeginFrame(XMMatrixIdentity());
				RequireLogicError([&] { fixture.renderer.Resize(size.first + 1, size.second + 1); },
					"Changed dimensions cannot be applied during frame recording");
				fixture.renderer.EndFrame();
			}
			fixture.renderer.WaitForGpu();
			Require(SUCCEEDED(fixture.renderer.GetDevice()->GetDeviceRemovedReason()),
				"Resized color/depth targets remain valid for submitted frames");
		}
	}

	ComPtr<ID3D12Resource> Buffer(ID3D12Device* device, D3D12_HEAP_TYPE type, D3D12_RESOURCE_STATES state)
	{
		D3D12_HEAP_PROPERTIES heap{};
		heap.Type = type;
		D3D12_RESOURCE_DESC desc{};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Width = sizeof(std::uint32_t);
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.SampleDesc.Count = 1;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		ComPtr<ID3D12Resource> buffer;
		ThrowIfFailed(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc, state, nullptr, IID_PPV_ARGS(&buffer)));
		return buffer;
	}

	struct CopyResources
	{
		static constexpr std::uint32_t Expected = 0x1234ABCD;
		ComPtr<ID3D12Resource> source, destination;
		ComPtr<ID3D12CommandAllocator> allocator;
		ComPtr<ID3D12GraphicsCommandList> commands;
		ComPtr<ID3D12Fence> completed;
		explicit CopyResources(ID3D12Device* device)
		{
			source = Buffer(device, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
			destination = Buffer(device, D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_STATE_COPY_DEST);
			void* mapped{};
			const D3D12_RANGE noRead{ 0, 0 };
			ThrowIfFailed(source->Map(0, &noRead, &mapped));
			*static_cast<std::uint32_t*>(mapped) = Expected;
			source->Unmap(0, nullptr);
			ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)));
			ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr, IID_PPV_ARGS(&commands)));
			commands->CopyBufferRegion(destination.Get(), 0, source.Get(), 0, sizeof(Expected));
			ThrowIfFailed(commands->Close());
			ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&completed)));
		}
	};

	// The test controls a genuine GPU queue dependency. Destruction also releases
	// the gate, so a failed assertion cannot strand pending work during cleanup.
	class QueueGate
	{
	public:
		explicit QueueGate(ID3D12Device* device)
		{
			ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
			auto release = m_release.get_future();
			m_worker = std::thread([this, release = std::move(release)]() mutable
			{
				release.wait();
				std::this_thread::sleep_for(std::chrono::milliseconds(150));
				fence->Signal(1);
			});
		}
		~QueueGate()
		{
			fence->Signal(1);
			OpenSoon();
			m_worker.join();
		}
		void OpenSoon() noexcept
		{
			if (!m_requested)
			{
				m_requested = true;
				m_release.set_value();
			}
		}
		ComPtr<ID3D12Fence> fence;
	private:
		std::promise<void> m_release;
		std::thread m_worker;
		bool m_requested{};
	};

	void ShutdownGuardWaitsBeforeOwnersAndPreservesException()
	{
		Fixture fixture;
		QueueGate gate(fixture.renderer.GetDevice());
		// Keep safety references outside the observed owner so a regression is an
		// assertion failure, without releasing memory the GPU could still use.
		auto resources = std::make_shared<CopyResources>(fixture.renderer.GetDevice());
		int releases = 0;
		bool callbackAfterCompletion = false;
		bool ownerDestroyed = false;
		bool ownerAfterCompletion = false;
		bool originalException = false;
		struct Owner
		{
			std::shared_ptr<CopyResources> resources;
			bool& destroyed;
			bool& afterCompletion;
			~Owner()
			{
				destroyed = true;
				afterCompletion = resources->completed->GetCompletedValue() == 1;
			}
		};
		struct OpenGateOnExit
		{
			QueueGate& gate;
			~OpenGateOnExit() { gate.OpenSoon(); }
		};
		try
		{
			Owner owner{ resources, ownerDestroyed, ownerAfterCompletion };
			RenderShutdownGuard shutdown(fixture.renderer);
			OpenGateOnExit openGate{ gate };
			ThrowIfFailed(fixture.renderer.GetCommandQueue()->Wait(gate.fence.Get(), 1));
			ID3D12CommandList* commands[] = { resources->commands.Get() };
			fixture.renderer.GetCommandQueue()->ExecuteCommandLists(1, commands);
			ThrowIfFailed(fixture.renderer.GetCommandQueue()->Signal(resources->completed.Get(), 1));
			fixture.renderer.BeginFrame(XMMatrixIdentity());
			RequireLogicError([&] { fixture.renderer.WaitForGpu(); }, "Normal GPU flush still rejects an open frame");
			fixture.renderer.DeferRelease([&]
			{
				++releases;
				callbackAfterCompletion = resources->completed->GetCompletedValue() == 1 && !ownerDestroyed;
			});
			Require(releases == 0 && resources->completed->GetCompletedValue() == 0,
				"Recorded-frame callback and submitted copy remain pending before unwinding");
			throw std::runtime_error("original frame failure");
		}
		catch (const std::runtime_error& error) { originalException = std::string_view(error.what()) == "original frame failure"; }
		Require(originalException, "Shutdown preserves the original exception during stack unwinding");
		Require(releases == 1 && callbackAfterCompletion, "Deferred callback runs once after queued GPU work, while owners still live");
		Require(ownerDestroyed && ownerAfterCompletion, "Guard finishes GPU work before the resource owner is destroyed");
		void* mapped{};
		const D3D12_RANGE readRange{ 0, sizeof(std::uint32_t) };
		ThrowIfFailed(resources->destination->Map(0, &readRange, &mapped));
		const auto copied = *static_cast<const std::uint32_t*>(mapped);
		const D3D12_RANGE noWrite{ 0, 0 };
		resources->destination->Unmap(0, &noWrite);
		Require(copied == CopyResources::Expected, "Submitted copy completes despite the abandoned render frame");
		fixture.renderer.Shutdown();
		Require(releases == 1, "Repeated shutdown does not repeat a drained callback");
	}

	void DeferredFailuresAndReentrantCallbacksDrainOnce()
	{
		DiagnosticCapture diagnostics;
		int throwing = 0, outer = 0, nested = 0, last = 0;
		Fixture fixture;
		fixture.renderer.BeginFrame(XMMatrixIdentity());
		fixture.renderer.DeferRelease([&] { ++throwing; throw std::runtime_error("deferred cleanup failure"); });
		fixture.renderer.DeferRelease([&]
		{
			++outer;
			fixture.renderer.DeferRelease([&] { ++nested; });
		});
		fixture.renderer.DeferRelease([&] { ++last; });
		Require(throwing == 0 && outer == 0 && nested == 0 && last == 0, "Recorded-frame callbacks are deferred");
		fixture.renderer.Shutdown();
		fixture.renderer.Shutdown();
		Require(throwing == 1 && outer == 1 && nested == 1 && last == 1,
			"Throwing and reentrant callbacks do not skip or repeat any release");
		Require(!diagnostics.messages.empty(), "A secondary release failure is reported without escaping shutdown");
	}

	ComPtr<ID3D12Device5> RemovableDeviceOrSkip(ID3D12Device* device)
	{
		ComPtr<ID3D12Device5> removable;
		if (FAILED(device->QueryInterface(IID_PPV_ARGS(&removable))))
		{
			const char* message = "SKIP forced device removal: ID3D12Device5 is unavailable.";
			std::cout << message << '\n';
#ifdef GAME_NATIVE_TESTS
			Microsoft::VisualStudio::CppUnitTestFramework::Logger::WriteMessage(message);
#endif
		}
		return removable;
	}

	void ShutdownTimeoutRemovesDeviceBeforeRelease()
	{
		DiagnosticCapture diagnostics;
		int releases = 0, diagnosticReleases = 0;
		bool removedBeforeRelease = false, removedBeforeDiagnosticRelease = false, diagnosticQueuedRelease = false;
		Fixture fixture;
		auto removable = RemovableDeviceOrSkip(fixture.renderer.GetDevice());
		if (!removable) return;
		ComPtr<ID3D12Fence> gate;
		ThrowIfFailed(fixture.renderer.GetDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&gate)));
		RenderShutdownGuard shutdown(fixture.renderer);
		diagnostics.observer = [&](std::string_view)
		{
			if (diagnosticQueuedRelease) return;
			diagnosticQueuedRelease = true;
			fixture.renderer.DeferRelease([&]
			{
				++diagnosticReleases;
				removedBeforeDiagnosticRelease = FAILED(fixture.renderer.GetDevice()->GetDeviceRemovedReason());
			});
		};
		fixture.renderer.BeginFrame(XMMatrixIdentity());
		fixture.renderer.DeferRelease([&]
		{
			++releases;
			removedBeforeRelease = FAILED(fixture.renderer.GetDevice()->GetDeviceRemovedReason());
		});
		// No draw workload is submitted behind this deliberately unopened gate.
		// The renderer must time out and remove its private WARP device before
		// allowing resources to be destroyed.
		ThrowIfFailed(fixture.renderer.GetCommandQueue()->Wait(gate.Get(), 1));
		fixture.renderer.Shutdown();
		fixture.renderer.Shutdown();
		Require(releases == 1 && removedBeforeRelease, "GPU timeout confirms device removal before releasing resources once");
		Require(diagnosticReleases == 1 && removedBeforeDiagnosticRelease,
			"A release requested by the shutdown diagnostic sink also waits for confirmed device removal");
		Require(std::any_of(diagnostics.messages.begin(), diagnostics.messages.end(), [](const auto& message)
		{
			return message.find("timed out") != std::string::npos;
		}), "GPU shutdown timeout is diagnosed");
	}

	void RemovedDeviceCanShutdown()
	{
		DiagnosticCapture diagnostics;
		int releases = 0;
		Fixture fixture;
		auto removable = RemovableDeviceOrSkip(fixture.renderer.GetDevice());
		if (!removable) return;
		fixture.renderer.BeginFrame(XMMatrixIdentity());
		fixture.renderer.DeferRelease([&] { ++releases; });
		removable->RemoveDevice();
		Require(FAILED(fixture.renderer.GetDevice()->GetDeviceRemovedReason()), "Forced removal reports device loss");
		fixture.renderer.Shutdown();
		fixture.renderer.Shutdown();
		Require(releases == 1, "Removed-device shutdown drains deferred resources exactly once");
	}
}

#define RENDERERLIFECYCLETESTS_CASES(TEST) \
	TEST(UninitializedShutdownIsIdempotentAndTerminal, "uninitialized renderer shutdown is idempotent and terminal", Cpu) \
	TEST(PartialInitializationCanShutdown, "WARP partial renderer initialization can shut down", Gpu) \
	TEST(SubmittedFrameCanShutdownTwice, "WARP submitted frame can shut down twice", Gpu) \
	TEST(ResizedFramesKeepValidTargets, "WARP resized frames retain valid color and depth targets", Gpu) \
	TEST(ShutdownGuardWaitsBeforeOwnersAndPreservesException, "WARP shutdown waits before owners and preserves the original exception", Gpu) \
	TEST(DeferredFailuresAndReentrantCallbacksDrainOnce, "WARP shutdown contains release failures and drains reentrant callbacks", Gpu) \
	TEST(ShutdownTimeoutRemovesDeviceBeforeRelease, "WARP shutdown timeout removes the device before releasing resources", Gpu) \
	TEST(RemovedDeviceCanShutdown, "WARP removed device can shut down", Gpu)

GAME_TEST_SUITE(RendererLifecycleTests, RENDERERLIFECYCLETESTS_CASES)
