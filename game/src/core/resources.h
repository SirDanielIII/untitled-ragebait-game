#pragma once

#include "config.h"
#include "layout.h"
#include "raylib.h"
#include <filesystem>

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

// Owns GPU/audio handles; destroyed before RaylibContext. Default font is borrowed.
class Resources
{
public:
    explicit Resources(const std::filesystem::path& directory);
    ~Resources();
    Resources(const Resources&) = delete;
    Resources& operator=(const Resources&) = delete;
    Font UiFont() const { return GetFontDefault(); }
    RenderTexture2D Canvas() const { return canvas; }
    void PlayClick(const GameConfig& config) const;

private:
    RenderTexture2D canvas = {};
    Sound click = {};
};
}
