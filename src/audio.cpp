#include "audio.h"

#include <algorithm>
#include <SDL3_mixer/SDL_mixer.h>

#include "logger.h"

namespace {
float VolumeToGain(int volume) {
    const int clamped = std::clamp(volume, 0, 128);
    return static_cast<float>(clamped) / 128.0f;
}

SDL_PropertiesID MakePlayOptions(int loops) {
    if (loops == 0) {
        return 0;
    }

    SDL_PropertiesID props = SDL_CreateProperties();
    if (!props) {
        return 0;
    }

    if (!SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER, loops)) {
        SDL_DestroyProperties(props);
        return 0;
    }

    return props;
}
}  // namespace

AudioEngine::AudioEngine() = default;
AudioEngine::~AudioEngine() { Shutdown(); }

bool AudioEngine::Init() {
    if (initialized_) {
        return true;
    }

    const bool audio_was_started_here = (SDL_WasInit(SDL_INIT_AUDIO) == 0);
    if (audio_was_started_here) {
        if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
            Logger::Log("AUDIO", Logger::Level::Error, "Failed to initialize SDL audio subsystem: %s", SDL_GetError());
            return false;
        }

        Logger::Log("AUDIO", Logger::Level::Info, "Initialized SDL audio subsystem.");
    }

    if (!MIX_Init()) {
        Logger::Log("AUDIO", Logger::Level::Error, "Failed to initialize SDL_mixer: %s", SDL_GetError());
        if (audio_was_started_here) {
            SDL_QuitSubSystem(SDL_INIT_AUDIO);
        }
        return false;
    }

    SDL_AudioSpec spec{};
    spec.format = SDL_AUDIO_F32;
    spec.channels = 2;
    spec.freq = 44100;

    mixer_ = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
    if (!mixer_) {
        Logger::Log("AUDIO", Logger::Level::Error, "Failed to create audio mixer: %s", SDL_GetError());
        MIX_Quit();
        if (audio_was_started_here) {
            SDL_QuitSubSystem(SDL_INIT_AUDIO);
        }
        return false;
    }

    MIX_SetMixerGain(mixer_, 1.0f);
    initialized_ = true;
    Logger::Log("AUDIO", Logger::Level::Info, "Audio engine initialized.");
    return true;
}

void AudioEngine::CleanupFinishedTracks() {
    auto it = active_sfx_tracks_.begin();
    while (it != active_sfx_tracks_.end()) {
        MIX_Track* track = *it;
        if (track && MIX_TrackPlaying(track)) {
            ++it;
            continue;
        }

        MIX_DestroyTrack(track);
        it = active_sfx_tracks_.erase(it);
    }

    if (music_track_ && !MIX_TrackPlaying(music_track_)) {
        MIX_DestroyTrack(music_track_);
        music_track_ = nullptr;
    }
}

void AudioEngine::Shutdown() {
    if (!initialized_) {
        return;
    }

    CleanupFinishedTracks();

    if (mixer_) {
        MIX_DestroyMixer(mixer_);
        mixer_ = nullptr;
    }

    music_track_ = nullptr;
    active_sfx_tracks_.clear();

    for (auto& p : sounds_) {
        if (p.second) {
            MIX_DestroyAudio(p.second);
        }
    }
    sounds_.clear();

    for (auto& p : musics_) {
        if (p.second) {
            MIX_DestroyAudio(p.second);
        }
    }
    musics_.clear();

    MIX_Quit();
    initialized_ = false;
    Logger::Log("AUDIO", Logger::Level::Info, "Audio engine shut down.");
}

bool AudioEngine::LoadSound(const std::string& id, const std::string& path) {
    if (!initialized_ && !Init()) {
        return false;
    }

    if (sounds_.count(id)) {
        return true;
    }

    MIX_Audio* audio = MIX_LoadAudio(mixer_, path.c_str(), true);
    if (!audio) {
        Logger::Log("AUDIO", Logger::Level::Warn, "Failed to load SFX '%s': %s", path.c_str(), SDL_GetError());
        return false;
    }

    sounds_[id] = audio;
    return true;
}

bool AudioEngine::LoadMusic(const std::string& id, const std::string& path) {
    if (!initialized_ && !Init()) {
        return false;
    }

    if (musics_.count(id)) {
        return true;
    }

    MIX_Audio* audio = MIX_LoadAudio(mixer_, path.c_str(), false);
    if (!audio) {
        Logger::Log("AUDIO", Logger::Level::Warn, "Failed to load music '%s': %s", path.c_str(), SDL_GetError());
        return false;
    }

    musics_[id] = audio;
    return true;
}

void AudioEngine::PlaySound(const std::string& id, int loops) {
    if (!initialized_ && !Init()) {
        return;
    }

    CleanupFinishedTracks();

    auto it = sounds_.find(id);
    if (it == sounds_.end() || !it->second) {
        return;
    }

    MIX_Track* track = MIX_CreateTrack(mixer_);
    if (!track) {
        Logger::Log("AUDIO", Logger::Level::Warn, "Failed to create SFX track: %s", SDL_GetError());
        return;
    }

    if (!MIX_SetTrackAudio(track, it->second)) {
        Logger::Log("AUDIO", Logger::Level::Warn, "Failed to bind SFX audio '%s': %s", id.c_str(), SDL_GetError());
        MIX_DestroyTrack(track);
        return;
    }

    if (!MIX_SetTrackGain(track, VolumeToGain(sfx_volume_))) {
        Logger::Log("AUDIO", Logger::Level::Warn, "Failed to set SFX gain: %s", SDL_GetError());
        MIX_DestroyTrack(track);
        return;
    }

    SDL_PropertiesID play_options = MakePlayOptions(loops);
    if (!MIX_PlayTrack(track, play_options)) {
        Logger::Log("AUDIO", Logger::Level::Warn, "Failed to play SFX '%s': %s", id.c_str(), SDL_GetError());
        if (play_options) {
            SDL_DestroyProperties(play_options);
        }
        MIX_DestroyTrack(track);
        return;
    }

    if (play_options) {
        SDL_DestroyProperties(play_options);
    }

    active_sfx_tracks_.push_back(track);
}

void AudioEngine::PlayMusic(const std::string& id, int loops) {
    if (!initialized_ && !Init()) {
        return;
    }

    CleanupFinishedTracks();

    auto it = musics_.find(id);
    if (it == musics_.end() || !it->second) {
        return;
    }

    if (music_track_) {
        MIX_DestroyTrack(music_track_);
        music_track_ = nullptr;
    }

    music_track_ = MIX_CreateTrack(mixer_);
    if (!music_track_) {
        Logger::Log("AUDIO", Logger::Level::Warn, "Failed to create music track: %s", SDL_GetError());
        return;
    }

    if (!MIX_SetTrackAudio(music_track_, it->second)) {
        Logger::Log("AUDIO", Logger::Level::Warn, "Failed to bind music audio '%s': %s", id.c_str(), SDL_GetError());
        MIX_DestroyTrack(music_track_);
        music_track_ = nullptr;
        return;
    }

    if (!MIX_SetTrackGain(music_track_, VolumeToGain(music_volume_))) {
        Logger::Log("AUDIO", Logger::Level::Warn, "Failed to set music gain: %s", SDL_GetError());
        MIX_DestroyTrack(music_track_);
        music_track_ = nullptr;
        return;
    }

    SDL_PropertiesID play_options = MakePlayOptions(loops);
    if (!MIX_PlayTrack(music_track_, play_options)) {
        Logger::Log("AUDIO", Logger::Level::Warn, "Failed to play music '%s': %s", id.c_str(), SDL_GetError());
        if (play_options) {
            SDL_DestroyProperties(play_options);
        }
        MIX_DestroyTrack(music_track_);
        music_track_ = nullptr;
        return;
    }

    if (play_options) {
        SDL_DestroyProperties(play_options);
    }
}

void AudioEngine::StopMusic() {
    if (!initialized_) {
        return;
    }

    if (music_track_) {
        MIX_DestroyTrack(music_track_);
        music_track_ = nullptr;
    }
}

void AudioEngine::SetSFXVolume(int volume) {
    sfx_volume_ = volume;
    CleanupFinishedTracks();

    const float gain = VolumeToGain(sfx_volume_);
    for (MIX_Track* track : active_sfx_tracks_) {
        if (track) {
            MIX_SetTrackGain(track, gain);
        }
    }
}

void AudioEngine::SetMusicVolume(int volume) {
    music_volume_ = volume;
    if (music_track_) {
        MIX_SetTrackGain(music_track_, VolumeToGain(music_volume_));
    }
}
