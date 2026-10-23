#pragma once

#include "engine/config/config.h"
#include "engine/graphics/layout.h"
#include "raylib.h"
#include "engine/resources/asset_paths.h"
#include <filesystem>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace rage
{
std::filesystem::path FindResourceDirectory();

class RaylibContext
{
public:
    RaylibContext(const GameConfig& config, bool hidden);
    ~RaylibContext();
    RaylibContext(const RaylibContext&) = delete;
    RaylibContext& operator=(const RaylibContext&) = delete;
};

// Owns font, textures and audio; destroyed before RaylibContext.
class Resources
{
public:
    explicit Resources(const std::filesystem::path& directory);
    ~Resources();
    Resources(const Resources&) = delete;
    Resources& operator=(const Resources&) = delete;
    Font UiFont() const { return uiFont; }
    bool HasCustomFont() const { return ownsFont; }
    bool DrawArt(ArtId id, Rectangle bounds, ArtFit fit = ArtFit::Contain, Color tint = WHITE) const;
    RenderTexture2D Canvas() const { return canvas; }
    void PlayClick(const GameConfig& config) const;

    void ResizeCanvas(int pixelWidth, int pixelHeight);
    float RenderScale() const { return renderScale; }
    void BeginCanvasCoordinates() const;
    Camera2D OutputCamera(Camera2D logicalCamera) const;
    void BeginClip(Rectangle logicalBounds) const;
    Vector2 MeasureText(std::string_view text, float size, float spacing = 1) const;
    void DrawText(std::string_view text, Vector2 position, float size, float spacing, Color color) const;

private:
    Font FontAtSize(int pixelSize) const;
    void ClearFontCache();
    RenderTexture2D canvas = {};
    float renderScale = 1;
    std::vector<unsigned char> fontBytes;
    std::string fontExtension;
    mutable std::map<int, Font> fontCache;
    Sound click = {};
    Font uiFont = {};
    bool ownsFont = false;
    std::array<Texture2D, static_cast<std::size_t>(ArtId::Count)> artwork = {};
};
}
