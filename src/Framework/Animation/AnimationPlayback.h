#pragma once

#include <cstdint>

struct AnimationClip;

// A forward, looping interval. Pose, root motion and events use the same endpoints.
// Endpoints are local seconds in [0, duration); completedLoops counts every wrap.
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
	// Seek/clip changes never produce displacement or events. Invalid input seeks to zero.
	void Seek(double seconds, double durationSeconds);
	// Zero, reverse, non-finite or unrepresentable advances leave playback unchanged.
	AnimationPlaybackInterval Advance(double deltaSeconds, double durationSeconds);
	double GetLocalTimeSeconds() const { return m_localTimeSeconds; }

private:
	double m_localTimeSeconds{};
};
