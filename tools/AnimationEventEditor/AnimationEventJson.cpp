#include "AnimationEventJson.h"

#include "Framework/Animation/AnimationEvents.h"

#include <cmath>
#include <filesystem>
#include <limits>

namespace AnimationEventEditorTool
{
	namespace
	{
		std::filesystem::path FromUtf8(std::string_view text)
		{
			return std::filesystem::path(std::u8string(text.begin(), text.end()));
		}
	}

	std::string MakeDefaultEventPath(const std::string& fbxPath)
	{
		auto path = FromUtf8(fbxPath);
		path.replace_extension(".anim_events.json");
		const auto utf8 = path.u8string();
		return std::string(utf8.begin(), utf8.end());
	}

	std::optional<AnimationEventFileData> LoadAnimationEventFile(const std::string& path, std::string& error)
	{
		const auto runtime = AnimationEvents::Load(FromUtf8(path), error);
		if (!runtime) return std::nullopt;
		AnimationEventFileData data;
		data.sourceFbx = runtime->sourceFbx;
		for (const auto& source : runtime->events)
		{
			AnimationEvent event;
			if (source.time > std::numeric_limits<float>::max() ||
				source.animation.size() >= sizeof(event.animation) || source.type.size() >= sizeof(event.type) ||
				source.name.size() >= sizeof(event.name) || source.bone.size() >= sizeof(event.bone) || source.cue.size() >= sizeof(event.cue))
			{
				error = "Event exceeds an editor field capacity; no text was truncated.";
				return std::nullopt;
			}
			event.time = static_cast<float>(source.time);
			CopyText(event.animation, source.animation);
			CopyText(event.type, source.type);
			CopyText(event.name, source.name);
			CopyText(event.bone, source.bone);
			CopyText(event.cue, source.cue);
			data.events.push_back(event);
		}
		return data;
	}

	bool SaveAnimationEventFile(const std::string& path, const AnimationEventFileData& data, std::string& error)
	{
		AnimationEvents::FileData runtime;
		runtime.sourceFbx = data.sourceFbx;
		for (const auto& event : data.events)
			runtime.events.push_back({ event.time, event.animation, event.type, event.name, event.bone, event.cue });
		return AnimationEvents::Save(FromUtf8(path), runtime, error);
	}
}
