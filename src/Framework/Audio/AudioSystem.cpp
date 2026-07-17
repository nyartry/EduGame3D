#include "Framework/Audio/AudioSystem.h"
#include "Framework/Assets/AssetPathResolver.h"

#include <Audio.h>
#include <Windows.h>

#include <algorithm>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

using namespace DirectX;

namespace
{
	void DebugLogAudio(const char* message)
	{
		OutputDebugStringA("[Audio] ");
		OutputDebugStringA(message);
		OutputDebugStringA("\n");
	}

	void DebugLogAudio(const std::wstring& message)
	{
		OutputDebugStringW(L"[Audio] ");
		OutputDebugStringW(message.c_str());
		OutputDebugStringW(L"\n");
	}

	std::wstring ResolveAssetPath(std::string_view path)
	{
		return AssetPathResolver::Resolve(path).wstring();
	}

	class BgmPlayer
	{
	public:
		void Register(std::string_view id, std::string_view path)
		{
			m_tracks[std::string(id)].path = ResolveAssetPath(path);
		}

		void Load(AudioEngine& engine)
		{
			for (auto& [id, track] : m_tracks)
			{
				(void)id;
				DebugLogAudio(std::wstring(L"Loading BGM: ") + track.path);
				track.effect = std::make_unique<SoundEffect>(&engine, track.path.c_str());
				track.instance = track.effect->CreateInstance();
				track.instance->SetVolume(m_volume);
			}
		}

		void SetVolume(float volume)
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

		void Play(std::string_view id)
		{
			if (m_hasCurrent && m_currentId == id)
			{
				auto currentTrack = m_tracks.find(std::string(id));
				if (currentTrack != m_tracks.end() && currentTrack->second.instance != nullptr &&
					currentTrack->second.instance->GetState() != PLAYING)
				{
					currentTrack->second.instance->SetVolume(m_volume);
					currentTrack->second.instance->Play(true);
				}
				return;
			}

			Stop();
			auto track = m_tracks.find(std::string(id));
			if (track == m_tracks.end() || track->second.instance == nullptr)
			{
				return;
			}
			track->second.instance->SetVolume(m_volume);
			track->second.instance->Play(true);
			m_currentId = id;
			m_hasCurrent = true;
		}

		void Stop()
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

	private:
		struct Track
		{
			std::wstring path;
			std::unique_ptr<SoundEffect> effect;
			std::unique_ptr<SoundEffectInstance> instance;
		};
		std::unordered_map<std::string, Track> m_tracks;
		std::string m_currentId;
		bool m_hasCurrent{};
		float m_volume{ 0.45f };
	};

	class SePlayer
	{
	public:
		void Register(std::string_view id, std::string_view path)
		{
			m_clips[std::string(id)].path = ResolveAssetPath(path);
		}

		void Load(AudioEngine& engine)
		{
			for (auto& [id, clip] : m_clips)
			{
				(void)id;
				DebugLogAudio(std::wstring(L"Loading SE: ") + clip.path);
				clip.effect = std::make_unique<SoundEffect>(&engine, clip.path.c_str());
			}
		}

		void SetVolume(float volume) { m_volume = std::clamp(volume, 0.0f, 1.0f); }

		void Play(std::string_view id)
		{
			auto clip = m_clips.find(std::string(id));
			if (clip != m_clips.end() && clip->second.effect != nullptr)
			{
				clip->second.effect->Play(m_volume, 0.0f, 0.0f);
			}
		}

	private:
		struct Clip
		{
			std::wstring path;
			std::unique_ptr<SoundEffect> effect;
		};
		std::unordered_map<std::string, Clip> m_clips;
		float m_volume{ 0.78f };
	};
}

struct AudioSystem::Impl
{
	std::unique_ptr<AudioEngine> engine;
	BgmPlayer bgmPlayer;
	SePlayer sePlayer;
	bool available{};
	bool comInitialized{};
};

AudioSystem::AudioSystem()
	: m_impl(std::make_unique<Impl>())
{
}

AudioSystem::~AudioSystem()
{
	if (m_impl == nullptr)
	{
		return;
	}
	m_impl->bgmPlayer.Stop();
	m_impl->engine.reset();
	if (m_impl->comInitialized)
	{
		CoUninitialize();
	}
}

bool AudioSystem::Initialize()
{
	try
	{
		const HRESULT comResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		if (SUCCEEDED(comResult))
		{
			m_impl->comInitialized = true;
		}
		else if (comResult != RPC_E_CHANGED_MODE)
		{
			throw std::runtime_error("CoInitializeEx");
		}

		m_impl->engine = std::make_unique<AudioEngine>();
		m_impl->bgmPlayer.Load(*m_impl->engine);
		m_impl->sePlayer.Load(*m_impl->engine);
		m_impl->available = true;
		DebugLogAudio("AudioSystem initialized.");
		return true;
	}
	catch (const std::exception& ex)
	{
		DebugLogAudio("AudioSystem initialization failed.");
		DebugLogAudio(ex.what());
		m_impl->bgmPlayer.Stop();
		m_impl->engine.reset();
		m_impl->available = false;
		return false;
	}
}

void AudioSystem::Update()
{
	if (m_impl->engine != nullptr && !m_impl->engine->Update())
	{
		m_impl->engine->Reset();
	}
}

void AudioSystem::RegisterBgm(std::string_view id, std::string_view assetPath) { m_impl->bgmPlayer.Register(id, assetPath); }
void AudioSystem::RegisterSe(std::string_view id, std::string_view assetPath) { m_impl->sePlayer.Register(id, assetPath); }
void AudioSystem::PlayBgm(std::string_view id) { if (m_impl->available) m_impl->bgmPlayer.Play(id); }
void AudioSystem::StopBgm() { m_impl->bgmPlayer.Stop(); }
void AudioSystem::PlaySe(std::string_view id) { if (m_impl->available) m_impl->sePlayer.Play(id); }
void AudioSystem::SetMasterVolume(float volume) { if (m_impl->engine != nullptr) m_impl->engine->SetMasterVolume(std::clamp(volume, 0.0f, 1.0f)); }
void AudioSystem::SetBgmVolume(float volume) { m_impl->bgmPlayer.SetVolume(volume); }
void AudioSystem::SetSeVolume(float volume) { m_impl->sePlayer.SetVolume(volume); }
bool AudioSystem::IsAvailable() const { return m_impl->available; }
