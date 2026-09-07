#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

// CPU allocation policy shared by upload buffers. A page belongs to one recorded
// frame, then remains immutable until that frame's submission fence completes.
// Callers own the backing resources and must retain them for this pool's lifetime.
class FenceRetiredPagePool
{
public:
	struct Allocation
	{
		std::size_t page{};
		std::size_t slot{};
	};

	explicit FenceRetiredPagePool(std::size_t slotsPerPage)
		: m_slotsPerPage(slotsPerPage)
	{
		if (slotsPerPage == 0)
		{
			throw std::invalid_argument("Upload pages need at least one slot.");
		}
	}

	void BeginFrame(std::uint64_t completedFence)
	{
		if (m_recording)
		{
			throw std::logic_error("The previous upload frame has not been submitted.");
		}
		for (Page& page : m_pages)
		{
			if (page.retireFence <= completedFence)
			{
				page.usedSlots = 0;
				page.retireFence = 0;
			}
		}
		m_completedFence = completedFence;
		m_nextPage = 0;
		m_recording = true;
	}

	Allocation Allocate(std::size_t slotCount = 1)
	{
		if (!m_recording)
		{
			throw std::logic_error("Upload allocation requires an active frame.");
		}
		if (slotCount == 0)
		{
			throw std::invalid_argument("An upload allocation must not be empty.");
		}
		while (m_nextPage < m_pages.size())
		{
			Page& page = m_pages[m_nextPage];
			if (page.retireFence == 0 && slotCount <= page.capacity - page.usedSlots)
			{
				const auto offset = page.usedSlots;
				page.usedSlots += slotCount;
				return { m_nextPage, offset };
			}
			++m_nextPage;
		}
		m_pages.push_back({ slotCount, 0, slotCount > m_slotsPerPage ? slotCount : m_slotsPerPage });
		return { m_nextPage, 0 };
	}

	void EndFrame(std::uint64_t submittedFence)
	{
		if (!m_recording || submittedFence <= m_lastSubmittedFence || submittedFence <= m_completedFence)
		{
			throw std::logic_error("Upload frames require a new, increasing submission fence.");
		}
		for (Page& page : m_pages)
		{
			if (page.retireFence == 0 && page.usedSlots != 0)
			{
				page.retireFence = submittedFence;
			}
		}
		m_lastSubmittedFence = submittedFence;
		m_recording = false;
	}

	std::size_t GetPageCount() const { return m_pages.size(); }
	std::size_t GetPageCapacity(std::size_t page) const { return m_pages.at(page).capacity; }

private:
	struct Page
	{
		std::size_t usedSlots{};
		std::uint64_t retireFence{};
		std::size_t capacity{};
	};

	const std::size_t m_slotsPerPage;
	std::vector<Page> m_pages;
	std::size_t m_nextPage{};
	std::uint64_t m_completedFence{};
	std::uint64_t m_lastSubmittedFence{};
	bool m_recording{};
};
