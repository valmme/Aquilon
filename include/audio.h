#ifndef AQUILON_AUDIO_H
#define AQUILON_AUDIO_H

#include <string>
#include <unordered_map>
#include <vector>

struct MIX_Audio;
struct MIX_Mixer;
struct MIX_Track;

class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();

    bool Init();
    void Shutdown();

    bool LoadSound(const std::string& id, const std::string& path);
    bool LoadMusic(const std::string& id, const std::string& path);

    void PlaySound(const std::string& id, int loops = 0);
    void PlayMusic(const std::string& id, int loops = -1);
    void StopMusic();

    void SetSFXVolume(int volume); // 0..128
    void SetMusicVolume(int volume); // 0..128

private:
    void CleanupFinishedTracks();

    MIX_Mixer* mixer_ = nullptr;
    MIX_Track* music_track_ = nullptr;
    std::vector<MIX_Track*> active_sfx_tracks_;
    std::unordered_map<std::string, MIX_Audio*> sounds_;
    std::unordered_map<std::string, MIX_Audio*> musics_;
    int sfx_volume_ = 128;
    int music_volume_ = 128;
    bool initialized_ = false;
};

#endif // AQUILON_AUDIO_H
