#include "Framework/Rendering/Core/FenceRetiredPagePool.h"
#include "TestSupport.h"

#include <algorithm>
#include <stdexcept>
#include <vector>

void RunRenderUploadGpuTests();

namespace
{
	void Require(bool condition, const char* message)
	{
		if (!condition) throw std::runtime_error(message);
	}

	template <typename Exception, typename Action>
	void RequireThrows(Action action, const char* message)
	{
		try { action(); }
		catch (const Exception&) { return; }
		throw std::runtime_error(message);
	}

	void CapacityAndFenceBoundaries()
	{
		FenceRetiredPagePool pool(1024);
		pool.BeginFrame(0);
		for (std::size_t index = 0; index < 1024; ++index)
		{
			const auto slot = pool.Allocate();
			Require(slot.page == 0 && slot.slot == index, "Capacity-exact allocations remain distinct.");
		}
		const auto overflow = pool.Allocate();
		Require(overflow.page == 1 && overflow.slot == 0, "The 1025th draw must grow instead of wrapping.");
		pool.EndFrame(10);

		pool.BeginFrame(9);
		const auto pending = pool.Allocate();
		Require(pending.page == 2, "No page can be reused before its fence completes.");
		pool.EndFrame(11);

		pool.BeginFrame(10);
		const auto recycled = pool.Allocate();
		Require(recycled.page == 0 && recycled.slot == 0, "Reuse starts exactly at fence completion.");
		pool.EndFrame(12);
		Require(pool.GetPageCount() == 3, "Completed pages are retained and reused.");
	}

	void MixedVertexSizesAndOutstandingFrames()
	{
		FenceRetiredPagePool pool(1024);
		struct LiveRange
		{
			FenceRetiredPagePool::Allocation allocation;
			std::size_t size;
			std::uint64_t fence;
		};
		std::vector<LiveRange> live;
		for (std::uint64_t frame = 1; frame <= 200; ++frame)
		{
			const auto completed = frame > 4 ? frame - 4 : 0;
			std::erase_if(live, [completed](const auto& range) { return range.fence <= completed; });
			pool.BeginFrame(completed);
			for (std::size_t draw = 0; draw < 13; ++draw)
			{
				const std::size_t size = static_cast<std::size_t>((frame * 17 + draw * 251) % 2048) + 1;
				const auto allocation = pool.Allocate(size);
				Require(allocation.slot + size <= pool.GetPageCapacity(allocation.page), "A vertex snapshot fits its page.");
				for (const auto& other : live)
				{
					Require(allocation.page != other.allocation.page ||
						allocation.slot + size <= other.allocation.slot ||
						other.allocation.slot + other.size <= allocation.slot,
						"Current and in-flight draw ranges must never overlap.");
				}
				live.push_back({ allocation, size, frame });
			}
			pool.EndFrame(frame);
		}
		pool.BeginFrame(200);
		const auto pagesBefore = pool.GetPageCount();
		pool.Allocate(1);
		Require(pool.GetPageCount() == pagesBefore, "Draining the queue permits reuse without growth.");
		pool.EndFrame(201);
	}

	void ContractValidation()
	{
		RequireThrows<std::invalid_argument>([] { FenceRetiredPagePool pool(0); }, "Empty pages are rejected.");
		FenceRetiredPagePool pool(2);
		RequireThrows<std::logic_error>([&] { pool.Allocate(); }, "Allocation outside a frame is rejected.");
		RequireThrows<std::logic_error>([&] { pool.EndFrame(1); }, "Submission without recording is rejected.");
		pool.BeginFrame(3);
		RequireThrows<std::logic_error>([&] { pool.BeginFrame(3); }, "Nested frames are rejected.");
		RequireThrows<std::invalid_argument>([&] { pool.Allocate(0); }, "Zero-size snapshots are rejected.");
		RequireThrows<std::logic_error>([&] { pool.EndFrame(3); }, "A fence already completed cannot protect new data.");
		pool.EndFrame(4);
		pool.BeginFrame(3);
		RequireThrows<std::logic_error>([&] { pool.EndFrame(4); }, "Fence values must increase even with no draws.");
		pool.EndFrame(5);
	}
}

#define RENDER_UPLOAD_TEST_CASES(TEST) \
	TEST(CapacityAndFenceBoundaries, "capacity and fence boundaries", Cpu) \
	TEST(MixedVertexSizesAndOutstandingFrames, "mixed vertex sizes and outstanding frames", Cpu) \
	TEST(ContractValidation, "render upload contract validation", Cpu) \
	TEST(RunRenderUploadGpuTests, "render upload WARP GPU regression", Gpu)

GAME_TEST_SUITE(RenderUploadTests, RENDER_UPLOAD_TEST_CASES)
