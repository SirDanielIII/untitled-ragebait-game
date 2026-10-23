#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace rage
{
inline constexpr int minTargetFps = 30;
inline constexpr int maxTargetFps = 360;
struct GameConfig
{
    int windowWidth = 1600;
    int windowHeight = 900;
    int targetFps = 60;
    bool unlimitedFps = false;
    bool showFps = false;
    float masterVolume = 0.5f;
    bool muted = false;
    float moveSpeed = 260.0f;
    float jumpSpeed = 620.0f;
    float gravity = 1500.0f;
    float charactersPerSecond = 40.0f;
    int lastLevel = 0;
    int EffectiveTargetFps() const { return unlimitedFps ? 0 : targetFps; }
};

// A deliberately small YAML subset: section maps containing plain number/bool scalars.
// Load overlays valid keys onto the current values; bad/missing keys keep their defaults.
class ConfigStore
{
public:
    GameConfig values;
    std::vector<std::string> diagnostics;
    bool Load(const std::filesystem::path& path, bool optional = false);
    bool Save(const std::filesystem::path& path);
    static std::filesystem::path DefaultSettingsPath();
};
}
