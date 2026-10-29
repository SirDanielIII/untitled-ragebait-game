// Start here: level registration, screen transitions and the game loop.
// Each level's layout and behavior live in levels/level_<number>/level_<number>.cpp.
#include "game.h"
#include "engine/window/game_window.h"
#include "ui/menus/menus.h"

#include <algorithm>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

namespace rage {
    // Add new levels to this list. The selector and factory use the same entries.
    namespace {
        struct Registration {
            const LevelDefinition & (*definition)();

            std::unique_ptr<Level> (*create)();
        };

        // Register new stages here; content and behavior stay in their own files.
        const Registration registrations[] = {
            {Level0Definition, MakeLevel0},
            {Level1Definition, MakeLevel1},
            {Level2Definition, MakeLevel2},
            {Level3Definition, MakeLevel3}
        };
    }

    const std::vector<LevelDefinition> &LevelCatalog() {
        static const auto levels = [] {
            std::vector<LevelDefinition> result;
            for (const auto &entry: registrations) result.push_back(entry.definition());
            return result;
        }();
        return levels;
    }

    const LevelDefinition &FindLevel(LevelId id) {
        const auto &levels = LevelCatalog();
        const auto found = std::find_if(levels.begin(), levels.end(), [id](const auto &level) { return level.id == id; });
        if (found == levels.end()) throw std::invalid_argument("Unknown level id");
        return *found;
    }

    std::unique_ptr<Level> CreateLevel(LevelId id, const GameConfig &config) {
        for (const auto &entry: registrations) {
            if (entry.definition().id != id) continue;
            auto activeLevel = entry.create();
            activeLevel->Reset(config); // Virtual hooks run after full construction.
            return activeLevel;
        }
        throw std::invalid_argument("Unknown level id");
    }

    // Screen and level handler. Pause and Settings keep the current level alive.
    Game::Game(GameConfig &config) : config(config) { BuildMenu(); }

    void Game::Enter(ScreenId next) {
        screen = next;
        if (screen == ScreenId::MainMenu || screen == ScreenId::LevelSelect) activeLevel.reset();
        BuildMenu();
    }

    void Game::BuildMenu(bool preserveFocus) {
        std::vector<MenuItem> items;
        switch (screen) {
            case ScreenId::MainMenu: items = MainMenuItems();
                break;
            case ScreenId::LevelSelect: items = LevelSelectItems();
                break;
            case ScreenId::Settings: items = SettingsMenuItems(config);
                break;
            case ScreenId::Paused: items = PauseMenuItems();
                break;
            case ScreenId::Playing: break;
        }
        menu.SetItems(std::move(items), preserveFocus);
    }

    bool Game::Update(float dt, const InputFrame &input) {
        if (screen == ScreenId::Playing) {
            if (input.pause) {
                Enter(ScreenId::Paused);
                return true;
            }
            if (activeLevel) activeLevel->Update(dt, input, config);
            return false;
        }
        if (screen == ScreenId::Paused && (input.pause || input.back)) {
            Enter(ScreenId::Playing);
            return true;
        }
        if (input.back) {
            if (screen == ScreenId::Settings) Enter(settingsReturn);
            else if (screen == ScreenId::LevelSelect) Enter(ScreenId::MainMenu);
            return true;
        }
        const auto action = menu.Update(input);
        if (!action) return false;
        Apply(*action);
        // Continuous slider changes are silent; ordinary button actions play one click.
        return action->kind != ActionKind::SetVolume && action->kind != ActionKind::SetDialogueSpeed
               && action->kind != ActionKind::SetTargetFps;
    }

    void Game::Apply(Action action) {
        switch (action.kind) {
            case ActionKind::StartLevel:
                activeLevel = CreateLevel(static_cast<LevelId>(action.value), config);
                config.lastLevel = action.value;
                settingsDirty = true;
                Enter(ScreenId::Playing);
                return;
            case ActionKind::OpenLevels: Enter(ScreenId::LevelSelect);
                return;
            case ActionKind::OpenSettings: settingsReturn = screen;
                Enter(ScreenId::Settings);
                return;
            case ActionKind::Back: Enter(settingsReturn);
                return;
            case ActionKind::MainMenu: Enter(ScreenId::MainMenu);
                return;
            case ActionKind::Quit: quit = true;
                return;
            case ActionKind::Resume: Enter(ScreenId::Playing);
                return;
            case ActionKind::Restart: if (activeLevel) activeLevel->Reset(config);
                Enter(ScreenId::Playing);
                return;
            case ActionKind::ToggleFps: config.showFps = !config.showFps;
                break;
            case ActionKind::ToggleMute: config.muted = !config.muted;
                break;
            case ActionKind::ToggleUnlimitedFps: config.unlimitedFps = !config.unlimitedFps;
                break;
            case ActionKind::SetTargetFps:
                config.targetFps = std::clamp(static_cast<int>(action.scalar), minTargetFps, maxTargetFps);
                settingsDirty = true;
                return; // Keep capture for the entire drag; Unlimited retains the saved cap.
            case ActionKind::SetVolume:
                config.masterVolume = std::clamp(action.scalar, 0.0f, 1.0f);
                settingsDirty = true;
                return; // Retain the widget's drag capture and keyboard focus.
            case ActionKind::SetDialogueSpeed:
                config.charactersPerSecond = std::clamp(action.scalar, 0.0f, 240.0f);
                settingsDirty = true;
                return;
        }
        settingsDirty = true;
        BuildMenu(true);
    }

    bool Game::TakeSettingsDirty() {
        const bool dirty = settingsDirty;
        settingsDirty = false;
        return dirty;
    }

    void Game::Draw(const Resources &resources, const std::string &settingsStatus) const {
        switch (screen) {
            case ScreenId::Playing:
                if (activeLevel) activeLevel->Draw(resources);
                return;
            case ScreenId::MainMenu: DrawMainMenu(resources);
                break;
            case ScreenId::Settings: DrawSettingsMenu(resources, config, settingsStatus);
                break;
            case ScreenId::LevelSelect: DrawLevelSelect(resources, menu.Selected());
                break;
            case ScreenId::Paused:
                if (activeLevel) {
                    activeLevel->Draw(resources);
                    DrawPauseMenu(resources, activeLevel->Definition());
                }
                break;
        }
        menu.Draw(resources);
    }

    // Desktop and browser frame loops. Window/rendering details live in engine/.
    void GameWindow::Run() {
        while (!game.WantsQuit() && !WindowShouldClose()) {
            InputFrame input = CaptureInput(MouseOnCanvas());
            if (!IsWindowFocused() && game.Screen() == ScreenId::Playing) input.pause = true;
            Frame(GetFrameTime(), input);
        }
        if (preferencesPending) SavePreferences();
    }

#if defined(PLATFORM_WEB)
    void GameWindow::RunBrowser(std::unique_ptr<GameWindow> application) {
        // This owner outlives main and releases all handles when the callback loop ends.
        static std::unique_ptr<GameWindow> owner;
        owner = std::move(application);
        emscripten_set_main_loop_arg([](void *pointer) {
            auto &instance = *static_cast<std::unique_ptr<GameWindow> *>(pointer);
            if (instance->game.WantsQuit() || WindowShouldClose()) {
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

// Startup and command-line test options.
int main(int argc, char **argv) {
    try {
        bool smoke = false;
        std::filesystem::path configPath;
        std::filesystem::path settingsPath;
        std::filesystem::path captures;
        for (int i = 1; i < argc; ++i) {
            const std::string argument = argv[i];
            if (argument == "--self-test") return rage::RunSelfTests();
            if (argument == "--smoke-test") smoke = true;
            else if ((argument == "--config" || argument == "--settings" || argument == "--capture-dir") && i + 1 < argc) {
                const std::filesystem::path value = argv[++i];
                if (argument == "--config") configPath = value;
                else if (argument == "--settings") settingsPath = value;
                else captures = value;
            } else {
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
        if (smoke) {
            rage::GameWindow app(std::move(config), resources, settingsPath, true);
            return app.RunSmokeTest(captures);
        }
        rage::GameWindow::RunBrowser(std::make_unique<rage::GameWindow>(std::move(config), resources, settingsPath, false));
#else
        rage::GameWindow app(std::move(config), resources, settingsPath, smoke);
        if (smoke) return app.RunSmokeTest(captures);
        app.Run();
#endif
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "Startup/test error: " << error.what() << '\n';
        return 1;
    }
}
