// Audio Data: the spreadsheet tab that describes every sound effect and music track.
//
// Sound designers fill in the "Audio" tab of the CISC 320 Game Data workbook and export it
// as game/resources/data/audio_data.csv. This file defines one row (SoundData) and the
// table of all rows (AudioData). No raylib, so it can be tested without an audio device.
// Loading and playing the files is in audio_manager.h. See README.md in this folder.
#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace rage {
    // Which volume slider a sound follows. Music uses the Music slider; the rest use SFX.
    enum class AudioCategory { Sfx, Ui, Music, Voice, Count };

    // One row of the Audio tab.
    struct SoundData {
        std::string soundId;     // What code asks for, e.g. "player_jump". Unique.
        std::string fileName;    // e.g. "jump.wav". Empty: no file yet, so it stays silent.
        AudioCategory category = AudioCategory::Sfx;
        float volume = 1;        // 0-1, multiplied by the category's slider.
        bool loops = false;      // Music loops by default; sound effects play once.
        int variants = 1;        // >1: files are numbered (jump_1.wav ... jump_3.wav), picked at random.
        std::string notes;       // For people only; the game ignores it.
        int row = 0;             // Spreadsheet row number, used in warning messages.

        bool IsMusic() const { return category == AudioCategory::Music; }
    };

    // The file names to load for a row. With one variant this is just fileName; with three,
    // "jump.wav" becomes jump_1.wav, jump_2.wav and jump_3.wav. Empty when fileName is empty.
    std::vector<std::string> VariantFileNames(const SoundData &sound);

    // Final volume for one sound before the master volume: row volume x its category's
    // slider, clamped to 0-1. (Master volume and mute are applied globally by GameWindow.)
    float EffectiveVolume(const SoundData &sound, float categoryVolume);

    // All imported rows, with lookups. Importing never throws: bad rows are skipped, bad
    // values fall back to defaults, and each problem is recorded in Problems() with its row.
    class AudioData {
    public:
        // Reads a CSV file. Returns false if the file is missing or any problem was found.
        bool LoadFile(const std::filesystem::path &path);

        // Parses CSV text. Returns false if any problem was found; valid rows are still kept.
        bool Parse(std::string_view csvText, std::string_view source = "Audio Data");

        void Clear();

        // Lookup by Sound ID, e.g. Find("player_jump"). Returns nullptr if missing.
        const SoundData *Find(std::string_view soundId) const;

        // Every sound, in sheet order.
        std::vector<const SoundData *> All() const;

        std::size_t Size() const { return sounds.size(); }
        const std::vector<std::string> &Problems() const { return problems; }

    private:
        void Report(std::string message);

        // unordered_map never moves its elements, so pointers from Find() stay valid.
        std::unordered_map<std::string, SoundData> sounds;
        std::vector<std::string> order;    // Every Sound ID, in sheet order.
        std::vector<std::string> problems; // Warnings for the console.
        std::string source;                // File name used in warnings.
    };
}
