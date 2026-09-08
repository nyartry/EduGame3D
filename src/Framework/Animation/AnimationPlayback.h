#pragma once

#include <cstdint>

struct AnimationClip;

enum class AnimationPlaybackMode
{
	Loop,
	Once
};

// Pose, root motion and events use the same forward interval. Loop endpoints are
// in [0, duration); Once may reach duration and never reports a completed loop.
struct AnimationPlaybackInterval
{
	double durationSeconds{};
	double fromSeconds{};
	double toSeconds{};
	std::uint64_t completedLoops{};
	bool advanced{};
};

double GetAnimationDurationSeconds(const AnimationClip& clip);

class AnimationPlayback
{
public:
	void Reset(AnimationPlaybackMode mode = AnimationPlaybackMode::Loop);
	// Seek/clip changes never produce displacement or events. Invalid input seeks to zero.
	// Once clamps to the endpoint and finishes there; invalid durations also finish.
	void Seek(double seconds, double durationSeconds);
	// Zero, reverse and non-finite advances leave playback unchanged. Once stops at
	// the endpoint; Loop also ignores advances with an unrepresentable loop count.
	AnimationPlaybackInterval Advance(double deltaSeconds, double durationSeconds);
	double GetLocalTimeSeconds() const { return m_localTimeSeconds; }
	AnimationPlaybackMode GetMode() const { return m_mode; }
	bool IsFinished() const { return m_finished; }

private:
	double m_localTimeSeconds{};
	AnimationPlaybackMode m_mode{ AnimationPlaybackMode::Loop };
	bool m_finished{};
};
