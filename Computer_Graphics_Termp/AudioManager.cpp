#include "AudioManager.h"
#include <fmod_errors.h>
#include <iostream>

AudioManager& AudioManager::Instance()
{
    static AudioManager inst;
    return inst;
}

// 음질이 자꾸 깨지네
bool AudioManager::Init() 
{
    // 1) 시스템 생성
    if (FMOD::System_Create(&m_system) != FMOD_OK)
        return false;

    // 믹서 샘플 레이트를 원본 MP3의 44.1 kHz에 고정
    m_system->setSoftwareFormat(
        44100,                      // 샘플 레이트 44.1 kHz
        FMOD_SPEAKERMODE_STEREO,    // 스테레오
        0                           // RAW 스피커 개수(일반 0)
    );

    // 고품질 스플라인 리샘플러 사용
    {
        FMOD_ADVANCEDSETTINGS adv{};
        m_system->getAdvancedSettings(&adv);
        adv.resamplerMethod = FMOD_DSP_RESAMPLER_SPLINE;
        m_system->setAdvancedSettings(&adv);
    }

    if (m_system->init(512, FMOD_INIT_NORMAL, nullptr) != FMOD_OK)
        return false;

    return true;
}

void AudioManager::Update() 
{
    if (m_system) m_system->update();
}

bool AudioManager::LoadSound(const std::string& name, const std::string& filepath, bool loop)
{
    if (!m_system)
        return false;

    FMOD::Sound* snd = nullptr;
    FMOD_MODE mode = FMOD_DEFAULT
        | FMOD_2D
        | FMOD_CREATESTREAM
        | (loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);

    FMOD_RESULT res = m_system->createStream(
        filepath.c_str(),
        mode,
        nullptr,
        &snd
    );

    if (res != FMOD_OK) {
        std::cerr << "AudioManager::LoadSound 실패: "
            << filepath << " ("
            << FMOD_ErrorString(res) << ")\n";
        return false;
    }

    m_sounds[name] = snd;
    return true;
}

void AudioManager::Play(const std::string& name, bool paused) 
{
    auto sit = m_sounds.find(name);
    if (sit == m_sounds.end()) return;

    // 이미 재생 중이라면 재생하지 않음
    auto cit = m_channels.find(name);
    if (cit != m_channels.end() && cit->second)
    {
        bool isPlaying = false;
        cit->second->isPlaying(&isPlaying);
        if (isPlaying)
        return;
    }

    // 새로 재생
    FMOD::Channel * ch = nullptr;
    m_system->playSound(sit->second, nullptr, paused, &ch);
    m_channels[name] = ch;
}

void AudioManager::Stop(const std::string& name) 
{
    auto it = m_channels.find(name);
    if (it != m_channels.end() && it->second)
        it->second->stop();
}

void AudioManager::SetVolume(const std::string& name, float volume) 
{
    auto it = m_channels.find(name);
    if (it != m_channels.end() && it->second)
        it->second->setVolume(volume);
}

bool AudioManager::IsPlaying(const std::string& name) const
{
    auto it = m_channels.find(name);
    if (it != m_channels.end() && it->second) {
        bool playing = false;
        it->second->isPlaying(&playing);
        return playing;
    }
    return false;
}

void AudioManager::Release() 
{
    for (auto& kv : m_channels) 
        if (kv.second) kv.second->stop();
    m_channels.clear();
    for (auto& kv : m_sounds)
        if (kv.second) kv.second->release();
    m_sounds.clear();
    if (m_system)
    {
        m_system->close();
        m_system->release();
        m_system = nullptr;
    }
}
