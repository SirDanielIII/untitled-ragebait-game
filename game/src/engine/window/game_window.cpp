#include "engine/window/game_window.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace rage
{
GameWindow::GameWindow(ConfigStore configuration, std::filesystem::path directory,
                         std::filesystem::path settings, bool test)
    : config(std::move(configuration)), settingsPath(std::move(settings)), smokeTest(test),
      context(config.values, test), resources(directory), game(config.values)
{
    if (!config.diagnostics.empty()) settingsStatus = "Some configuration values were ignored; see the console.";
    for (const auto& diagnostic : config.diagnostics) TraceLog(LOG_WARNING, "%s", diagnostic.c_str());
}

Rectangle GameWindow::Destination() const
{
    // Snap the letterbox to actual framebuffer pixels, including HiDPI output.
    const auto dpi = WindowPixelScale();
    const float pixelRatioX = dpi.x;
    const float pixelRatioY = dpi.y;
    const auto physical = CanvasDestination(static_cast<float>(GetRenderWidth()), static_cast<float>(GetRenderHeight()));
    const float left = std::round(physical.x);
    const float top = std::round(physical.y);
    return {left / pixelRatioX, top / pixelRatioY,
            (std::round(physical.x + physical.width) - left) / pixelRatioX,
            (std::round(physical.y + physical.height) - top) / pixelRatioY};
}

Vector2 GameWindow::MouseOnCanvas() const
{
    return CanvasCoordinates(GetMousePosition(), Destination());
}

void GameWindow::Frame(float dt, const InputFrame& input)
{
    const float delta = std::isfinite(dt) ? std::clamp(dt, 0.0f, 0.1f) : 0.0f;
    const int previousFps = config.values.EffectiveTargetFps();
    const bool menuAction = game.Update(delta, input);
    if (!smokeTest && previousFps != config.values.EffectiveTargetFps()) SetTargetFPS(config.values.EffectiveTargetFps());
    if (IsAudioDeviceReady()) SetMasterVolume(config.values.muted ? 0.0f : config.values.masterVolume);
    if (menuAction) resources.PlayClick(config.values);
    if (game.TakeSettingsDirty() && !smokeTest)
    {
        preferencesPending = true;
        settingsStatus = "Changes apply now; drag changes save on release.";
    }
    if (preferencesPending && (!input.mouseDown || game.WantsQuit())) SavePreferences();
    Draw();
}

void GameWindow::SavePreferences()
{
    const bool saved = config.Save(settingsPath);
    settingsStatus = saved ? "Preferences saved." : "Saving failed; changes still apply this session.";
    if (!saved) TraceLog(LOG_WARNING, "%s", config.diagnostics.back().c_str());
    preferencesPending = false;
}

void GameWindow::Draw()
{
    const auto destination = Destination();
    const auto dpi = WindowPixelScale();
    const int width = static_cast<int>(std::round(destination.width * dpi.x));
    const int height = static_cast<int>(std::round(destination.height * dpi.y));
    resources.ResizeCanvas(width, height);
    BeginTextureMode(resources.Canvas());
    resources.BeginCanvasCoordinates();
    ClearBackground(Color{19, 25, 39, 255});
    game.Draw(resources, settingsStatus);
    if (config.values.showFps) resources.DrawText(TextFormat("%d FPS", GetFPS()), {1410, 18}, 20, 1, RAYWHITE);
    EndMode2D();
    EndTextureMode();

    BeginDrawing();
    ClearBackground(BLACK);
    const auto canvas = resources.Canvas();
    // RGB has already been composited into the canvas. Preserve those colors;
    // applying glyph-edge alpha a second time would darken the text.
    BeginBlendMode(BLEND_ALPHA_PREMULTIPLY);
    DrawTexturePro(canvas.texture, {0, 0, static_cast<float>(canvas.texture.width), -static_cast<float>(canvas.texture.height)},
                   destination, {0, 0}, 0, WHITE);
    EndBlendMode();
    EndDrawing();
}

}
