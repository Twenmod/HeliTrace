#include "precomp.h"
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h" // Before precomp to fix define warning
#include "AudioManager.h"


#include "camera.h"

AudioManager* AudioManager::m_audioManager = nullptr;

AudioManager& AudioManager::GetAudioManager()
{
	if (m_audioManager == nullptr) m_audioManager = new AudioManager();
	return *m_audioManager;
}

ma_sound* AudioManager::GetSound(const int _id)
{
	for (auto p : m_soundsPlaying)
	{
		if (p.second == _id)
		{
			return p.first;
		}
	}
	return nullptr;
}

bool AudioManager::IsPlaying(const int _id)
{
	ma_sound* sound = GetSound(_id);
	if (sound == nullptr) return false;
	else return ma_sound_is_playing(sound);
}

void AudioManager::SetMusicVol(const float _vol)
{
	m_musicVolume = _vol;
	ma_sound_set_volume(m_currentMusic, m_musicVolume);

}


AudioManager::AudioManager()
{
	ma_result result;

	m_engine = new ma_engine();
	ma_engine_config engineConfig;

	engineConfig = ma_engine_config_init();
	engineConfig.listenerCount = 1;

	result = ma_engine_init(&engineConfig, m_engine);
	if (result != MA_SUCCESS)
	{
		printf("ERROR::MINIAUDIO Audio Engine failed to start\n");
	}
	ma_engine_listener_set_enabled(m_engine, 0, true);
	ma_engine_set_volume(m_engine, BASE_VOLUME);
}

AudioManager::~AudioManager()
{
	for (int i = 0; i < m_soundsPlaying.size(); i++)
	{
		// Cleanup
		ma_sound_uninit(m_soundsPlaying[i].first);
		delete m_soundsPlaying[i].first;
	}
	if (m_currentMusic != nullptr)
	{
		ma_sound_uninit(m_currentMusic);
		delete m_currentMusic;
	}
	ma_engine_uninit(m_engine);
	delete m_engine;
}

void AudioManager::Tick()
{
	if (m_listener != nullptr)
	{
		ma_engine_listener_set_position(m_engine, 0, m_listener->camPos.x, m_listener->camPos.y, m_listener->camPos.z);
		ma_engine_listener_set_direction(m_engine, 0, -m_listener->front.x, -m_listener->front.y, -m_listener->front.z);
		ma_engine_listener_set_cone(m_engine, 0, 0.5f * PI, PI, 0.2f);
	}

	if (m_currentMusic != nullptr)
	{
		//if (!ma_sound_is_playing(m_currentMusic))
		//{
		//	ma_sound_start(m_currentMusic);
		//}
	}

	for (int i = 0; i < m_soundsPlaying.size(); i++)
	{
		if (!ma_sound_is_playing(m_soundsPlaying[i].first))
		{
			// Cleanup
			ma_sound_uninit(m_soundsPlaying[i].first);
			delete m_soundsPlaying[i].first;
			m_soundsPlaying.erase(m_soundsPlaying.begin() + i);
			i--;
		}
	}
}

uint AudioManager::PlaySoundFile(const char* _soundFile, const bool _looping, const float _baseVolume,
                                 const bool _positional,
                                 const float3 _position, const float _falloff)
{
	ma_sound* sound = new ma_sound;

	ma_result result = ma_sound_init_from_file(m_engine, _soundFile, MA_SOUND_FLAG_STREAM, NULL, NULL, sound);
	if (result != MA_SUCCESS)
	{
		printf("Failed to load sound: %s\n", _soundFile);
		delete sound;
		return 0;
	}
	ma_sound_set_looping(sound, _looping);
	float attenuation = _baseVolume;
	if (m_listener != nullptr && _positional)
	{
		ma_sound_set_positioning(sound, ma_positioning_absolute);
		ma_sound_set_position(sound, _position.x, _position.y, _position.z);
		ma_sound_set_attenuation_model(sound, ma_attenuation_model_inverse); // Basic distance attenuation
		ma_sound_set_min_distance(sound, 0.1f);
		ma_sound_set_max_distance(sound, 1000.f);
		ma_sound_set_min_gain(sound, -100);
		ma_sound_set_max_gain(sound, 100);
		ma_sound_set_rolloff(sound, _falloff);
		ma_sound_set_spatialization_enabled(sound, true);
	}
	else
	{
		ma_sound_set_attenuation_model(sound, ma_attenuation_model_none);
		ma_sound_set_spatialization_enabled(sound, false);
	}

	ma_sound_set_volume(sound, attenuation * m_sfxVolume);


	// Play the sound
	ma_sound_start(sound);
	m_soundsPlaying.push_back(std::make_pair<ma_sound*, int>(std::move(sound), m_idCount));
	return m_idCount++;
}

void AudioManager::PlayMusic(const char* _soundFile, const float /*_baseVolume*/)
{
	if (m_currentMusic != nullptr)
	{
		ma_sound_uninit(m_currentMusic);
		delete m_currentMusic;
	}

	ma_sound* sound = new ma_sound;

	ma_result result = ma_sound_init_from_file(m_engine, _soundFile, MA_SOUND_FLAG_STREAM, NULL, NULL, sound);
	if (result != MA_SUCCESS)
	{
		printf("Failed to load sound: %s\n", _soundFile);
		delete sound;
		return;
	}
	ma_sound_set_spatialization_enabled(sound, false);

	ma_sound_set_volume(sound, m_musicVolume * 0.2f);

	// Play the sound
	ma_sound_start(sound);
	m_currentMusic = sound;
}

void AudioManager::StopMusic()
{
	if (m_currentMusic != nullptr)
	{
		ma_sound_uninit(m_currentMusic);
		delete m_currentMusic;
		m_currentMusic = nullptr;
	}
}

void AudioManager::StopAll()
{
	for (int i = 0; i < m_soundsPlaying.size(); i++)
	{
		// Cleanup
		ma_sound_stop(m_soundsPlaying[i].first);
		ma_sound_uninit(m_soundsPlaying[i].first);
		delete m_soundsPlaying[i].first;
		m_soundsPlaying.erase(m_soundsPlaying.begin() + i);
		i--;
	}
}
