#include "Framework/Animation/AnimationEvents.h"
#include "../tools/AnimationEventEditor/AnimationEventJson.h"

#include <cmath>
#include <filesystem>
#include <limits>
#include <stdexcept>

namespace
{
	void Require(bool condition, const char* message)
	{
		if (!condition) throw std::runtime_error(message);
	}

	void EventIntervals()
	{
		const std::vector<AnimationEvents::Event> events
		{
			{ 0.0, "walk", "Start", "zero" },
			{ 0.5, "walk", "Footstep", "middle" },
			{ 1.0, "walk", "End", "end" },
			{ 0.5, "other", "Footstep", "other clip" },
			{ 1.1, "walk", "Invalid", "outside clip" }
		};
		AnimationPlayback playback;
		auto fired = AnimationEvents::Collect(events, "walk", playback.Advance(0.5, 1.0));
		Require(fired.size() == 1 && fired[0].event.name == "middle", "Initial zero is excluded; upper endpoint fires");
		fired = AnimationEvents::Collect(events, "walk", playback.Advance(0.5, 1.0));
		Require(fired.size() == 2 && fired[0].event.name == "end" && fired[1].event.name == "zero", "Boundary fires end then zero without repeating midpoint");
		Require(AnimationEvents::Collect(events, "walk", playback.Advance(0.0, 1.0)).empty(), "Stationary update does not refire boundary");
		fired = AnimationEvents::Collect(events, "walk", playback.Advance(2.5, 1.0));
		Require(fired.size() == 7, "Two whole cycles and partial interval retain every event");
		for (std::size_t index = 1; index < fired.size(); ++index)
			Require(fired[index - 1].offsetSeconds <= fired[index].offsetSeconds, "Events are chronological");
		playback.Seek(0.8, 1.0);
		Require(AnimationEvents::Collect(events, "walk", playback.Advance(0.0, 1.0)).empty(), "Seek produces no traversal");
		fired = AnimationEvents::Collect(events, "walk", playback.Advance(1.4, 1.0));
		Require(fired.size() == 5, "Partial start and end include only crossed events");
		bool rejected = false;
		try { AnimationEvents::Collect(events, "walk", playback.Advance(10000.0, 1.0), 8); }
		catch (const std::length_error&) { rejected = true; }
		Require(rejected, "Occurrence overflow is explicit and bounded");
	}

	void JsonContract()
	{
		std::string error;
		const std::string prefix = R"({"schema":"open-campus-animation-events-v1","sourceFbx":"model.fbx","events":[)";
		const std::string event = R"({"time":0.5,"animation":"walk","type":"Footstep","name":"\u8db3\ud83d\udc63","cue":"step\nleft"})";
		const auto parsed = AnimationEvents::Parse(prefix + event + "]}", error);
		Require(parsed && error.empty() && parsed->events.size() == 1, "Shared v1 JSON loads");
		Require(parsed->events[0].name.size() == 7 && parsed->events[0].cue == "step\nleft", "Unicode surrogate and standard escapes decode");
		const auto roundTrip = AnimationEvents::Parse(AnimationEvents::Serialize(*parsed), error);
		Require(roundTrip && roundTrip->events[0].name == parsed->events[0].name, "Shared writer round-trips UTF-8");
		for (const std::string& invalid : std::vector<std::string>{
			R"({"events":[]})",
			R"({"schema":"future-version","events":[]})",
			prefix + event + "]} trailing",
			prefix + event + ",]}",
			prefix + R"({"time":-1,"animation":"walk","type":"Step"}]})",
			prefix + R"({"time":1e999,"animation":"walk","type":"Step"}]})",
			prefix + R"({"time":1.,"animation":"walk","type":"Step"}]})",
			prefix + R"({"time":01,"animation":"walk","type":"Step"}]})",
			prefix + R"({"time":+1,"animation":"walk","type":"Step"}]})",
			prefix + R"({"time":0,"time":1,"animation":"walk","type":"Step"}]})",
			prefix + R"({"time":0,"animation":"walk"}]})",
			prefix + R"({"time":0,"animation":"walk","type":"\udc00"}]})" })
		{
			Require(!AnimationEvents::Parse(invalid, error) && !error.empty(), "Invalid JSON/schema/event is rejected with an error");
		}
		AnimationEvents::FileData invalidData = *parsed;
		invalidData.events[0].time = std::numeric_limits<double>::quiet_NaN();
		bool rejected = false;
		try { AnimationEvents::Serialize(invalidData); }
		catch (const std::invalid_argument&) { rejected = true; }
		Require(rejected, "Writer rejects invalid runtime data");
	}

	void EditorAndRuntimeRoundTrip()
	{
		// Disposable artifact under the repository's ignored build output directory.
		const auto directory = std::filesystem::path(__FILE__).parent_path().parent_path() / "x64" / "AnimationEventRoundTrip";
		std::filesystem::create_directories(directory);
		const auto path = directory / std::filesystem::path(u8"足音.anim_events.json");
		const auto utf8 = path.u8string();
		const std::string utf8Path(utf8.begin(), utf8.end());
		AnimationEventEditorTool::AnimationEventFileData editor;
		editor.sourceFbx = "source.fbx";
		AnimationEventEditorTool::AnimationEvent event;
		event.time = 0.5f;
		AnimationEventEditorTool::CopyText(event.animation, "walk");
		AnimationEventEditorTool::CopyText(event.type, "Footstep");
		AnimationEventEditorTool::CopyText(event.cue, "left");
		editor.events.push_back(event);
		std::string error;
		Require(AnimationEventEditorTool::SaveAnimationEventFile(utf8Path, editor, error), "Editor uses common writer with Unicode file path");
		const auto runtime = AnimationEvents::Load(path, error);
		Require(runtime && runtime->events.size() == 1 && runtime->events[0].cue == "left", "Runtime reads editor output");
		const auto loaded = AnimationEventEditorTool::LoadAnimationEventFile(utf8Path, error);
		Require(loaded && AnimationEventEditorTool::AnimationEventListsEqual(editor.events, loaded->events), "Editor round trip retains fields");
		auto oversized = *runtime;
		oversized.events[0].cue = std::string(256, 'x');
		Require(AnimationEvents::Save(path, oversized, error), "Runtime fields are not editor buffers");
		Require(!AnimationEventEditorTool::LoadAnimationEventFile(utf8Path, error), "Editor reports oversized field instead of silently truncating");
		std::filesystem::remove(path);
	}
}

void TestAnimationEvents()
{
	EventIntervals();
	JsonContract();
	EditorAndRuntimeRoundTrip();
}
