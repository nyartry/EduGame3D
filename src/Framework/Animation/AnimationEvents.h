#pragma once

#include "Framework/Animation/AnimationPlayback.h"

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace AnimationEvents
{
	inline constexpr std::string_view Schema = "open-campus-animation-events-v1";

	// Runtime data has no editor buffer sizes, selection or undo state.
	struct Event
	{
		double time{};
		std::string animation;
		std::string type;
		std::string name;
		std::string bone;
		std::string cue;
	};

	struct FileData
	{
		std::string sourceFbx;
		std::vector<Event> events;
	};

	struct Occurrence
	{
		Event event;
		double offsetSeconds{}; // Time since the start of this update.
	};

	std::optional<FileData> Parse(std::string_view json, std::string& error);
	std::optional<FileData> Load(const std::filesystem::path& path, std::string& error);
	std::string Serialize(const FileData& data);
	bool Save(const std::filesystem::path& path, const FileData& data, std::string& error);

	// Forward playback emits (from, to], once per crossed cycle, in chronological
	// order. Time zero fires at wraps only; at a wrap duration events precede zero
	// events. Seek, clip switch and stationary updates emit nothing. Events outside
	// [0, duration], or belonging to another clip, are ignored.
	// Throws length_error before allocation if the explicit occurrence budget is exceeded.
	std::vector<Occurrence> Collect(const std::vector<Event>& events, std::string_view animation,
		const AnimationPlaybackInterval& interval, std::size_t maximumOccurrences = 4096);
}
