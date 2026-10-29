#pragma once

#include "engine/config/config.h"
#include "levels/level.h"
#include "ui/menu/menu.h"
#include <memory>

namespace rage {
    enum class ScreenId { MainMenu, LevelSelect, Settings, Playing, Paused };

    // The level handler, implemented in main.cpp. Owns navigation and the active level.
    // Can also run without a window so navigation and gameplay can be tested.
    class Game {
    public:
        explicit Game(GameConfig &config);

        bool Update(float dt, const InputFrame &input); // true for discrete menu click feedback
        void Draw(const Resources &resources, const std::string &settingsStatus) const;

        ScreenId Screen() const { return screen; }
        const Level *ActiveLevel() const { return activeLevel.get(); }
        const Menu &Navigation() const { return menu; }
        bool WantsQuit() const { return quit; }

        bool TakeSettingsDirty();

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
