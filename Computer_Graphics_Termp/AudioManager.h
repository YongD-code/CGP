#pragma once
#include <fmod.hpp>
#include <string>
#include <unordered_map>

class AudioManager
{
public:
    // 접근할 싱글톤 인스턴스
    static AudioManager& Instance();

    bool Init();
    void Update();

    // BGM 같은 루프 사운드인 경우 true
    bool LoadSound(const std::string& name, const std::string& filepath, bool loop = false);
    void Play(const std::string& name, bool paused = false);
    void Stop(const std::string& name);
    void SetVolume(const std::string& name, float volume);
    bool IsPlaying(const std::string& name) const;
    void Release();

private:
    AudioManager() = default;
    ~AudioManager() = default;
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    FMOD::System* m_system = nullptr;
    std::unordered_map<std::string, FMOD::Sound*> m_sounds;
    std::unordered_map<std::string, FMOD::Channel*> m_channels;
};
