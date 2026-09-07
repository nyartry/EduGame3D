#include "Framework/Animation/AnimationPlayback.h"

#include "Framework/Models/SkinnedModelData.h"

#include <cmath>
#include <limits>

namespace
{
	bool IsDurationValid(double duration)
	{
		return std::isfinite(duration) && duration > 0.0;
	}
}

double GetAnimationDurationSeconds(const AnimationClip& clip)
{
	if (!IsDurationValid(clip.ticksPerSecond) || !IsDurationValid(clip.durationTicks)) return 0.0;
	const double duration = clip.durationTicks / clip.ticksPerSecond;
	return IsDurationValid(duration) ? duration : 0.0;
}

void AnimationPlayback::Seek(double seconds, double durationSeconds)
{
	m_localTimeSeconds = IsDurationValid(durationSeconds) && std::isfinite(seconds) && seconds > 0.0
		? std::fmod(seconds, durationSeconds) : 0.0;
}

AnimationPlaybackInterval AnimationPlayback::Advance(double deltaSeconds, double durationSeconds)
{
	if (!IsDurationValid(durationSeconds))
	{
		m_localTimeSeconds = 0.0;
		return {};
	}
	Seek(m_localTimeSeconds, durationSeconds);
	AnimationPlaybackInterval interval{ durationSeconds, m_localTimeSeconds, m_localTimeSeconds };
	if (!std::isfinite(deltaSeconds) || deltaSeconds <= 0.0) return interval;

	// Split before adding so small updates retain precision even after many cycles.
	const double remainder = std::fmod(deltaSeconds, durationSeconds);
	// Derive the quotient from the same remainder: floor(delta / duration) can
	// round up while fmod returns nearly a full period (for example 1.0 / 0.1).
	const double loops = std::round((deltaSeconds - remainder) / durationSeconds);
	if (!std::isfinite(loops) || loops >= static_cast<double>((std::numeric_limits<std::uint64_t>::max)()))
		return interval;
	interval.completedLoops = static_cast<std::uint64_t>(loops);
	const double distanceToEnd = durationSeconds - m_localTimeSeconds;
	if (remainder >= distanceToEnd)
	{
		++interval.completedLoops;
		interval.toSeconds = remainder - distanceToEnd;
	}
	else
	{
		interval.toSeconds = m_localTimeSeconds + remainder;
		// Rounding at the boundary must still agree with the sampled pose.
		if (interval.toSeconds >= durationSeconds)
		{
			++interval.completedLoops;
			interval.toSeconds = 0.0;
		}
	}
	interval.advanced = interval.completedLoops != 0 || interval.toSeconds > interval.fromSeconds;
	m_localTimeSeconds = interval.toSeconds;
	return interval;
}
