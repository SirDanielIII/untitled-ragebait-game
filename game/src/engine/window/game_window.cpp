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

        // Copy the canvas to the window so pixel art stays crisp at any window size
        // (see ChoosePresentation in engine/graphics/layout.h).
        const auto canvas = resources.Canvas();
        const Rectangle destination = Destination();
        const Presentation presentation = ChoosePresentation(destination.width * WindowPixelScale().x / canvasWidth);
        // Render textures are stored upside down; a negative source height flips them back.
        const Rectangle canvasSource{0, 0, static_cast<float>(canvas.texture.width), -static_cast<float>(canvas.texture.height)};
        RenderTexture2D upscaled = {};
        if (presentation.mode == PresentMode::SharpUpscale) upscaled = resources.UpscaleTarget(presentation.factor);
        if (upscaled.id != 0) {
            // Step 1: enlarge to a whole multiple with nearest-neighbour (every pixel stays square).
            SetTextureFilter(canvas.texture, TEXTURE_FILTER_POINT);
            BeginTextureMode(upscaled);
            ClearBackground(BLANK);
            BeginBlendMode(BLEND_ALPHA_PREMULTIPLY);
            DrawTexturePro(canvas.texture, canvasSource,
                           {0, 0, static_cast<float>(upscaled.texture.width), static_cast<float>(upscaled.texture.height)},
                           {0, 0}, 0, WHITE);
            EndBlendMode();
            EndTextureMode();
        } else {
            SetTextureFilter(canvas.texture,
                             presentation.mode == PresentMode::Exact ? TEXTURE_FILTER_POINT : TEXTURE_FILTER_BILINEAR);
        }

        BeginDrawing();
        ClearBackground(BLACK);
        // RGB has already been composited into the canvas. Preserve those colors;
        // applying glyph-edge alpha a second time would darken the text.
        BeginBlendMode(BLEND_ALPHA_PREMULTIPLY);
        if (upscaled.id != 0) {
            // Step 2: shrink the enlarged copy to the window with smooth (bilinear) filtering.
            DrawTexturePro(upscaled.texture,
                           {0, 0, static_cast<float>(upscaled.texture.width), -static_cast<float>(upscaled.texture.height)},
                           destination, {0, 0}, 0, WHITE);
        } else {
            DrawTexturePro(canvas.texture, canvasSource, destination, {0, 0}, 0, WHITE);
        }
        EndBlendMode();
        EndDrawing();
    }
}
