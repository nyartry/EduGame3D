#pragma once

#include "Framework/Audio/AudioIds.h"

class IAudioService
{
public:
	virtual ~IAudioService() = default;

	virtual void PlayBgm(BgmId id) = 0;
	virtual void StopBgm() = 0;
	virtual void PlaySe(SeId id) = 0;
	virtual void SetMasterVolume(float volume) = 0;
	virtual void SetBgmVolume(float volume) = 0;
	virtual void SetSeVolume(float volume) = 0;
};
