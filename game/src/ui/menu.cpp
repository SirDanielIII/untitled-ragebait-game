#include "menu.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace rage
{
namespace
{
Rectangle ItemBounds(const MenuItem& item)
{
    if (const auto* button = std::get_if<Button>(&item)) return button->bounds;
    return std::get<SliderItem>(item).slider.Bounds();
}

bool Contains(Rectangle bounds, Vector2 point)
{
    return point.x >= bounds.x && point.x <= bounds.x + bounds.width
        && point.y >= bounds.y && point.y <= bounds.y + bounds.height;
}
}

void Menu::SetItems(std::vector<MenuItem> next, bool preserveFocus)
{
    items = std::move(next);
    selected = preserveFocus && !items.empty() ? std::clamp(selected, 0, static_cast<int>(items.size()) - 1) : 0;
}

std::optional<Action> Menu::Update(const InputFrame& input)
{
    if (items.empty()) return std::nullopt;
    const int count = static_cast<int>(items.size());
    bool dragging = false;
    for (int i = 0; i < count; ++i)
    {
        if (auto* item = std::get_if<SliderItem>(&items[i]); item && item->slider.IsDragging())
        {
            selected = i;
            dragging = true;
            break;
        }
    }
    // Keep pointer capture on the same slider until the button is released.
    if (!dragging)
    {
        if (input.up) selected = (selected + count - 1) % count;
        if (input.down) selected = (selected + 1) % count;
        if (input.mouseMoved || input.click)
            for (int i = 0; i < count; ++i)
                if (Contains(ItemBounds(items[i]), input.mouse)) { selected = i; break; }
    }
    if (auto* slider = std::get_if<SliderItem>(&items[selected]))
    {
        if (slider->slider.Update(input, true)) return Action{slider->action, 0, slider->slider.Value()};
    }
    else if (const auto* button = std::get_if<Button>(&items[selected]))
    {
        if (input.confirm || (input.click && Contains(button->bounds, input.mouse))) return button->action;
    }
    return std::nullopt;
}

void Menu::Draw(Font font) const
{
    const Color cream = {253, 249, 175, 255};
    for (int i = 0; i < static_cast<int>(items.size()); ++i)
    {
        const bool focused = i == selected;
        if (const auto* slider = std::get_if<SliderItem>(&items[i]))
        {
            const Rectangle bounds = slider->slider.Bounds();
            DrawTextEx(font, slider->label.c_str(), {bounds.x, bounds.y - 98}, 28, 1, cream);
            const std::string valueText = slider->slider.Value() == 0.0f && !slider->zeroText.empty()
                ? slider->zeroText : std::to_string(static_cast<int>(std::round(slider->slider.Value() * slider->displayScale))) + slider->unit;
            DrawTextEx(font, valueText.c_str(), {bounds.x, bounds.y - 52}, 26, 1, RAYWHITE);
            slider->slider.Draw(focused);
            continue;
        }
        const auto& button = std::get<Button>(items[i]);
        const auto bounds = button.bounds;
        if (button.style == ButtonStyle::BackArrow)
        {
            DrawTriangle({bounds.x, bounds.y + bounds.height / 2},
                         {bounds.x + bounds.width, bounds.y + bounds.height}, {bounds.x + bounds.width, bounds.y},
                         focused ? cream : button.fill);
            continue;
        }
        if (button.style == ButtonStyle::Checkbox)
        {
            const Rectangle box = {bounds.x, bounds.y + (bounds.height - 48) / 2, 48, 48};
            DrawRectangleRec(box, button.checked ? button.fill : Color{32, 32, 32, 255});
            DrawRectangleLinesEx(box, focused ? 5.0f : 4.0f, focused ? Color{255, 209, 65, 255} : cream);
            DrawTextEx(font, button.label.c_str(), {bounds.x + 70, bounds.y + (bounds.height - button.fontSize) / 2},
                       button.fontSize, 1, focused ? cream : RAYWHITE);
            continue;
        }
        DrawRectangleRec(bounds, button.fill);
        if (focused) DrawRectangleLinesEx(bounds, 4, cream);
        const float fit = std::min(button.fontSize, button.fontSize * (bounds.width - 30) /
            std::max(1.0f, MeasureTextEx(font, button.label.c_str(), button.fontSize, 1).x));
        const Vector2 size = MeasureTextEx(font, button.label.c_str(), fit, 1);
        DrawTextEx(font, button.label.c_str(), {bounds.x + (bounds.width - size.x) / 2, bounds.y + (bounds.height - size.y) / 2},
                   fit, 1, RAYWHITE);
    }
}
}
