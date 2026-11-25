// AudioManager: loading Audio Data files and playing them. See audio_manager.h.
#include "engine/audio/audio_manager.h"

#include <algorithm>
#include <cmath>

namespace rage {
    AudioManager::~AudioManager() { Unload(); }

    void AudioManager::WarnOnce(const std::string &key, int logLevel, const std::string &message) {
        // The same missing file would otherwise be reported every time it is played.
        if (warned.insert(key).second) TraceLog(logLevel, "%s", message.c_str());
    }

    // Sound effects are looked up in audio/sfx/ and music in audio/music/, then the other
    // folder. A File Name with a folder, like "level1/rain.ogg", is read from audio/level1/.
    std::filesystem::path AudioManager::Resolve(const SoundData &sound, const std::string &file) const {
        std::string name = file;
        std::replace(name.begin(), name.end(), '\\', '/');
        if (name.find('/') != std::string::npos) return directory / "audio" / name;
        const auto preferred = directory / (sound.IsMusic() ? musicFolder : sfxFolder) / name;
        const auto other = directory / (sound.IsMusic() ? sfxFolder : musicFolder) / name;
        return !FileExists(preferred.string().c_str()) && FileExists(other.string().c_str()) ? other : preferred;
    }

    float AudioManager::CategoryVolume(AudioCategory category) const {
        return category == AudioCategory::Music ? musicVolume : sfxVolume;
    }

    void AudioManager::Load(const std::filesystem::path &resourceDirectory) {
        Unload(); // Loading twice must not leak the first set of sounds.
        directory = resourceDirectory;
        data.LoadFile(directory / audioDataPath);
        for (const auto &problem: data.Problems()) TraceLog(LOG_WARNING, "%s", problem.c_str());
        if (!IsAudioDeviceReady()) {
            TraceLog(LOG_INFO, "Audio: no audio device; %d sounds are silent", static_cast<int>(data.Size()));
            return;
        }
        int loaded = 0;
        int silent = 0;
        for (const auto *sound: data.All()) {
            const auto names = VariantFileNames(*sound);
            if (names.empty()) {
                ++silent;
                TraceLog(LOG_INFO, "Audio: %s has no File Name yet; it is silent", sound->soundId.c_str());
                continue;
            }
            for (const auto &name: names) {
                const auto path = Resolve(*sound, name);
                if (!FileExists(path.string().c_str())) {
                    ++silent;
                    WarnOnce("missing:" + path.string(), LOG_WARNING, "Audio: " + name + " for " + sound->soundId
                             + " was not found in resources/" + (sound->IsMusic() ? musicFolder : sfxFolder) + "; it is silent");
                    continue;
                }
                if (sound->IsMusic()) continue; // Music is streamed from disk when it plays.
                const std::string key = path.string();
                if (!files.contains(key)) {
                    Voices entry;
                    const Sound base = LoadSound(key.c_str());
                    if (!IsSoundValid(base)) {
                        ++silent;
                        WarnOnce("invalid:" + key, LOG_WARNING, "Audio: " + name + " for " + sound->soundId
                                 + " could not be loaded; it is silent");
                        continue;
                    }
                    entry.voices.push_back(base);
                    // Aliases share the loaded data, so extra voices cost almost no memory.
                    for (int i = 1; i < voicesPerSound; ++i) {
                        const Sound alias = LoadSoundAlias(base);
                        if (IsSoundValid(alias)) entry.voices.push_back(alias);
                    }
                    files.emplace(key, std::move(entry));
                    ++loaded;
                }
                soundFiles[sound->soundId].push_back(key);
            }
        }
        TraceLog(LOG_INFO, "Audio Data: %d sounds, %d files loaded, %d silent",
                 static_cast<int>(data.Size()), loaded, silent);
    }

    void AudioManager::Unload() {
        UnloadTrack(current);
        UnloadTrack(previous);
        for (auto &[path, entry]: files) {
            // Aliases must be released before the sound whose data they share.
            for (std::size_t i = entry.voices.size(); i-- > 1;) UnloadSoundAlias(entry.voices[i]);
            if (!entry.voices.empty()) UnloadSound(entry.voices[0]);
        }
        files.clear();
        soundFiles.clear();
        data.Clear();
        warned.clear();
        requestedMusic.clear();
        paused = false;
    }

    void AudioManager::Play(std::string_view soundId) {
        const SoundData *sound = data.Find(soundId);
        if (!sound) {
            WarnOnce("unknown:" + std::string(soundId), LOG_WARNING,
                     "Audio: no sound \"" + std::string(soundId) + "\" in " + audioDataPath + "; it is silent");
            return;
        }
        if (sound->IsMusic()) {
            PlayMusic(soundId);
            return;
        }
        if (!IsAudioDeviceReady()) return;
        const auto found = soundFiles.find(sound->soundId);
        if (found == soundFiles.end() || found->second.empty()) return; // Missing file: already logged.
        // Pick a variant at random, then the next free voice of that file.
        std::uniform_int_distribution<std::size_t> pick(0, found->second.size() - 1);
        Voices &entry = files.at(found->second[pick(random)]);
        const Sound &voice = entry.voices[entry.next++ % entry.voices.size()];
        SetSoundVolume(voice, EffectiveVolume(*sound, CategoryVolume(sound->category)));
        PlaySound(voice);
    }

    void AudioManager::PlayMusic(std::string_view soundId, float fadeSeconds) {
        if (soundId == requestedMusic) return; // Already playing (or already known to be missing).
        requestedMusic = std::string(soundId);
        if (soundId.empty()) {
            MoveCurrentToPrevious(fadeSeconds);
            return;
        }
        const SoundData *sound = data.Find(soundId);
        const auto silence = [&](const std::string &key, int level, const std::string &message) {
            WarnOnce(key, level, message);
            MoveCurrentToPrevious(fadeSeconds);
        };
        if (!sound) {
            silence("unknown:" + requestedMusic, LOG_WARNING,
                    "Audio: no music \"" + requestedMusic + "\" in " + audioDataPath + "; music is silent");
            return;
        }
        if (!sound->IsMusic()) {
            silence("notmusic:" + requestedMusic, LOG_WARNING,
                    "Audio: \"" + requestedMusic + "\" is not in the Music category; play it with Play() instead");
            return;
        }
        if (!IsAudioDeviceReady()) return;
        if (sound->fileName.empty()) {
            silence("nofile:" + requestedMusic, LOG_INFO, "Audio: " + requestedMusic + " has no File Name yet; music is silent");
            return;
        }
        const auto path = Resolve(*sound, sound->fileName);
        if (!FileExists(path.string().c_str())) {
            MoveCurrentToPrevious(fadeSeconds); // The missing file was already logged by Load.
            return;
        }
        Music music = LoadMusicStream(path.string().c_str());
        if (!IsMusicValid(music)) {
            silence("invalid:" + path.string(), LOG_WARNING, "Audio: " + sound->fileName + " could not be streamed; music is silent");
            return;
        }
        music.looping = sound->loops;
        MoveCurrentToPrevious(fadeSeconds);
        current = {requestedMusic, music, sound->volume, fadeSeconds > 0 ? 0.0f : 1.0f, 1,
                   fadeSeconds > 0 ? 1 / fadeSeconds : 0.0f, true};
        ApplyMusicVolume(current); // Set before playing, so a fade-in really starts silent.
        PlayMusicStream(current.music);
    }

    void AudioManager::StopMusic(float fadeSeconds) {
        requestedMusic.clear();
        MoveCurrentToPrevious(fadeSeconds);
    }

    // The playing track starts fading out; only one fading track is kept, so an older one
    // (from a very quick second switch) is dropped straight away.
    void AudioManager::MoveCurrentToPrevious(float fadeSeconds) {
        if (!current.valid) return;
        UnloadTrack(previous);
        previous = current;
        current = {};
        FadeOut(previous, fadeSeconds);
        if (!previous.valid) return;
        ApplyMusicVolume(previous);
    }

    void AudioManager::FadeOut(Track &track, float seconds) {
        track.target = 0;
        if (seconds > 0) {
            track.speed = 1 / seconds;
            return;
        }
        UnloadTrack(track); // No fade: stop at once.
    }

    void AudioManager::UnloadTrack(Track &track) {
        if (track.valid) {
            StopMusicStream(track.music);
            UnloadMusicStream(track.music);
        }
        track = {};
    }

    void AudioManager::ApplyMusicVolume(Track &track) {
        if (!track.valid) return;
        const float level = track.rowVolume * musicVolume * track.fade * (paused ? pausedMusicLevel : 1.0f);
        SetMusicVolume(track.music, std::clamp(level, 0.0f, 1.0f));
    }

    void AudioManager::Update(float dt) {
        if (!std::isfinite(dt) || dt < 0) dt = 0;
        for (Track *track: {&current, &previous}) {
            if (!track->valid) continue;
            UpdateMusicStream(track->music); // Streams the next chunk; music stops without this.
            if (track->speed > 0) {
                const float step = track->speed * dt;
                track->fade = track->fade < track->target ? std::min(track->target, track->fade + step)
                                                          : std::max(track->target, track->fade - step);
            } else track->fade = track->target;
            ApplyMusicVolume(*track);
        }
        if (previous.valid && previous.fade <= 0) UnloadTrack(previous); // Faded out: release it.
    }

    void AudioManager::SetVolumes(float music, float sfx) {
        musicVolume = std::clamp(std::isfinite(music) ? music : 1.0f, 0.0f, 1.0f);
        sfxVolume = std::clamp(std::isfinite(sfx) ? sfx : 1.0f, 0.0f, 1.0f);
        ApplyMusicVolume(current);
        ApplyMusicVolume(previous);
    }

    void AudioManager::SetPaused(bool nowPaused) {
        if (nowPaused == paused) return;
        paused = nowPaused;
        if (paused) {
            // Cut sound effects that were playing when the game paused.
            for (auto &[path, entry]: files)
                for (const Sound &voice: entry.voices)
                    if (IsSoundPlaying(voice)) StopSound(voice);
        }
        ApplyMusicVolume(current);
        ApplyMusicVolume(previous);
    }

    bool AudioManager::IsLoaded(std::string_view soundId) const {
        const auto found = soundFiles.find(std::string(soundId));
        return found != soundFiles.end() && !found->second.empty();
    }

    bool AudioManager::IsPlaying(std::string_view soundId) const {
        const auto found = soundFiles.find(std::string(soundId));
        if (found == soundFiles.end()) return false;
        for (const auto &path: found->second)
            for (const Sound &voice: files.at(path).voices)
                if (IsSoundPlaying(voice)) return true;
        return false;
    }

    bool AudioManager::IsMusicPlaying() const {
        return current.valid && IsMusicStreamPlaying(current.music);
    }
}
