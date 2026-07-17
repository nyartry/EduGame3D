#pragma once

#include "Framework/Audio/IAudioService.h"

#include <Audio.h>

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

class AudioPlayerBase
{
public:
	virtual ~AudioPlayerBase() = default;
	virtual bool Load(DirectX::AudioEngine& engine) = 0;
	virtual void SetVolume(float volume) = 0;

protected:
	static std::wstring ToWidePath(const char* path);
	static std::wstring ResolveAssetPath(const char* path);
};

class BgmPlayer final : public AudioPlayerBase
{
public:
	void Register(std::string_view id, std::string_view path);
	bool Load(DirectX::AudioEngine& engine) override;
	void SetVolume(float volume) override;
	void Play(std::string_view id);
	void Stop();

private:
	struct BgmTrack
	{
		std::wstring path;
		std::unique_ptr<DirectX::SoundEffect> effect;
		std::unique_ptr<DirectX::SoundEffectInstance> instance;
	};

	std::unordered_map<std::string, BgmTrack> m_tracks;
	std::string m_currentId;
	bool m_hasCurrent{};
	float m_volume{ 0.45f };
};

class SePlayer final : public AudioPlayerBase
{
public:
	void Register(std::string_view id, std::string_view path);
	bool Load(DirectX::AudioEngine& engine) override;
	void SetVolume(float volume) override;
	void Play(std::string_view id);

private:
	struct SeClip
	{
		std::wstring path;
		std::unique_ptr<DirectX::SoundEffect> effect;
	};

	std::unordered_map<std::string, SeClip> m_clips;
	float m_volume{ 0.78f };
};

class AudioSystem final : public IAudioService
{
public:
	~AudioSystem();

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
	std::unique_ptr<DirectX::AudioEngine> m_engine;
	BgmPlayer m_bgmPlayer;
	SePlayer m_sePlayer;
	bool m_available{};
	bool m_comInitialized{};
};
