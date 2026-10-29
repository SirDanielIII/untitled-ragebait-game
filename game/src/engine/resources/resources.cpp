#include "engine/resources/resources.h"

#include <stdexcept>
#include <algorithm>
#include <vector>

namespace rage {
    std::filesystem::path FindResourceDirectory() {
        const std::vector<std::filesystem::path> candidates = {
            std::filesystem::path(GetApplicationDirectory()) / "resources",
            std::filesystem::path(GetApplicationDirectory()).parent_path() / "resources",
            "resources", "game/resources"
        };
        for (const auto &candidate: candidates) {
            std::error_code error;
            if (std::filesystem::is_directory(candidate, error)) return candidate;
        }
        return candidates.front(); // Loaders diagnose missing files and retain defaults.
    }

    Resources::Resources(const std::filesystem::path &directory) {
        canvas = LoadRenderTexture(canvasWidth, canvasHeight);
        if (canvas.id == 0) throw std::runtime_error("Could not allocate the game canvas");
        SetTextureFilter(canvas.texture, TEXTURE_FILTER_BILINEAR);
        uiFont = GetFontDefault();
        const auto fontPath = directory / uiFontPath;
        if (FileExists(fontPath.string().c_str())) {
            const Font loaded = LoadFontEx(fontPath.string().c_str(), 64, nullptr, 0);
            if (IsFontValid(loaded) && loaded.texture.id != GetFontDefault().texture.id) {
                uiFont = loaded;
                ownsFont = true;
                SetTextureFilter(uiFont.texture, TEXTURE_FILTER_BILINEAR);
            }
        }
        if (!ownsFont) TraceLog(LOG_WARNING, "Custom font unavailable: %s; using default font", fontPath.string().c_str());
        for (std::size_t i = 0; i < artwork.size(); ++i) {
            const auto path = directory / artPaths[i];
            if (FileExists(path.string().c_str())) artwork[i] = LoadTexture(path.string().c_str());
        }
        const auto clickPath = directory / "coin.wav";
        if (IsAudioDeviceReady() && FileExists(clickPath.string().c_str())) click = LoadSound(clickPath.string().c_str());
    }

    Resources::~Resources() {
        for (const auto texture: artwork) if (IsTextureValid(texture)) UnloadTexture(texture);
        if (ownsFont) UnloadFont(uiFont);
        if (IsSoundValid(click)) UnloadSound(click);
        if (canvas.id != 0) UnloadRenderTexture(canvas);
    }

    void Resources::BeginClip(Rectangle bounds) const {
        BeginScissorMode(static_cast<int>(bounds.x), static_cast<int>(bounds.y),
                         static_cast<int>(bounds.width), static_cast<int>(bounds.height));
    }

    Vector2 Resources::MeasureText(std::string_view text, float size, float spacing) const {
        return MeasureTextEx(uiFont, std::string(text).c_str(), size, spacing);
    }

    void Resources::DrawText(std::string_view text, Vector2 position, float size, float spacing, Color color) const {
        DrawTextEx(uiFont, std::string(text).c_str(), position, size, spacing, color);
    }

    bool Resources::DrawArt(ArtId id, Rectangle bounds, ArtFit fit, Color tint) const {
        const auto index = static_cast<std::size_t>(id);
        if (index >= artwork.size()) return false;
        const Texture2D texture = artwork[index];
        if (!IsTextureValid(texture)) return false;
        if (fit == ArtFit::Contain) {
            const float scale = std::min(bounds.width / texture.width, bounds.height / texture.height);
            const Vector2 size{texture.width * scale, texture.height * scale};
            bounds.x += (bounds.width - size.x) / 2;
            bounds.y += bounds.height - size.y; // Bottom aligned: feet stay on the ground.
            bounds.width = size.x;
            bounds.height = size.y;
        }
        DrawTexturePro(texture, {0, 0, static_cast<float>(texture.width), static_cast<float>(texture.height)},
                       bounds, {0, 0}, 0, tint);
        return true;
    }

    void Resources::PlayClick(const GameConfig &config) const {
        if (!IsSoundValid(click) || config.muted) return;
        SetSoundVolume(click, 1.0f); // Global master volume is applied by GameWindow each frame.
        PlaySound(click);
    }
}
