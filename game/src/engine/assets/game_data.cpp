// Game Data import: turns the exported CSV into AnimationData rows, and the maths that
// turns a row plus a time into "which part of the PNG to draw". See game_data.h.
#include "engine/assets/game_data.h"
#include "engine/data/csv.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <optional>
#include <sstream>

namespace rage {
    // The CSV helpers (Trim, Key, Number...) live in engine/data/csv, shared with other importers.
    namespace {
        using csv::Integer;
        using csv::IsIdentifier;
        using csv::Key;
        using csv::Number;
        using csv::Trim;

        // The sheet's columns, A to M. A plain enum so each name doubles as an array index.
        enum Column { AnimationId, AssetId, FileName, FrameWidth, FrameHeight, Frames, FramesPerRow, Fps, Loops,
            PivotColumn, Faces, LayerColumn, Notes, ColumnCount };

        // Sheet header text, in the documented column order (A-M), with accepted alternatives.
        const std::vector<csv::ColumnName> &Columns() {
            static const std::vector<csv::ColumnName> columns = {
                {"Animation ID"}, {"Asset ID"}, {"File Name"}, {"Frame Width (px)", {"Frame Width"}},
                {"Frame Height (px)", {"Frame Height"}}, {"Frames"}, {"Frames per Row"}, {"FPS"}, {"Loops"},
                {"Pivot"}, {"Faces"}, {"Layer"}, {"Notes"}
            };
            return columns;
        }
    }

    // Kept for existing callers; the parser itself is csv::Parse.
    std::vector<std::vector<std::string>> ParseCsv(std::string_view text) { return csv::Parse(text); }

    // Example: 6 frames, 3 per row, 32x32 each. Frame 4 is column 4 % 3 = 1, row 4 / 3 = 1,
    // so it starts at (32, 32).
    FrameRect FrameSource(const AnimationData &animation, int frame) {
        const int count = std::max(1, animation.frames);
        frame = std::clamp(frame, 0, count - 1);
        const int columns = std::max(1, animation.Columns());
        return {
            static_cast<float>(frame % columns * animation.frameWidth),
            static_cast<float>(frame / columns * animation.frameHeight),
            static_cast<float>(animation.frameWidth), static_cast<float>(animation.frameHeight)
        };
    }

    // Example: 12 FPS for 0.25 s is position 3.0, so frame 3. A 6-frame looping walk at
    // 0.5 s is position 6.0, which wraps back to frame 0.
    int FrameAt(const AnimationData &animation, double seconds) {
        if (!animation.IsAnimated() || !std::isfinite(seconds) || seconds <= 0) return 0;
        const double position = seconds * animation.fps;
        if (animation.loops) return static_cast<int>(std::fmod(position, animation.frames));
        return std::min(static_cast<int>(position), animation.frames - 1);
    }

    bool IsFinished(const AnimationData &animation, double seconds) {
        if (!animation.IsAnimated()) return true;
        return !animation.loops && std::isfinite(seconds) && seconds * animation.fps >= animation.frames;
    }

    void PivotOffset(Pivot pivot, float width, float height, float &x, float &y) {
        switch (pivot) {
            case Pivot::BottomCentre: x = width / 2;
                y = height;
                return;
            case Pivot::TopLeft: x = 0;
                y = 0;
                return;
            case Pivot::Centre: x = width / 2;
                y = height / 2;
                return;
        }
        x = y = 0;
    }

    bool ShouldFlip(Facing art, Facing object) {
        return art != Facing::None && object != Facing::None && art != object;
    }

    float FitScale(int frameWidth, int frameHeight, float targetWidth, float targetHeight, float pixelsPerUnit) {
        if (frameWidth <= 0 || frameHeight <= 0 || pixelsPerUnit <= 0) return 1;
        // The smaller ratio keeps the aspect ratio: grow until the first side touches the target.
        const float fit = std::min(targetWidth / frameWidth, targetHeight / frameHeight);
        // Round down to whole screen pixels so pixel art stays crisp, but never below one.
        const float screenPixels = std::max(1.0f, std::floor(fit * pixelsPerUnit));
        return screenPixels / pixelsPerUnit;
    }

    void GameData::Clear() {
        animations.clear();
        assetAnimations.clear();
        order.clear();
        problems.clear();
    }

    void GameData::Report(std::string message) {
        problems.push_back(source + ": " + std::move(message));
    }

    // Reads the whole file into memory and hands it to Parse.
    bool GameData::LoadFile(const std::filesystem::path &path) {
        Clear();
        source = path.filename().string();
        const auto text = csv::ReadFile(path);
        if (!text) {
            Report("file not found at " + path.string() + "; no Game Data animations are available");
            return false;
        }
        return Parse(*text, source);
    }

    // Import in three steps: find the header row, work out which column is which, then
    // check and store each data row. Nothing here throws; problems go to Report().
    bool GameData::Parse(std::string_view csvText, std::string_view sourceName) {
        Clear();
        source = std::string(sourceName);
        const auto rows = csv::Parse(csvText);
        const auto blank = csv::IsBlank;
        // Step 1: the header is the first row that is not blank.
        std::size_t headerRow = 0;
        while (headerRow < rows.size() && blank(rows[headerRow])) ++headerRow;
        if (headerRow == rows.size()) {
            Report("no header row");
            return false;
        }

        // Step 2: find each column by its header text, so reordered columns still work.
        // If a header was renamed (the sheet's column H once said "bottom-centre" instead
        // of "FPS"), fall back to the documented position, as long as that cell is not
        // some other known header. index[column] = cell position, or -1 if missing.
        const auto &header = rows[headerRow];
        const auto &columns = Columns();
        const csv::ColumnMap map = csv::MapColumns(header, columns);
        const std::vector<int> &index = map.index;
        for (const int c: map.renamed) {
            if (c == Notes) continue; // Notes is never read by the game; no need to warn.
            Report("column " + csv::ColumnLetter(c) + " header is \"" + Trim(header[c]) + "\" but should be \""
                   + columns[c].header + "\"; reading it as " + columns[c].header);
        }
        // Without these four columns no row can be used, even as a placeholder.
        for (const Column required: {AnimationId, AssetId, FrameWidth, FrameHeight}) {
            if (index[required] < 0) {
                Report(std::string("missing required column \"") + columns[required].header + "\"; nothing was imported");
                return false;
            }
        }

        // Step 3: check and store each data row.
        for (std::size_t r = headerRow + 1; r < rows.size(); ++r) {
            const auto &cells = rows[r];
            if (blank(cells)) continue; // Google Sheets exports empty rows as ",,,,".
            const int sheetRow = static_cast<int>(r) + 1; // Sheets count rows from 1.
            // cell(Fps) returns this row's trimmed FPS text, or "" if the column is missing.
            const auto cell = [&](Column column) {
                const int i = index[column];
                return i >= 0 && i < static_cast<int>(cells.size()) ? Trim(cells[i]) : std::string();
            };
            AnimationData data;
            data.row = sheetRow;
            data.animationId = cell(AnimationId);
            data.assetId = cell(AssetId);
            const std::string label = "row " + std::to_string(sheetRow)
                                      + (data.animationId.empty() ? "" : " (" + data.animationId + ")");
            const auto problem = [&](const std::string &message) { Report(label + ": " + message); };

            // Rows that cannot be used at all are skipped. From here on, a bad value is
            // reported and replaced with a default, and the rest of the row is kept.
            if (data.animationId.empty()) {
                problem("Animation ID is blank; row skipped");
                continue;
            }
            if (data.assetId.empty()) {
                problem("Asset ID is blank; row skipped");
                continue;
            }
            if (!IsIdentifier(data.animationId) || !IsIdentifier(data.assetId))
                problem("IDs should be lowercase letters, digits and underscores");
            if (animations.contains(data.animationId)) {
                problem("duplicate Animation ID; the earlier row is used");
                continue;
            }

            // The frame size is needed even for a placeholder, so a bad size skips the row.
            const auto width = Integer(cell(FrameWidth));
            const auto height = Integer(cell(FrameHeight));
            if (!width || !height || *width <= 0 || *height <= 0) {
                problem("Frame Width and Frame Height must be positive whole numbers (got \"" + cell(FrameWidth)
                        + "\" x \"" + cell(FrameHeight) + "\"); row skipped");
                continue;
            }
            data.frameWidth = *width;
            data.frameHeight = *height;

            // "none" marks an object that is never drawn (a trigger zone). A blank name means
            // "art not made yet" and is not a problem: a placeholder is drawn.
            const std::string file = cell(FileName);
            if (Key(file) == "none") data.invisible = true;
            else data.fileName = file;

            if (const auto text = cell(Frames); !text.empty()) {
                const auto frames = Integer(text);
                if (frames && *frames >= 1) data.frames = *frames;
                else problem("Frames \"" + text + "\" is not a whole number of at least 1; using 1");
            }
            if (const auto text = cell(FramesPerRow); !text.empty()) {
                const auto perRow = Integer(text);
                if (perRow && *perRow >= 1) data.framesPerRow = std::min(*perRow, data.frames);
                else problem("Frames per Row \"" + text + "\" is not a whole number of at least 1; using one row");
            }
            if (const auto text = cell(Fps); !text.empty()) {
                const auto fps = Number(text);
                if (fps && *fps > 0) data.fps = static_cast<float>(*fps);
                else problem("FPS \"" + text + "\" is not a positive number; showing the first frame only");
            }

            // Key() lowercases and strips symbols, so "Yes", "YES" and " yes " all match "yes".
            const auto loops = Key(cell(Loops));
            if (loops == "yes" || loops == "y" || loops == "true") data.loops = true;
            else if (loops == "no" || loops == "n" || loops == "false") data.loops = false;
            else if (loops.empty()) {
                // Blank is normal for static assets and tile sets (several frames, no FPS).
                data.loops = data.IsAnimated();
                if (data.IsAnimated()) problem("Loops is blank; looping");
            } else {
                data.loops = true;
                problem("Loops \"" + cell(Loops) + "\" should be Yes or No; looping");
            }

            const auto pivot = Key(cell(PivotColumn));
            if (pivot == "bottomcentre" || pivot == "bottomcenter") data.pivot = Pivot::BottomCentre;
            else if (pivot == "topleft") data.pivot = Pivot::TopLeft;
            else if (pivot == "centre" || pivot == "center") data.pivot = Pivot::Centre;
            else problem("Pivot \"" + cell(PivotColumn) + "\" should be bottom-centre, top-left or centre; using bottom-centre");

            const auto faces = Key(cell(Faces));
            if (faces == "right") data.faces = Facing::Right;
            else if (faces == "left") data.faces = Facing::Left;
            else if (!faces.empty()) problem("Faces \"" + cell(Faces) + "\" should be right, left or blank; ignoring it");

            const auto layer = Key(cell(LayerColumn));
            if (layer == "background") data.layer = Layer::Background;
            else if (layer == "middle") data.layer = Layer::Middle;
            else if (layer == "foreground") data.layer = Layer::Foreground;
            else if (layer == "ui") data.layer = Layer::Ui;
            else problem("Layer \"" + cell(LayerColumn) + "\" should be Background, Middle, Foreground or UI; using Middle");

            // The row is valid: store it and index it under its Asset ID.
            data.notes = cell(Notes);
            assetAnimations[data.assetId].push_back(data.animationId);
            order.push_back(data.animationId);
            const std::string id = data.animationId;
            animations.emplace(id, std::move(data));
        }
        return problems.empty();
    }

    // unordered_map lookups need a std::string key, hence the std::string(...) conversions.
    const AnimationData *GameData::Find(std::string_view animationId) const {
        const auto found = animations.find(std::string(animationId));
        return found == animations.end() ? nullptr : &found->second;
    }

    const AnimationData *GameData::Find(std::string_view assetId, std::string_view state) const {
        const auto asset = assetAnimations.find(std::string(assetId));
        if (asset == assetAnimations.end() || asset->second.empty()) return nullptr;
        if (state.empty()) return Find(asset->second.front());
        if (const auto *exact = Find(std::string(assetId) + "_" + std::string(state)); exact && exact->assetId == assetId)
            return exact;
        // No exact match: accept any of this asset's animations ending in "_<state>", so
        // ("player_character", "walk") finds "player_walk".
        const std::string suffix = "_" + std::string(state);
        for (const auto &id: asset->second) {
            if (id == state || (id.size() > suffix.size() && id.ends_with(suffix))) return Find(id);
        }
        return nullptr;
    }

    std::vector<const AnimationData *> GameData::ForAsset(std::string_view assetId) const {
        std::vector<const AnimationData *> result;
        if (const auto asset = assetAnimations.find(std::string(assetId)); asset != assetAnimations.end())
            for (const auto &id: asset->second) result.push_back(Find(id));
        return result;
    }

    std::vector<const AnimationData *> GameData::All() const {
        std::vector<const AnimationData *> result;
        for (const auto &id: order) result.push_back(Find(id));
        return result;
    }
}
