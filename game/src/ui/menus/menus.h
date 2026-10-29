#pragma once
#include "engine/config/config.h"
#include "engine/resources/resources.h"
#include "ui/menu/menu.h"
#include "levels/level.h"

namespace rage {
    std::vector<MenuItem> MainMenuItems();

    std::vector<MenuItem> SettingsMenuItems(const GameConfig &config);

    std::vector<MenuItem> LevelSelectItems();

    std::vector<MenuItem> PauseMenuItems();

    void DrawMainMenu(const Resources &resources);

    void DrawSettingsMenu(const Resources &resources, const GameConfig &config, const std::string &status);

    void DrawLevelSelect(const Resources &resources, int selected);

    void DrawPauseMenu(const Resources &resources, const LevelDefinition &level);
}
