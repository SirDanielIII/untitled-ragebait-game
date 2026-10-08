#include "application.h"

#include <algorithm>
#include <cmath>
#include <utility>

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

namespace rage
{
Application::Application(ConfigStore configuration, std::filesystem::path directory,
                         std::filesystem::path settings, bool test)
    : config(std::move(configuration)), settingsPath(std::move(settings)), smokeTest(test),
      context(config.values, test), resources(directory), session(config.values)
{
    if (!config.diagnostics.empty()) settingsStatus = "Some configuration values were ignored; see the console.";
    for (const auto& diagnostic : config.diagnostics) TraceLog(LOG_WARNING, "%s", diagnostic.c_str());
}

Rectangle Application::Destination() const
{
    return CanvasDestination(static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight()));
}

Vector2 Application::MouseOnCanvas() const
{
    return CanvasCoordinates(GetMousePosition(), Destination());
}

void Application::Frame(float dt, const InputFrame& input)
{
    const float delta = std::isfinite(dt) ? std::clamp(dt, 0.0f, 0.1f) : 0.0f;
    const int previousFps = config.values.targetFps;
    const bool menuAction = session.Update(delta, input);
    if (!smokeTest && previousFps != config.values.targetFps) SetTargetFPS(config.values.targetFps);
    if (IsAudioDeviceReady()) SetMasterVolume(config.values.muted ? 0.0f : config.values.masterVolume);
    if (menuAction) resources.PlayClick(config.values);
    if (session.TakeSettingsDirty() && !smokeTest)
    {
        preferencesPending = true;
        settingsStatus = "Changes apply now; drag changes save on release.";
    }
    if (preferencesPending && (!input.mouseDown || session.WantsQuit())) SavePreferences();
    Draw();
}

void Application::SavePreferences()
{
    const bool saved = config.Save(settingsPath);
    settingsStatus = saved ? "Preferences saved." : "Saving failed; changes still apply this session.";
    if (!saved) TraceLog(LOG_WARNING, "%s", config.diagnostics.back().c_str());
    preferencesPending = false;
}

void Application::Draw()
{
    BeginTextureMode(resources.Canvas());
    ClearBackground(Color{19, 25, 39, 255});
    session.Draw(resources.UiFont(), settingsStatus);
    if (config.values.showFps) DrawFPS(canvasWidth - 125, 18);
    EndTextureMode();

    BeginDrawing();
    ClearBackground(BLACK);
    const auto canvas = resources.Canvas();
    DrawTexturePro(canvas.texture, {0, 0, static_cast<float>(canvasWidth), -static_cast<float>(canvasHeight)},
                   Destination(), {0, 0}, 0, WHITE);
    EndDrawing();
}

void Application::Run()
{
    while (!session.WantsQuit() && !WindowShouldClose())
    {
        InputFrame input = CaptureInput(MouseOnCanvas());
        if (!IsWindowFocused() && session.Screen() == ScreenId::Playing) input.pause = true;
        Frame(GetFrameTime(), input);
    }
    if (preferencesPending) SavePreferences();
}

#if defined(PLATFORM_WEB)
void Application::RunBrowser(std::unique_ptr<Application> application)
{
    // This owner outlives main and releases all handles when the callback loop ends.
    static std::unique_ptr<Application> owner;
    owner = std::move(application);
    emscripten_set_main_loop_arg([](void* pointer)
    {
        auto& instance = *static_cast<std::unique_ptr<Application>*>(pointer);
        if (instance->session.WantsQuit() || WindowShouldClose())
        {
            emscripten_cancel_main_loop();
            if (instance->preferencesPending) instance->SavePreferences();
            instance.reset();
            return;
        }
        instance->Frame(GetFrameTime(), CaptureInput(instance->MouseOnCanvas()));
    }, &owner, 0, 0);
}
#endif
}
