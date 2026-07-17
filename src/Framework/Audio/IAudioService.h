#pragma once

#include <string_view>

class IAudioService
{
public:
	virtual ~IAudioService() = default;

	virtual void RegisterBgm(std::string_view id, std::string_view assetPath) = 0;
	virtual void RegisterSe(std::string_view id, std::string_view assetPath) = 0;
	virtual void PlayBgm(std::string_view id) = 0;
	virtual void StopBgm() = 0;
	virtual void PlaySe(std::string_view id) = 0;
	virtual void SetMasterVolume(float volume) = 0;
	virtual void SetBgmVolume(float volume) = 0;
	virtual void SetSeVolume(float volume) = 0;
};
