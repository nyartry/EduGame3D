#include "Audio/AudioSystem.h"

#include <algorithm>
#include <exception>

using namespace DirectX;

namespace
{
	constexpr const char* TitleBgmPath = "Content\\Audio\\BGM\\title_theme.wav";
	constexpr const char* GameBgmPath = "Content\\Audio\\BGM\\game_theme.wav";
	constexpr const char* ButtonSePath = "Content\\Audio\\SE\\button_click.wav";
}

std::wstring AudioPlayerBase::ToWidePath(const char* path)
{
	std::wstring widePath;
	while (*path != '\0')
	{
		widePath.push_back(static_cast<wchar_t>(*path));
		++path;
	}
	return widePath;
}

void BgmPlayer::Register(BgmId id, const char* path)
{
	m_tracks[id].path = ToWidePath(path);
}

bool BgmPlayer::Load(AudioEngine& engine)
{
	for (auto& [id, track] : m_tracks)
	{
		(void)id;
		track.effect = std::make_unique<SoundEffect>(&engine, track.path.c_str());
		track.instance = track.effect->CreateInstance();
		track.instance->SetVolume(m_volume);
	}

	return true;
}

void BgmPlayer::SetVolume(float volume)
{
	m_volume = std::clamp(volume, 0.0f, 1.0f);
	for (auto& [id, track] : m_tracks)
	{
		(void)id;
		if (track.instance != nullptr)
		{
			track.instance->SetVolume(m_volume);
		}
	}
}

void BgmPlayer::Play(BgmId id)
{
	if (m_hasCurrent && m_currentId == id)
	{
		return;
	}

	Stop();

	auto track = m_tracks.find(id);
	if (track == m_tracks.end() || track->second.instance == nullptr)
	{
		return;
	}

	track->second.instance->SetVolume(m_volume);
	track->second.instance->Play(true);
	m_currentId = id;
	m_hasCurrent = true;
}

void BgmPlayer::Stop()
{
	if (!m_hasCurrent)
	{
		return;
	}

	auto track = m_tracks.find(m_currentId);
	if (track != m_tracks.end() && track->second.instance != nullptr)
	{
		track->second.instance->Stop();
	}
	m_hasCurrent = false;
}

void SePlayer::Register(SeId id, const char* path)
{
	m_clips[id].path = ToWidePath(path);
}

bool SePlayer::Load(AudioEngine& engine)
{
	for (auto& [id, clip] : m_clips)
	{
		(void)id;
		clip.effect = std::make_unique<SoundEffect>(&engine, clip.path.c_str());
	}

	return true;
}

void SePlayer::SetVolume(float volume)
{
	m_volume = std::clamp(volume, 0.0f, 1.0f);
}

void SePlayer::Play(SeId id)
{
	auto clip = m_clips.find(id);
	if (clip == m_clips.end() || clip->second.effect == nullptr)
	{
		return;
	}

	clip->second.effect->Play(m_volume, 0.0f, 0.0f);
}

bool AudioSystem::Initialize()
{
	try
	{
		m_engine = std::make_unique<AudioEngine>();
		m_bgmPlayer.Register(BgmId::Title, TitleBgmPath);
		m_bgmPlayer.Register(BgmId::Game, GameBgmPath);
		m_sePlayer.Register(SeId::Button, ButtonSePath);
		m_bgmPlayer.Load(*m_engine);
		m_sePlayer.Load(*m_engine);
		m_available = true;
		return true;
	}
	catch (const std::exception&)
	{
		m_bgmPlayer.Stop();
		m_engine.reset();
		m_available = false;
		return false;
	}
}

void AudioSystem::Update()
{
	if (m_engine == nullptr)
	{
		return;
	}

	if (!m_engine->Update())
	{
		m_engine->Reset();
	}
}

void AudioSystem::PlayBgm(BgmId id)
{
	if (m_available)
	{
		m_bgmPlayer.Play(id);
	}
}

void AudioSystem::StopBgm()
{
	m_bgmPlayer.Stop();
}

void AudioSystem::PlaySe(SeId id)
{
	if (m_available)
	{
		m_sePlayer.Play(id);
	}
}

void AudioSystem::SetMasterVolume(float volume)
{
	if (m_engine != nullptr)
	{
		m_engine->SetMasterVolume(std::clamp(volume, 0.0f, 1.0f));
	}
}

void AudioSystem::SetBgmVolume(float volume)
{
	m_bgmPlayer.SetVolume(volume);
}

void AudioSystem::SetSeVolume(float volume)
{
	m_sePlayer.SetVolume(volume);
}

bool AudioSystem::IsAvailable() const
{
	return m_available;
}
