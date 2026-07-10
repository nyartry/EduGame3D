#include "Framework/Audio/AudioSystem.h"

#include <algorithm>
#include <exception>
#include <filesystem>
#include <system_error>

#include <Windows.h>

using namespace DirectX;

namespace
{
	constexpr const char* TitleBgmPath = "Content\\Audio\\BGM\\title_theme.wav";
	constexpr const char* GameBgmPath = "Content\\Audio\\BGM\\game_theme.wav";
	constexpr const char* ButtonSePath = "Content\\Audio\\SE\\button_click.wav";

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

	std::filesystem::path GetExecutableDirectory()
	{
		wchar_t modulePath[MAX_PATH]{};
		const DWORD length = GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
		if (length == 0 || length >= MAX_PATH)
		{
			return {};
		}

		return std::filesystem::path(modulePath).parent_path();
	}

	bool TryResolvePath(const std::filesystem::path& candidate, std::wstring& resolvedPath)
	{
		std::error_code error;
		if (!std::filesystem::exists(candidate, error))
		{
			return false;
		}

		const std::filesystem::path absolutePath = std::filesystem::absolute(candidate, error);
		resolvedPath = error ? candidate.wstring() : absolutePath.wstring();
		return true;
	}
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

std::wstring AudioPlayerBase::ResolveAssetPath(const char* path)
{
	const std::filesystem::path relativePath{ ToWidePath(path) };
	std::wstring resolvedPath;
	if (relativePath.is_absolute() && TryResolvePath(relativePath, resolvedPath))
	{
		return resolvedPath;
	}

	std::error_code error;
	std::filesystem::path roots[] =
	{
		std::filesystem::current_path(error),
		GetExecutableDirectory()
	};

	for (const std::filesystem::path& root : roots)
	{
		if (root.empty())
		{
			continue;
		}

		std::filesystem::path searchRoot = root;
		for (int depth = 0; depth < 6 && !searchRoot.empty(); ++depth)
		{
			if (TryResolvePath(searchRoot / relativePath, resolvedPath))
			{
				return resolvedPath;
			}

			const std::filesystem::path parent = searchRoot.parent_path();
			if (parent == searchRoot)
			{
				break;
			}
			searchRoot = parent;
		}
	}

	DebugLogAudio(std::wstring(L"Asset not found: ") + relativePath.wstring());
	return relativePath.wstring();
}

void BgmPlayer::Register(BgmId id, const char* path)
{
	m_tracks[id].path = ResolveAssetPath(path);
}

bool BgmPlayer::Load(AudioEngine& engine)
{
	for (auto& [id, track] : m_tracks)
	{
		(void)id;
		DebugLogAudio(std::wstring(L"Loading BGM: ") + track.path);
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
		auto currentTrack = m_tracks.find(id);
		if (currentTrack != m_tracks.end() &&
			currentTrack->second.instance != nullptr &&
			currentTrack->second.instance->GetState() != PLAYING)
		{
			currentTrack->second.instance->SetVolume(m_volume);
			currentTrack->second.instance->Play(true);
		}
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
	m_clips[id].path = ResolveAssetPath(path);
}

bool SePlayer::Load(AudioEngine& engine)
{
	for (auto& [id, clip] : m_clips)
	{
		(void)id;
		DebugLogAudio(std::wstring(L"Loading SE: ") + clip.path);
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
		const HRESULT comResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		if (SUCCEEDED(comResult))
		{
			m_comInitialized = true;
			DebugLogAudio("COM initialized for audio.");
		}
		else if (comResult == RPC_E_CHANGED_MODE)
		{
			DebugLogAudio("COM was already initialized with another threading model.");
		}
		else
		{
			DebugLogAudio("COM initialization failed for audio.");
			throw std::runtime_error("CoInitializeEx");
		}

		m_engine = std::make_unique<AudioEngine>();
		m_bgmPlayer.Register(BgmId::Title, TitleBgmPath);
		m_bgmPlayer.Register(BgmId::Game, GameBgmPath);
		m_sePlayer.Register(SeId::Button, ButtonSePath);
		m_bgmPlayer.Load(*m_engine);
		m_sePlayer.Load(*m_engine);
		m_available = true;
		DebugLogAudio("AudioSystem initialized.");
		return true;
	}
	catch (const std::exception& ex)
	{
		DebugLogAudio("AudioSystem initialization failed.");
		DebugLogAudio(ex.what());
		m_bgmPlayer.Stop();
		m_engine.reset();
		m_available = false;
		return false;
	}
}

AudioSystem::~AudioSystem()
{
	m_bgmPlayer.Stop();
	m_engine.reset();
	if (m_comInitialized)
	{
		CoUninitialize();
		m_comInitialized = false;
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
