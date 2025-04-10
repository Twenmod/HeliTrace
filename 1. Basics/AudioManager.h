#pragma once

namespace Tmpl8
{
	class Camera;
}

struct ma_engine;
struct ma_sound;


class AudioManager
{
public:
	void Tick();
	void SetListener(Camera& _listener) { m_listener = &_listener; }
	uint PlaySoundFile(const char* _soundFile, bool _looping = false, float _baseVolume = 1.f,
	                   bool _positional = false, float3 _position = float3(0), float falloff = 0.15f);
	void PlayMusic(const char* _soundFile, float _baseVolume = 1.f);
	void StopMusic();
	void StopAll();
	static AudioManager& GetAudioManager();
	ma_sound* GetSound(int _id);
	bool IsPlaying(const int _id);
	void SetSfxVol(const float _vol) { m_sfxVolume = _vol; }
	void SetMusicVol(const float _vol);
	float GetSfxVol() const { return m_sfxVolume; }
	float GetMusicVol() const { return m_musicVolume; }

protected:
	AudioManager();
	~AudioManager();
	static AudioManager* m_audioManager;
	ma_engine* m_engine;
	Camera* m_listener{nullptr};
	std::vector<std::pair<ma_sound*, int>> m_soundsPlaying;
	uint m_idCount{1};
	ma_sound* m_currentMusic{nullptr};
	float m_musicVolume = MUSIC_VOLUME;
	float m_sfxVolume = SFX_VOLUME;
};
