/*******************************************************************************************
*
*   raylib game template
*
*
*   Code licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2021-2026 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

// Altered from the Raylib template: C++ application ownership and foundation entry point.
#include "core/application.h"

#include <exception>
#include <iostream>
#include <string>
#include <utility>

int main(int argc, char** argv)
{
    try
    {
        bool smoke = false;
        std::filesystem::path configPath;
        std::filesystem::path settingsPath;
        std::filesystem::path captures;
        for (int i = 1; i < argc; ++i)
        {
            const std::string argument = argv[i];
            if (argument == "--self-test") return rage::RunSelfTests();
            if (argument == "--smoke-test") smoke = true;
            else if ((argument == "--config" || argument == "--settings" || argument == "--capture-dir") && i + 1 < argc)
            {
                const std::filesystem::path value = argv[++i];
                if (argument == "--config") configPath = value;
                else if (argument == "--settings") settingsPath = value;
                else captures = value;
            }
            else
            {
                std::cerr << "Usage: untitled-ragebait-game [--self-test | --smoke-test] [--config file] [--settings file] [--capture-dir dir]\n";
                return 2;
            }
        }
        const auto resources = rage::FindResourceDirectory();
        if (configPath.empty()) configPath = resources / "config.yaml";
        if (settingsPath.empty()) settingsPath = rage::ConfigStore::DefaultSettingsPath();
        rage::ConfigStore config;
        config.Load(configPath);
        if (!smoke) config.Load(settingsPath, true);
#if defined(PLATFORM_WEB)
        // Desktop remains the verified build; browser persistence needs an IDBFS mount.
        if (smoke)
        {
            rage::Application app(std::move(config), resources, settingsPath, true);
            return app.RunSmokeTest(captures);
        }
        rage::Application::RunBrowser(std::make_unique<rage::Application>(std::move(config), resources, settingsPath, false));
#else
        rage::Application app(std::move(config), resources, settingsPath, smoke);
        if (smoke) return app.RunSmokeTest(captures);
        app.Run();
#endif
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Startup/test error: " << error.what() << '\n';
        return 1;
    }
}
