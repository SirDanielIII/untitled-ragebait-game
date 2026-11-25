// Audio Data import: turns the exported CSV into SoundData rows. See audio_data.h.
#include "engine/audio/audio_data.h"
#include "engine/data/csv.h"

#include <algorithm>

namespace rage {
    namespace {
        // The Audio tab's columns, A to G, in the documented order.
        enum Column { SoundId, FileName, CategoryColumn, Volume, Loops, Variants, Notes, ColumnCount };

        const std::vector<csv::ColumnName> &Columns() {
            static const std::vector<csv::ColumnName> columns = {
                {"Sound ID"}, {"File Name"}, {"Category"}, {"Volume"}, {"Loops"}, {"Variants"}, {"Notes"}
            };
            return columns;
        }

        // Upper limit for Variants, so a typo like 300 can't try to open hundreds of files.
        constexpr int maxVariants = 20;
    }

    std::vector<std::string> VariantFileNames(const SoundData &sound) {
        if (sound.fileName.empty()) return {};
        if (sound.variants <= 1) return {sound.fileName};
        // "jump.wav" -> "jump" + "_1" + ".wav". A name without an extension just gets "_1".
        const auto dot = sound.fileName.find_last_of('.');
        const auto slash = sound.fileName.find_last_of("/\\");
        const bool hasExtension = dot != std::string::npos && (slash == std::string::npos || dot > slash);
        const std::string stem = hasExtension ? sound.fileName.substr(0, dot) : sound.fileName;
        const std::string extension = hasExtension ? sound.fileName.substr(dot) : "";
        std::vector<std::string> names;
        for (int i = 1; i <= sound.variants; ++i) names.push_back(stem + "_" + std::to_string(i) + extension);
        return names;
    }

    float EffectiveVolume(const SoundData &sound, float categoryVolume) {
        return std::clamp(sound.volume * categoryVolume, 0.0f, 1.0f);
    }

    void AudioData::Clear() {
        sounds.clear();
        order.clear();
        problems.clear();
    }

    void AudioData::Report(std::string message) {
        problems.push_back(source + ": " + std::move(message));
    }

    bool AudioData::LoadFile(const std::filesystem::path &path) {
        Clear();
        source = path.filename().string();
        const auto text = csv::ReadFile(path);
        if (!text) {
            Report("file not found at " + path.string() + "; no sounds or music are available");
            return false;
        }
        return Parse(*text, source);
    }

    // Same three steps as Game Data: find the header row, map the columns, then check
    // and store each row. Nothing here throws; problems go to Report().
    bool AudioData::Parse(std::string_view csvText, std::string_view sourceName) {
        Clear();
        source = std::string(sourceName);
        const auto rows = csv::Parse(csvText);
        std::size_t headerRow = 0;
        while (headerRow < rows.size() && csv::IsBlank(rows[headerRow])) ++headerRow;
        if (headerRow == rows.size()) {
            Report("no header row");
            return false;
        }

        const auto &header = rows[headerRow];
        const auto &columns = Columns();
        const csv::ColumnMap map = csv::MapColumns(header, columns);
        for (const int c: map.renamed) {
            if (c == Notes) continue;
            Report("column " + csv::ColumnLetter(c) + " header is \"" + csv::Trim(header[c]) + "\" but should be \""
                   + columns[c].header + "\"; reading it as " + columns[c].header);
        }
        if (map.index[SoundId] < 0) {
            Report("missing required column \"Sound ID\"; nothing was imported");
            return false;
        }

        for (std::size_t r = headerRow + 1; r < rows.size(); ++r) {
            const auto &cells = rows[r];
            if (csv::IsBlank(cells)) continue;
            const int sheetRow = static_cast<int>(r) + 1;
            const auto cell = [&](Column column) {
                const int i = map.index[column];
                return i >= 0 && i < static_cast<int>(cells.size()) ? csv::Trim(cells[i]) : std::string();
            };
            SoundData sound;
            sound.row = sheetRow;
            sound.soundId = cell(SoundId);
            const std::string label = "row " + std::to_string(sheetRow) + (sound.soundId.empty() ? "" : " (" + sound.soundId + ")");
            const auto problem = [&](const std::string &message) { Report(label + ": " + message); };

            if (sound.soundId.empty()) {
                problem("Sound ID is blank; row skipped");
                continue;
            }
            if (!csv::IsIdentifier(sound.soundId)) problem("IDs should be lowercase letters, digits and underscores");
            if (sounds.contains(sound.soundId)) {
                problem("duplicate Sound ID; the earlier row is used");
                continue;
            }

            sound.fileName = cell(FileName); // Blank is fine: the sound is planned but has no file yet.

            const auto category = csv::Key(cell(CategoryColumn));
            if (category == "sfx" || category == "soundeffect" || category.empty()) sound.category = AudioCategory::Sfx;
            else if (category == "ui") sound.category = AudioCategory::Ui;
            else if (category == "music") sound.category = AudioCategory::Music;
            else if (category == "voice") sound.category = AudioCategory::Voice;
            else problem("Category \"" + cell(CategoryColumn) + "\" should be SFX, UI, Music or Voice; using SFX");

            if (const auto text = cell(Volume); !text.empty()) {
                const auto volume = csv::Number(text);
                if (!volume) problem("Volume \"" + text + "\" is not a number; using 1");
                else if (*volume < 0 || *volume > 1) {
                    sound.volume = std::clamp(static_cast<float>(*volume), 0.0f, 1.0f);
                    problem("Volume \"" + text + "\" should be between 0 and 1; using " + (sound.volume == 0 ? "0" : "1"));
                } else sound.volume = static_cast<float>(*volume);
            }

            const auto loops = csv::Key(cell(Loops));
            if (loops == "yes" || loops == "y" || loops == "true") sound.loops = true;
            else if (loops == "no" || loops == "n" || loops == "false") sound.loops = false;
            else {
                // Blank means the usual choice: music loops, sound effects play once.
                sound.loops = sound.IsMusic();
                if (!loops.empty()) problem("Loops \"" + cell(Loops) + "\" should be Yes or No; using " + (sound.loops ? "Yes" : "No"));
            }

            if (const auto text = cell(Variants); !text.empty()) {
                const auto variants = csv::Integer(text);
                if (variants && *variants >= 1 && *variants <= maxVariants) sound.variants = *variants;
                else problem("Variants \"" + text + "\" should be a whole number from 1 to " + std::to_string(maxVariants) + "; using 1");
            }
            if (sound.IsMusic() && sound.variants > 1) {
                problem("music cannot have variants; playing File Name as written");
                sound.variants = 1;
            }

            sound.notes = cell(Notes);
            order.push_back(sound.soundId);
            const std::string id = sound.soundId;
            sounds.emplace(id, std::move(sound));
        }
        return problems.empty();
    }

    const SoundData *AudioData::Find(std::string_view soundId) const {
        const auto found = sounds.find(std::string(soundId));
        return found == sounds.end() ? nullptr : &found->second;
    }

    std::vector<const SoundData *> AudioData::All() const {
        std::vector<const SoundData *> result;
        for (const auto &id: order) result.push_back(Find(id));
        return result;
    }
}
