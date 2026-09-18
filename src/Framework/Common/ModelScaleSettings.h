#pragma once

struct ModelScaleSettings
{
	bool normalizeHeight{ false };
	float targetHeight{ 1.0f };

	static ModelScaleSettings OriginalSize()
	{
		return ModelScaleSettings{};
	}

	static ModelScaleSettings NormalizeToHeight(float height)
	{
		ModelScaleSettings settings;
		settings.normalizeHeight = true;
		settings.targetHeight = height;
		return settings;
	}
};
