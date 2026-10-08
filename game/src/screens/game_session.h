#pragma once

#include "../core/config.h"
#include "../levels/level_scene.h"
#include "../ui/menu.h"
#include <memory>

namespace rage
{
enum class ScreenId { MainMenu, LevelSelect, Settings, Playing, Paused };

// Owns screen navigation and the active scene. No screen owns a loop or Raylib context.
class GameSession
{
public:
    explicit GameSession(GameConfig& config);
    bool Update(float dt, const InputFrame& input); // true for discrete menu click feedback
    void Draw(Font font, const std::string& settingsStatus) const;
    ScreenId Screen() const { return screen; }
    const LevelScene* Scene() const { return scene.get(); }
    const Menu& Navigation() const { return menu; }
    bool WantsQuit() const { return quit; }
    bool TakeSettingsDirty();

private:
    void Enter(ScreenId next);
    void Apply(Action action);
    void BuildMenu(bool preserveFocus = false);
    GameConfig& config;
    ScreenId screen = ScreenId::MainMenu;
    ScreenId settingsReturn = ScreenId::MainMenu;
    std::unique_ptr<LevelScene> scene;
    Menu menu;
    bool quit = false;
    bool settingsDirty = false;
};
}
