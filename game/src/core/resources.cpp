#include "resources.h"

#include <stdexcept>
#include <vector>

namespace rage
{
std::filesystem::path FindResourceDirectory()
{
    const std::vector<std::filesystem::path> candidates = {
        std::filesystem::path(GetApplicationDirectory()) / "resources",
        "resources", "game/src/resources", "src/resources"
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
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | (hidden ? FLAG_WINDOW_HIDDEN : 0));
    InitWindow(config.windowWidth, config.windowHeight, "Untitled Ragebait Game");
    if (!IsWindowReady()) throw std::runtime_error("Raylib could not create a window");
    SetWindowMinSize(640, 480);
    SetExitKey(KEY_NULL); // Esc belongs to navigation/pause, not unconditional shutdown.
    SetTargetFPS(hidden ? 0 : config.targetFps);
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
    const auto clickPath = directory / "coin.wav";
    if (IsAudioDeviceReady() && FileExists(clickPath.string().c_str())) click = LoadSound(clickPath.string().c_str());
}

Resources::~Resources()
{
    if (IsSoundValid(click)) UnloadSound(click);
    if (canvas.id != 0) UnloadRenderTexture(canvas);
}

void Resources::PlayClick(const GameConfig& config) const
{
    if (!IsSoundValid(click) || config.muted) return;
    SetSoundVolume(click, 1.0f); // Global master volume is applied by Application each frame.
    PlaySound(click);
}
}
