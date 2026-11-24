// Game Data: the spreadsheet that describes every sprite and animation in the game.
//
// Artists fill in the "CISC 320 Game Data" Google Sheet and export it as
// game/resources/data/game_data.csv. This file defines what one row looks like in C++
// (AnimationData) and the table that holds all rows (GameData).
//
// This part has no raylib calls, so it can be tested without opening a window.
// Loading PNGs and drawing them is in asset_manager.h. See README.md in this folder.
#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace rage {
    // Which point of the image is placed at an object's position.
    //   BottomCentre: between the feet; characters and props stand on the ground with it.
    //   TopLeft:      the image's corner; tiles and backgrounds line up on a grid with it.
    //   Centre:       the middle; floating objects and effects.
    enum class Pivot { BottomCentre, TopLeft, Centre };

    // Which way a drawing faces (the sheet's Faces column), or which way an object is
    // currently facing in the game. None means "no direction": never mirrored.
    enum class Facing { None, Right, Left };

    // Draw order, back to front. Count is not a layer; it is the number of layers.
    enum class Layer { Background, Middle, Foreground, Ui, Count };

    // One row of the Game Data sheet, which describes one animation.
    // A static image is just an animation with one frame and no FPS.
    struct AnimationData {
        std::string animationId; // What code asks for, e.g. "player_walk". Unique.
        std::string assetId;     // The object it belongs to, e.g. "player_character".
        std::string fileName;    // PNG name. Empty: no art yet, so a placeholder is drawn.
        int frameWidth = 0;      // Size of ONE frame in pixels (not the whole strip).
        int frameHeight = 0;
        int frames = 1;          // Number of frames in the PNG.
        int framesPerRow = 0;    // For grid layouts. 0 means every frame is in one row.
        float fps = 0;           // Playback speed. 0 means static: always show frame 0.
        bool loops = false;      // true: repeat. false: play once and hold the last frame.
        Pivot pivot = Pivot::BottomCentre;
        Facing faces = Facing::None;
        Layer layer = Layer::Middle;
        bool invisible = false;  // File Name was "none": a gameplay object that is never drawn.
        std::string notes;       // Notes column. For people only; the game ignores it.
        int row = 0;             // Spreadsheet row number, used in warning messages.

        bool IsAnimated() const { return frames > 1 && fps > 0; }

        // Grid shape of the PNG, in frames. A plain strip is `frames` columns by 1 row.
        int Columns() const { return framesPerRow > 0 ? framesPerRow : frames; }
        int Rows() const { return (frames + Columns() - 1) / Columns(); } // Rounds up.
    };

    // A rectangle in pixels. Used for "where is frame N inside the PNG".
    struct FrameRect {
        float x = 0;
        float y = 0;
        float width = 0;
        float height = 0;
    };

    // Where frame `frame` sits inside the PNG. Frames are read left to right, then top to
    // bottom. Out-of-range frame numbers are clamped to the first or last frame.
    FrameRect FrameSource(const AnimationData &animation, int frame);

    // Which frame to show after `seconds` of playback, using the row's FPS and Loops.
    // Non-looping animations stop on their last frame. Static assets always return 0.
    int FrameAt(const AnimationData &animation, double seconds);

    // True once a non-looping animation has played its last frame (e.g. a jump or a fall).
    // Looping animations never finish; static assets count as finished.
    bool IsFinished(const AnimationData &animation, double seconds);

    // Distance from an image's top-left corner to its pivot point, written to `x` and `y`.
    // Example: a 48x80 frame with BottomCentre gives x = 24, y = 80.
    void PivotOffset(Pivot pivot, float width, float height, float &x, float &y);

    // Mirror the image when the art faces one way and the object faces the other.
    // Art with no direction (Facing::None) is never mirrored.
    bool ShouldFlip(Facing art, Facing object);

    // Uniform scale that fits a frame inside a target size without stretching, rounded down so
    // each art pixel covers a whole number of screen pixels. `pixelsPerUnit` is the camera zoom
    // (2 in levels, 1 for UI). Never goes below one screen pixel per art pixel.
    // Example: 16x32 into 30x44 at zoom 2: min(1.875, 1.375) * 2 = 2.75 -> 2 pixels -> scale 1.
    float FitScale(int frameWidth, int frameHeight, float targetWidth, float targetHeight, float pixelsPerUnit = 1);

    // All imported rows, with lookups.
    //
    // Importing never throws. Bad rows are skipped and bad values fall back to defaults;
    // each problem is recorded in Problems() with its sheet row, for the console.
    class GameData {
    public:
        // Reads a CSV file. Returns false if the file is missing or any problem was found.
        bool LoadFile(const std::filesystem::path &path);

        // Parses CSV text. Returns false if any problem was found; valid rows are still kept.
        // `source` is the name shown at the start of each problem message.
        bool Parse(std::string_view csv, std::string_view source = "Game Data");

        void Clear();

        // Lookup by Animation ID, e.g. Find("player_walk"). Returns nullptr if missing.
        // The returned pointer stays valid until the data is cleared or reloaded.
        const AnimationData *Find(std::string_view animationId) const;

        // Lookup by object and state, e.g. Find("player_character", "walk") -> player_walk.
        // Tries "<assetId>_<state>" first, then any animation of that asset whose ID ends
        // in "_<state>". An empty state returns the asset's first row (handy for static art).
        const AnimationData *Find(std::string_view assetId, std::string_view state) const;

        // Every animation of one object, in sheet order.
        std::vector<const AnimationData *> ForAsset(std::string_view assetId) const;

        // Every animation, in sheet order.
        std::vector<const AnimationData *> All() const;

        std::size_t Size() const { return animations.size(); }
        const std::vector<std::string> &Problems() const { return problems; }

    private:
        void Report(std::string message);

        // Rows by Animation ID. unordered_map never moves its elements when it grows,
        // so pointers returned by Find() stay valid while more rows are added.
        std::unordered_map<std::string, AnimationData> animations;
        // Asset ID -> that asset's Animation IDs, in sheet order.
        std::unordered_map<std::string, std::vector<std::string>> assetAnimations;
        std::vector<std::string> order;    // Every Animation ID, in sheet order.
        std::vector<std::string> problems; // Warnings for the console.
        std::string source;                // File name used in warnings.
    };

    // Splits CSV text into rows of cells. Handles the parts of the CSV format that
    // Google Sheets produces: quoted cells containing commas, quotes or line breaks,
    // Windows (CRLF) or Unix (LF) line endings, and an optional UTF-8 byte-order mark.
    std::vector<std::vector<std::string>> ParseCsv(std::string_view text);
}
