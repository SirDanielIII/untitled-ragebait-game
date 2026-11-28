#include "engine/window/game_window.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

#if defined(RAGE_DESKTOP_GLFW)
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#endif

namespace rage {
    RaylibContext::RaylibContext(const GameConfig &config, bool hidden) {
        SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI | (hidden ? FLAG_WINDOW_HIDDEN : 0));
        InitWindow(config.windowWidth, config.windowHeight, "Untitled Ragebait Game");
        if (!IsWindowReady()) throw std::runtime_error("Raylib could not create a window");
        SetWindowMinSize(640, 360);
#if defined(RAGE_DESKTOP_GLFW)
        // Native resize constraints affect dragging, while maximize/fullscreen use the desktop size.
        glfwSetWindowAspectRatio(glfwGetCurrentContext(), canvasWidth, canvasHeight);
        const int monitor = GetCurrentMonitor();
        if (GetRenderWidth() >= GetMonitorWidth(monitor) || GetRenderHeight() >= GetMonitorHeight(monitor))
            MaximizeWindow();
        PollInputEvents();
#endif
        SetExitKey(KEY_NULL);
        SetTargetFPS(hidden ? 0 : config.EffectiveTargetFps());
        InitAudioDevice();
    }

    RaylibContext::~RaylibContext() {
        if (IsAudioDeviceReady()) CloseAudioDevice();
        if (IsWindowReady()) CloseWindow();
    }

    GameWindow::GameWindow(ConfigStore configuration, std::filesystem::path directory,
                           std::filesystem::path settings, bool test)
        : config(std::move(configuration)), settingsPath(std::move(settings)), smokeTest(test),
          context(config.values, test), resources(directory), game(config.values) {
        for (const auto &diagnostic: config.diagnostics) TraceLog(LOG_WARNING, "%s", diagnostic.c_str());
        // Smoke tests begin windowed; the desktop preference is restored on normal startup.
#if !defined(PLATFORM_WEB)
        if (smokeTest) game.SetFullscreenPreference(false);
        else ApplyFullscreenPreference();
#endif
    }

    void GameWindow::ApplyFullscreenPreference() {
#if !defined(PLATFORM_WEB)
        if (config.values.fullscreen == IsWindowFullscreen()) return;
        const bool entering = config.values.fullscreen;
        if (entering) {
            restoreMaximized = IsWindowMaximized();
            // Let raylib remember the restored window rectangle, not the maximized one.
            if (restoreMaximized) {
                RestoreWindow();
                PollInputEvents();
            }
        }
        ToggleFullscreen(); // Raylib selects the current monitor's native video mode.
        if (!IsWindowFullscreen() && restoreMaximized) {
            MaximizeWindow();
            restoreMaximized = false;
        }
        if (config.values.fullscreen != IsWindowFullscreen()) {
            TraceLog(LOG_WARNING, "WINDOW: Fullscreen transition failed");
            game.SetFullscreenPreference(IsWindowFullscreen());
        }
#endif
    }

    Rectangle GameWindow::Destination() const {
        // Always fit the logical 16:9 layout, including non-16:9 window sizes,
        // programmatic resizes and fractional DPI. Never stretch one axis.
        const Rectangle bounds = CanvasDestination(static_cast<float>(GetRenderWidth()),
                                                   static_cast<float>(GetRenderHeight()));
        const auto dpi = WindowPixelScale();
        return {bounds.x / dpi.x, bounds.y / dpi.y, bounds.width / dpi.x, bounds.height / dpi.y};
    }

    Vector2 GameWindow::MouseOnCanvas() const {
        return CanvasCoordinates(GetMousePosition(), Destination());
    }

    void GameWindow::Frame(float dt, const InputFrame &input) {
        const float delta = std::isfinite(dt) ? std::clamp(dt, 0.0f, 0.1f) : 0.0f;
        const int previousFps = config.values.EffectiveTargetFps();
        const bool previousFullscreen = config.values.fullscreen;
        const bool menuAction = game.Update(delta, input);
        if (previousFullscreen != config.values.fullscreen) ApplyFullscreenPreference();
        if (!smokeTest && previousFps != config.values.EffectiveTargetFps()) SetTargetFPS(config.values.EffectiveTargetFps());
        if (IsAudioDeviceReady()) SetMasterVolume(config.values.muted ? 0.0f : config.values.masterVolume);
        // Audio: volumes, pause ducking, the screen's music, then the sounds requested this frame.
        auto &audio = resources.Audio();
        audio.SetVolumes(config.values.musicVolume, config.values.sfxVolume);
        audio.SetPaused(game.IsLevelPaused()); // Before the click, so pausing doesn't cut its own click.
        audio.PlayMusic(game.MusicTrack());
        for (const auto &soundId: game.TakeSoundRequests()) audio.Play(soundId);
        if (menuAction) resources.PlayClick(config.values);
        audio.Update(delta);
        if (game.TakeSettingsDirty() && !smokeTest) {
            preferencesPending = true;
        }
        if (preferencesPending && (!input.mouseDown || game.WantsQuit())) SavePreferences();
        Draw();
    }

    void GameWindow::SavePreferences() {
        const bool settingsSaved = config.SaveFile(settingsPath, ConfigFile::Settings);
        const bool gameSaved = config.SaveFile(settingsPath.parent_path() / "game.yaml", ConfigFile::SaveData);
        const bool saved = settingsSaved && gameSaved;
        if (!saved) TraceLog(LOG_WARNING, "%s", config.diagnostics.back().c_str());
        preferencesPending = false;
    }

    void GameWindow::Draw() {
        const Rectangle destination = Destination();
        const auto dpi = WindowPixelScale();
        const Rectangle physical{destination.x * dpi.x, destination.y * dpi.y,
                                 destination.width * dpi.x, destination.height * dpi.y};
        if (physical.width <= 0 || physical.height <= 0) return;

        BeginDrawing();
        ClearBackground(BLACK);
        game.DrawFrame(resources, physical);
        EndDrawing();
    }
}
