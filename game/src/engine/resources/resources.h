#pragma once

#include "engine/config/config.h"
#include "engine/graphics/layout.h"
#include "raylib.h"
#include "engine/resources/asset_paths.h"
#include <filesystem>
#include <string>
#include <string_view>

namespace rage {
    std::filesystem::path FindResourceDirectory();

    // Owns font, textures and audio; destroyed before RaylibContext.
    class Resources {
    public:
        explicit Resources(const std::filesystem::path &directory);

        ~Resources();

        Resources(const Resources &) = delete;

        Resources &operator=(const Resources &) = delete;

        Font UiFont() const { return uiFont; }
        bool HasCustomFont() const { return ownsFont; }

        bool DrawArt(ArtId id, Rectangle bounds, ArtFit fit = ArtFit::Contain, Color tint = WHITE) const;

        RenderTexture2D Canvas() const { return canvas; }

        void PlayClick(const GameConfig &config) const;

        void BeginClip(Rectangle logicalBounds) const;

        Vector2 MeasureText(std::string_view text, float size, float spacing = 1) const;

        void DrawText(std::string_view text, Vector2 position, float size, float spacing, Color color) const;

    private:
        RenderTexture2D canvas = {};
        Sound click = {};
        Font uiFont = {};
        bool ownsFont = false;
        std::array<Texture2D, static_cast<std::size_t>(ArtId::Count)> artwork = {};
    };
}
