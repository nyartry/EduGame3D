#pragma once

#include <algorithm>
#include <cmath>
#include <stdexcept>

// Wall-clock collection belongs to the launcher. Long stalls are bounded and
// excess whole steps are discarded, keeping only the fractional remainder.
class FixedStepClock final
{
public:
	explicit FixedStepClock(double stepSeconds = 1.0 / 60.0, unsigned maxSteps = 8,
		double maxFrameSeconds = 0.25)
		: m_step(stepSeconds), m_maxSteps(maxSteps), m_maxFrame(maxFrameSeconds)
	{
		if (!std::isfinite(m_step) || m_step <= 0.0 || m_maxSteps == 0 ||
			!std::isfinite(m_maxFrame) || m_maxFrame < m_step)
		{
			throw std::invalid_argument("Invalid fixed-step clock settings");
		}
	}

	unsigned Advance(double elapsedSeconds)
	{
		if (!std::isfinite(elapsedSeconds) || elapsedSeconds <= 0.0) return 0;
		m_accumulator += std::min(elapsedSeconds, m_maxFrame);
		const double wholeSteps = std::floor((m_accumulator + m_step * 1.0e-9) / m_step);
		m_accumulator = std::max(0.0, m_accumulator - wholeSteps * m_step);
		return static_cast<unsigned>(std::min(wholeSteps, static_cast<double>(m_maxSteps)));
	}

	void Reset() { m_accumulator = 0.0; }
	float GetStepSeconds() const { return static_cast<float>(m_step); }
	double GetInterpolationAlpha() const { return m_accumulator / m_step; }

private:
	double m_step;
	unsigned m_maxSteps;
	double m_maxFrame;
	double m_accumulator{};
};
