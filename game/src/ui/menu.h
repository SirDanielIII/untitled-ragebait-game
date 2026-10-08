#pragma once

#include "../core/input.h"
#include "slider.h"
#include "raylib.h"
#include <optional>
#include <string>
#include <vector>
#include <variant>

namespace rage
{
enum class ActionKind
{
    StartLevel, OpenLevels, OpenSettings, Back, MainMenu, Quit, Resume, Restart,
    ToggleFps, ToggleMute, SetTargetFps, SetVolume, SetDialogueSpeed
};
struct Action { ActionKind kind; int value = 0; float scalar = 0.0f; };
enum class ButtonStyle { Filled, Checkbox, BackArrow };
struct Button
{
    std::string label;
    Action action;
    Rectangle bounds;
    Color fill = {8, 137, 177, 255};
    ButtonStyle style = ButtonStyle::Filled;
    bool checked = false;
    float fontSize = 28.0f;
};
struct SliderItem
{
    std::string label;
    ActionKind action;
    Slider slider;
    std::string unit;
    float displayScale = 1.0f;
    std::string zeroText;
};
using MenuItem = std::variant<Button, SliderItem>;

class Menu
{
public:
    void SetItems(std::vector<MenuItem> items, bool preserveFocus = false);
    std::optional<Action> Update(const InputFrame& input);
    void Draw(Font font) const;
    int Selected() const { return selected; }
    const std::vector<MenuItem>& Items() const { return items; }

private:
    std::vector<MenuItem> items;
    int selected = 0;
};
}
