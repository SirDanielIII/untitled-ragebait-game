#pragma once

#include "config.h"
#include "resources.h"
#include "../screens/game_session.h"
#include <filesystem>

namespace rage
{
class Application
{
public:
    Application(ConfigStore config, std::filesystem::path resourceDirectory,
                std::filesystem::path settingsPath, bool smokeTest);
    void Run();
    int RunSmokeTest(const std::filesystem::path& captureDirectory = {});
#if defined(PLATFORM_WEB)
    static void RunBrowser(std::unique_ptr<Application> application);
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
    // Declaration order is ownership order: session -> resources -> contexts on destruction.
    RaylibContext context;
    Resources resources;
    GameSession session;
    std::string settingsStatus = "Preferences ready.";
    bool preferencesPending = false;
};

int RunSelfTests();
}
