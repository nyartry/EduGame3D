#pragma once

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

namespace AnimationEventEditorTool
{
	template <size_t Count>
	void CopyText(char (&destination)[Count], std::string_view text)
	{
		const size_t length = std::min(Count - 1, text.size());
		std::copy_n(text.data(), length, destination);
		destination[length] = '\0';
	}

	struct AnimationEvent
	{
		float time{};
		char animation[96]{};
		char type[48]{};
		char name[96]{};
		char bone[96]{};
		char cue[128]{};
	};

	struct AnimationEventFileData
	{
		std::string sourceFbx;
		std::vector<AnimationEvent> events;
	};

	struct EventHistoryState
	{
		std::vector<AnimationEvent> events;
		int selectedEvent{ -1 };
	};

	enum class CameraDragMode
	{
		None,
		Orbit,
		Pan,
		Dolly
	};

	inline bool AnimationEventEquals(const AnimationEvent& left, const AnimationEvent& right)
	{
		return left.time == right.time
			&& std::strcmp(left.animation, right.animation) == 0
			&& std::strcmp(left.type, right.type) == 0
			&& std::strcmp(left.name, right.name) == 0
			&& std::strcmp(left.bone, right.bone) == 0
			&& std::strcmp(left.cue, right.cue) == 0;
	}

	inline bool AnimationEventListsEqual(const std::vector<AnimationEvent>& left, const std::vector<AnimationEvent>& right)
	{
		if (left.size() != right.size())
		{
			return false;
		}

		for (size_t index = 0; index < left.size(); ++index)
		{
			if (!AnimationEventEquals(left[index], right[index]))
			{
				return false;
			}
		}

		return true;
	}
}
