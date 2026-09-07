#pragma once

#include "Framework/Scene/Input/Input.h"
#include <cstdint>
#include <limits>

// Frame input remains available to UI. Each queued edge is delivered to one
// simulation snapshot; every reader of that snapshot sees the same edges.
class SimulationInputBuffer final
{
public:
	void Capture(const Input& frame)
	{
		m_latest = frame;
		for (std::size_t i = 0; i < Input::KeyCount; ++i)
		{
			Queue(m_pressed[i], frame.m_pressedKeys[i]);
			Queue(m_released[i], frame.m_releasedKeys[i]);
		}
		Queue(m_mousePressed, frame.m_leftMousePressed);
		Queue(m_mouseReleased, frame.m_leftMouseReleased);
	}

	Input ConsumeStep()
	{
		Input snapshot = m_latest;
		for (std::size_t i = 0; i < Input::KeyCount; ++i)
		{
			snapshot.m_pressedKeys[i] = Consume(m_pressed[i]);
			snapshot.m_releasedKeys[i] = Consume(m_released[i]);
		}
		snapshot.m_leftMousePressed = Consume(m_mousePressed);
		snapshot.m_leftMouseReleased = Consume(m_mouseReleased);
		return snapshot;
	}

	void Reset() { *this = SimulationInputBuffer{}; }

private:
	static void Queue(std::uint32_t& count, bool edge)
	{
		if (edge && count != (std::numeric_limits<std::uint32_t>::max)()) ++count;
	}
	static bool Consume(std::uint32_t& count)
	{
		if (count == 0) return false;
		--count;
		return true;
	}
	Input m_latest;
	std::array<std::uint32_t, Input::KeyCount> m_pressed{};
	std::array<std::uint32_t, Input::KeyCount> m_released{};
	std::uint32_t m_mousePressed{};
	std::uint32_t m_mouseReleased{};
};
