#pragma once

#include "engine/config/config.h"
#include "levels/level.h"
#include "ui/menu/menu.h"
#include <memory>
#include <string>
#include <vector>

namespace rage {
    enum class ScreenId { MainMenu, LevelSelect, Settings, Playing, Paused };

    // The level handler, implemented in main.cpp. Owns navigation and the active level.
    // Can also run without a window so navigation and gameplay can be tested.
    class Game {
    public:
        explicit Game(GameConfig &config);

        bool Update(float dt, const InputFrame &input); // true for discrete menu click feedback
        // Draw directly to the active framebuffer, using a physical-pixel destination.
        void DrawFrame(Resources &resources, Rectangle physical) const;
        bool HasVisibleWorld() const;
        void DrawScene(const Resources &resources) const;
        void DrawUi(const Resources &resources) const;

        ScreenId Screen() const { return screen; }
        const Level *ActiveLevel() const { return activeLevel.get(); }
        const Menu &Navigation() const { return menu; }
        bool WantsQuit() const { return quit; }

        bool TakeSettingsDirty();
        void SetFullscreenPreference(bool enabled);

        // Sound IDs the active level requested since the last call (none outside a level).
        std::vector<std::string> TakeSoundRequests();
        // Music for the current screen: the level's track while a level is loaded, otherwise
        // the menu theme. GameWindow passes it to AudioManager::PlayMusic every frame.
        std::string MusicTrack() const;
        // True while a level is loaded but not being played (paused, or Settings from pause).
        bool IsLevelPaused() const { return activeLevel && screen != ScreenId::Playing; }

    private:
        void Enter(ScreenId next);

        void Apply(Action action);

        void BuildMenu(bool preserveFocus = false);

        GameConfig &config;
        ScreenId screen = ScreenId::MainMenu;
        ScreenId settingsReturn = ScreenId::MainMenu;
        std::unique_ptr<Level> activeLevel;
        Menu menu;
        bool quit = false;
        bool settingsDirty = false;
    };
}
