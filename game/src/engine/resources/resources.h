#pragma once

#include "engine/assets/asset_manager.h"
#include "engine/audio/audio_manager.h"
#include "engine/config/config.h"
#include "engine/graphics/layout.h"
#include "raylib.h"
#include "engine/resources/asset_paths.h"
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <utility>
#include <cstdint>

namespace rage {
    std::filesystem::path FindResourceDirectory();

    enum class FontRole { Regular, Heading, Typewriter };

    // Owns font, textures and audio; destroyed before RaylibContext.
    class Resources {
    public:
        explicit Resources(const std::filesystem::path &directory);

        ~Resources();

        Resources(const Resources &) = delete;

        Resources &operator=(const Resources &) = delete;

        Font UiFont() const { return ResolveFont(FontRole::Regular, 24, 1); }
        bool HasCustomFont(FontRole role = FontRole::Regular) const;
        // The full frame uses logical coordinates mapped to physical framebuffer pixels.
        // Set once per frame before drawing the world and UI.
        void BeginFrame();
        void SetViewport(Rectangle physicalDestination);
        std::size_t CachedFontCount() const;
        std::uint64_t FontBuildCount() const { return fontBuildCount; }

        bool DrawArt(ArtId id, Rectangle bounds, ArtFit fit = ArtFit::Contain, Color tint = WHITE) const;

        // Sprites and animations described by data/game_data.csv.
        const AssetManager &Assets() const { return assets; }

        // Sound effects and music described by data/audio_data.csv.
        AudioManager &Audio() { return audio; }
        const AudioManager &Audio() const { return audio; }

        // Plays the menu click (the "ui_click" Sound ID) unless audio is muted.
        void PlayClick(const GameConfig &config);

        void BeginClip(Rectangle logicalBounds) const;

        Vector2 MeasureText(std::string_view text, float size, float spacing = 1,
                            FontRole role = FontRole::Regular) const;

        void DrawText(std::string_view text, Vector2 position, float size, float spacing, Color color,
                      FontRole role = FontRole::Regular) const;

        // Center the visible glyphs (not the advance box) in logical bounds.
        void DrawCenteredText(std::string_view text, Rectangle bounds, float size, float spacing,
                              Color color, FontRole role = FontRole::Regular) const;

    private:
        struct CachedFont { Font font{}; std::uint64_t lastUsed = 0; };
        struct Face {
            std::vector<unsigned char> bytes;
            mutable std::unordered_map<int, CachedFont> sizes;
            mutable std::vector<std::pair<int, float>> logicalAdvances; // codepoint, normalized advance
        };
        std::array<Face, 3> faces;
        Rectangle viewport{0, 0, canvasWidth, canvasHeight};
        Vector2 viewportScale{1, 1};
        std::uint64_t frameNumber = 0;
        mutable std::uint64_t fontBuildCount = 0;
        Font ResolveFont(FontRole role, float size, float physicalScale) const;
        const std::vector<std::pair<int, float>> &LogicalAdvances(FontRole role) const;
        Vector2 LayoutText(std::string_view text, Vector2 position, float size, float spacing,
                           Color color, FontRole role, bool draw, Rectangle *inkBounds = nullptr) const;
        std::array<Texture2D, static_cast<std::size_t>(ArtId::Count)> artwork = {};
        AssetManager assets;
        AudioManager audio; // Unloaded with Resources, before the audio device closes.
    };
}
