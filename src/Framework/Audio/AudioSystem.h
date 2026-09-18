#pragma once

#include "Framework/Audio/IAudioService.h"

#include <memory>
#include <string_view>

class AudioSystem final : public IAudioService
{
public:
	AudioSystem();
	~AudioSystem();
	AudioSystem(AudioSystem&&) = delete;
	AudioSystem& operator=(AudioSystem&&) = delete;
	AudioSystem(const AudioSystem&) = delete;
	AudioSystem& operator=(const AudioSystem&) = delete;

	bool Initialize();
	void Update();

	void RegisterBgm(std::string_view id, std::string_view assetPath) override;
	void RegisterSe(std::string_view id, std::string_view assetPath) override;
	void PlayBgm(std::string_view id) override;
	void StopBgm() override;
	void PlaySe(std::string_view id) override;
	void SetMasterVolume(float volume) override;
	void SetBgmVolume(float volume) override;
	void SetSeVolume(float volume) override;

	bool IsAvailable() const;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
