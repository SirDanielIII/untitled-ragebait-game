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
        if (!config.diagnostics.empty()) settingsStatus = "Some configuration values were ignored; see the console.";
        for (const auto &diagnostic: config.diagnostics) TraceLog(LOG_WARNING, "%s", diagnostic.c_str());
    }

    Rectangle GameWindow::Destination() const {
        Rectangle bounds{0, 0, static_cast<float>(GetRenderWidth()), static_cast<float>(GetRenderHeight())};
#if defined(RAGE_DESKTOP_GLFW)
        if (IsWindowMaximized() || IsWindowFullscreen() || IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE))
#endif
            bounds = CanvasDestination(bounds.width, bounds.height);
        const auto dpi = WindowPixelScale();
        return {bounds.x / dpi.x, bounds.y / dpi.y, bounds.width / dpi.x, bounds.height / dpi.y};
    }

    Vector2 GameWindow::MouseOnCanvas() const {
        return CanvasCoordinates(GetMousePosition(), Destination());
    }

    void GameWindow::Frame(float dt, const InputFrame &input) {
        const float delta = std::isfinite(dt) ? std::clamp(dt, 0.0f, 0.1f) : 0.0f;
        const int previousFps = config.values.EffectiveTargetFps();
        const bool menuAction = game.Update(delta, input);
        if (!smokeTest && previousFps != config.values.EffectiveTargetFps()) SetTargetFPS(config.values.EffectiveTargetFps());
        if (IsAudioDeviceReady()) SetMasterVolume(config.values.muted ? 0.0f : config.values.masterVolume);
        if (menuAction) resources.PlayClick(config.values);
        if (game.TakeSettingsDirty() && !smokeTest) {
            preferencesPending = true;
            settingsStatus = "Changes apply now; drag changes save on release.";
        }
        if (preferencesPending && (!input.mouseDown || game.WantsQuit())) SavePreferences();
        Draw();
    }

    void GameWindow::SavePreferences() {
        const bool saved = config.Save(settingsPath);
        settingsStatus = saved ? "Preferences saved." : "Saving failed; changes still apply this session.";
        if (!saved) TraceLog(LOG_WARNING, "%s", config.diagnostics.back().c_str());
        preferencesPending = false;
    }

    void GameWindow::Draw() {
        BeginTextureMode(resources.Canvas());
        ClearBackground(Color{19, 25, 39, 255});
        game.Draw(resources, settingsStatus);
        if (config.values.showFps) resources.DrawText(TextFormat("%d FPS", GetFPS()), {1410, 18}, 20, 1, RAYWHITE);
        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLACK);
        const auto canvas = resources.Canvas();
        // RGB has already been composited into the canvas. Preserve those colors;
        // applying glyph-edge alpha a second time would darken the text.
        BeginBlendMode(BLEND_ALPHA_PREMULTIPLY);
        DrawTexturePro(canvas.texture, {0, 0, static_cast<float>(canvas.texture.width), -static_cast<float>(canvas.texture.height)},
                       Destination(), {0, 0}, 0, WHITE);
        EndBlendMode();
        EndDrawing();
    }
}
