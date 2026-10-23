#pragma once

#include "engine/config/config.h"
#include "engine/resources/resources.h"
#include "game.h"
#include <filesystem>

namespace rage
{
// Owns the window, settings, rendering resources and Game, in lifetime order.
// Run/RunBrowser live in main.cpp; frame rendering and persistence live here.
class GameWindow
{
public:
    GameWindow(ConfigStore config, std::filesystem::path resourceDirectory,
                std::filesystem::path settingsPath, bool smokeTest);
    void Run();
    int RunSmokeTest(const std::filesystem::path& captureDirectory = {});
#if defined(PLATFORM_WEB)
    static void RunBrowser(std::unique_ptr<GameWindow> application);
#endif

private:
    void Frame(float dt, const InputFrame& input);
    void Draw();
    void SavePreferences();
    Rectangle Destination() const;
    Vector2 MouseOnCanvas() const;

    ConfigStore config;
    std::filesystem::path settingsPath;
    bool smokeTest;
    // Declaration order is ownership order: game -> resources -> contexts on destruction.
    RaylibContext context;
    Resources resources;
    Game game;
    std::string settingsStatus = "Preferences ready.";
    bool preferencesPending = false;
};

int RunSelfTests();
}
