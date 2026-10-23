#include "engine/resources/resources.h"

#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <vector>

namespace rage
{
std::filesystem::path FindResourceDirectory()
{
    const std::vector<std::filesystem::path> candidates = {
        std::filesystem::path(GetApplicationDirectory()) / "resources",
        std::filesystem::path(GetApplicationDirectory()).parent_path() / "resources",
        "resources", "game/resources"
    };
    for (const auto& candidate : candidates)
    {
        std::error_code error;
        if (std::filesystem::is_directory(candidate, error)) return candidate;
    }
    return candidates.front(); // Loaders diagnose missing files and retain defaults.
}

RaylibContext::RaylibContext(const GameConfig& config, bool hidden)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI | (hidden ? FLAG_WINDOW_HIDDEN : 0));
    InitWindow(config.windowWidth, config.windowHeight, "Untitled Ragebait Game");
    if (!IsWindowReady()) throw std::runtime_error("Raylib could not create a window");
    SetWindowMinSize(640, 480);
    SetExitKey(KEY_NULL); // Esc belongs to navigation/pause, not unconditional shutdown.
    SetTargetFPS(hidden ? 0 : config.EffectiveTargetFps());
    InitAudioDevice(); // Audio failure is nonfatal; Resources checks readiness.
}

RaylibContext::~RaylibContext()
{
    if (IsAudioDeviceReady()) CloseAudioDevice();
    if (IsWindowReady()) CloseWindow();
}

Resources::Resources(const std::filesystem::path& directory)
{
    canvas = LoadRenderTexture(canvasWidth, canvasHeight);
    if (canvas.id == 0) throw std::runtime_error("Could not allocate the game canvas");
    SetTextureFilter(canvas.texture, TEXTURE_FILTER_POINT);
    uiFont = GetFontDefault();
    const auto fontPath = directory / uiFontPath;
    if (FileExists(fontPath.string().c_str()))
    {
        int byteCount = 0;
        unsigned char* bytes = LoadFileData(fontPath.string().c_str(), &byteCount);
        if (bytes && byteCount > 0) fontBytes.assign(bytes, bytes + byteCount);
        if (bytes) UnloadFileData(bytes);
        fontExtension = fontPath.extension().string();
        const Font loaded = fontBytes.empty() ? GetFontDefault() :
            LoadFontFromMemory(fontExtension.c_str(), fontBytes.data(), static_cast<int>(fontBytes.size()), 24, nullptr, 0);
        if (IsFontValid(loaded) && loaded.texture.id != GetFontDefault().texture.id)
        {
            uiFont = loaded;
            ownsFont = true;
            SetTextureFilter(uiFont.texture, TEXTURE_FILTER_POINT);
        }
    }
    if (!ownsFont) TraceLog(LOG_WARNING, "Custom font unavailable: %s; using default font", fontPath.string().c_str());
    for (std::size_t i = 0; i < artwork.size(); ++i)
    {
        const auto path = directory / artPaths[i];
        if (FileExists(path.string().c_str())) artwork[i] = LoadTexture(path.string().c_str());
    }
    const auto clickPath = directory / "coin.wav";
    if (IsAudioDeviceReady() && FileExists(clickPath.string().c_str())) click = LoadSound(clickPath.string().c_str());
}

Resources::~Resources()
{
    ClearFontCache();
    for (const auto texture : artwork) if (IsTextureValid(texture)) UnloadTexture(texture);
    if (ownsFont) UnloadFont(uiFont);
    if (IsSoundValid(click)) UnloadSound(click);
    if (canvas.id != 0) UnloadRenderTexture(canvas);
}

void Resources::ClearFontCache()
{
    for (const auto& [size, font] : fontCache)
        if (font.texture.id != GetFontDefault().texture.id) UnloadFont(font);
    fontCache.clear();
}

void Resources::ResizeCanvas(int pixelWidth, int pixelHeight)
{
    pixelWidth = std::max(1, pixelWidth);
    pixelHeight = std::max(1, pixelHeight);
    if (canvas.texture.width == pixelWidth && canvas.texture.height == pixelHeight) return;
    const auto next = LoadRenderTexture(pixelWidth, pixelHeight);
    if (next.id == 0) throw std::runtime_error("Could not resize the game canvas");
    UnloadRenderTexture(canvas);
    canvas = next;
    SetTextureFilter(canvas.texture, TEXTURE_FILTER_POINT);
    renderScale = std::min(static_cast<float>(pixelWidth) / canvasWidth, static_cast<float>(pixelHeight) / canvasHeight);
    ClearFontCache();
}

void Resources::BeginCanvasCoordinates() const
{
    Camera2D logical = {};
    logical.zoom = renderScale;
    BeginMode2D(logical);
}

Camera2D Resources::OutputCamera(Camera2D logicalCamera) const
{
    logicalCamera.offset.x *= renderScale;
    logicalCamera.offset.y *= renderScale;
    logicalCamera.zoom *= renderScale;
    return logicalCamera;
}

void Resources::BeginClip(Rectangle bounds) const
{
    const int left = static_cast<int>(std::round(bounds.x * renderScale));
    const int top = static_cast<int>(std::round(bounds.y * renderScale));
    const int right = static_cast<int>(std::round((bounds.x + bounds.width) * renderScale));
    const int bottom = static_cast<int>(std::round((bounds.y + bounds.height) * renderScale));
    BeginScissorMode(left, top, std::max(0, right - left), std::max(0, bottom - top));
}

Font Resources::FontAtSize(int pixelSize) const
{
    if (!ownsFont) return uiFont;
    if (pixelSize == uiFont.baseSize) return uiFont;
    if (const auto found = fontCache.find(pixelSize); found != fontCache.end()) return found->second;
    Font font = LoadFontFromMemory(fontExtension.c_str(), fontBytes.data(), static_cast<int>(fontBytes.size()), pixelSize, nullptr, 0);
    if (!IsFontValid(font)) font = GetFontDefault();
    SetTextureFilter(font.texture, TEXTURE_FILTER_POINT);
    fontCache.emplace(pixelSize, font);
    return font;
}

Vector2 Resources::MeasureText(std::string_view text, float size, float spacing) const
{
    const int pixels = std::max(1, static_cast<int>(std::round(size * renderScale)));
    const Font font = FontAtSize(pixels);
    const auto measured = MeasureTextEx(font, std::string(text).c_str(), static_cast<float>(pixels), std::round(spacing * renderScale));
    return {measured.x / renderScale, measured.y / renderScale};
}

void Resources::DrawText(std::string_view text, Vector2 position, float size, float spacing, Color color) const
{
    if (text.empty()) return;
    const int pixels = std::max(1, static_cast<int>(std::round(size * renderScale)));
    const Font font = FontAtSize(pixels);
    // Draw glyphs in framebuffer pixels, bypassing the logical layout transform.
    // The atlas and destination are the same size; neither is resampled.
    Camera2D native = {};
    native.zoom = 1;
    BeginMode2D(native);
    DrawTextEx(font, std::string(text).c_str(),
               {std::round(position.x * renderScale), std::round(position.y * renderScale)},
               static_cast<float>(pixels), std::round(spacing * renderScale), color);
    BeginCanvasCoordinates();
}

bool Resources::DrawArt(ArtId id, Rectangle bounds, ArtFit fit, Color tint) const
{
    const auto index = static_cast<std::size_t>(id);
    if (index >= artwork.size()) return false;
    const Texture2D texture = artwork[index];
    if (!IsTextureValid(texture)) return false;
    if (fit == ArtFit::Contain)
    {
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

void Resources::PlayClick(const GameConfig& config) const
{
    if (!IsSoundValid(click) || config.muted) return;
    SetSoundVolume(click, 1.0f); // Global master volume is applied by GameWindow each frame.
    PlaySound(click);
}
}
