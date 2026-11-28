// AudioManager: loads the files listed in Audio Data and plays them by Sound ID.
//
// There is one AudioManager for the whole game. Resources owns it (resources.Audio()), and
// GameWindow drives it every frame: it plays the sounds the game requested, keeps the
// requested music track streaming, and applies the Music/SFX volume sliders.
//
//   audio.Play("player_jump");          // sound effect: overlaps, random variant
//   audio.PlayMusic("level_1_theme");   // crossfades from the current track
//   audio.StopMusic(0.5f);              // fades out over half a second
//
// Gameplay code does not call this directly. A Level requests sounds (Level::RequestSound)
// and GameWindow plays them, so gameplay stays testable without speakers.
//
// Missing files, bad rows or no audio device never stop the game: each problem is logged
// once and the sound simply stays silent.
#pragma once

#include "engine/audio/audio_data.h"
#include "raylib.h"

#include <cstddef>
#include <filesystem>
#include <random>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace rage {
    // Where things live, relative to the game/resources/ folder.
    inline constexpr const char *audioDataPath = "data/audio_data.csv";
    inline constexpr const char *sfxFolder = "audio/sfx";     // Sound effects, UI and voice: WAV.
    inline constexpr const char *musicFolder = "audio/music"; // Music, streamed: OGG.

    // Sound IDs the engine itself uses. Add them to the Audio tab to hear them.
    inline constexpr const char *uiClickSound = "ui_click";
    inline constexpr const char *menuMusic = "menu_theme";

    class AudioManager {
    public:
        // Copies of each sound effect, so rapid repeats overlap instead of cutting each other off.
        static constexpr int voicesPerSound = 4;
        // Music volume while the game is paused (a fraction of normal).
        static constexpr float pausedMusicLevel = 0.35f;

        AudioManager() = default;

        ~AudioManager(); // Unloads everything.

        // Copying is disabled: two copies would both try to unload the same sounds.
        AudioManager(const AudioManager &) = delete;

        AudioManager &operator=(const AudioManager &) = delete;

        // Reads data/audio_data.csv and loads every sound effect once. Music is streamed
        // when played. Without an audio device only the rows are read.
        void Load(const std::filesystem::path &resourceDirectory);

        // Stops and unloads everything and forgets all rows.
        void Unload();

        const AudioData &Data() const { return data; }

        // Plays a sound effect (SFX, UI or Voice). A Music row is passed to PlayMusic.
        void Play(std::string_view soundId);

        // Switches to a music track, crossfading over `fadeSeconds`. Asking for the track that
        // is already playing does nothing, so it is safe to call every frame. An empty ID or
        // an unknown/missing track fades the music out.
        void PlayMusic(std::string_view soundId, float fadeSeconds = 1);

        // Fades the music out over `fadeSeconds` (0 stops it at once).
        void StopMusic(float fadeSeconds = 1);

        // Call once per frame: keeps music streaming and moves fades along.
        void Update(float dt);

        // Slider values, 0-1. Music rows use `music`; everything else uses `sfx`.
        // (Master volume and mute are applied globally by GameWindow.)
        void SetVolumes(float music, float sfx);

        // Entering pause stops sound effects that are playing and turns the music down;
        // leaving pause brings the music back. Sounds played while paused (menu clicks) still play.
        void SetPaused(bool paused);

        // For tests and debugging.
        bool IsLoaded(std::string_view soundId) const;
        bool IsPlaying(std::string_view soundId) const;
        const std::string &CurrentMusic() const { return current.id; }
        bool IsMusicPlaying() const;

    private:
        // One loaded file: the sound plus its aliases (extra voices that share its data).
        struct Voices {
            std::vector<Sound> voices; // voices[0] is the loaded sound; the rest are aliases.
            std::size_t next = 0;      // Next voice to use, round robin.
        };

        // A music stream that is playing or fading.
        struct Track {
            std::string id;
            Music music = {};
            float rowVolume = 1;
            float fade = 0;    // Current fade level, 0-1.
            float target = 1;  // Fade level it is moving toward.
            float speed = 0;   // Fade change per second; 0 means jump straight to the target.
            bool valid = false;
            float appliedVolume = -1; // Unset until the first volume reaches this stream.
        };

        std::filesystem::path Resolve(const SoundData &sound, const std::string &file) const;
        float CategoryVolume(AudioCategory category) const;
        void FadeOut(Track &track, float seconds);
        void MoveCurrentToPrevious(float fadeSeconds);
        void UnloadTrack(Track &track);
        void ApplyMusicVolume(Track &track);
        void WarnOnce(const std::string &key, int logLevel, const std::string &message);

        AudioData data;
        std::filesystem::path directory;
        std::unordered_map<std::string, Voices> files;                    // Loaded files by path.
        std::unordered_map<std::string, std::vector<std::string>> soundFiles; // Sound ID -> its file paths.
        Track current;                // The track that is playing (fading in or at full volume).
        Track previous;               // The track fading out after a switch.
        std::string requestedMusic;   // Last ID given to PlayMusic, so repeated calls do nothing.
        float musicVolume = 1;
        float sfxVolume = 1;
        bool paused = false;
        std::unordered_set<std::string> warned; // Messages already logged once.
        std::mt19937 random{std::random_device{}()};
    };
}
