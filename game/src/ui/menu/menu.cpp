#include "ui/menu/menu.h"
#include "ui/theme.h"
#include "ui/text/text.h"

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

void Menu::Draw(const Resources& resources) const
{
    const Color cream = theme::paper;
    for (int i = 0; i < static_cast<int>(items.size()); ++i)
    {
        const bool focused = i == selected;
        if (const auto* slider = std::get_if<SliderItem>(&items[i]))
        {
            const Rectangle bounds = slider->slider.Bounds();
            DrawTextFit(resources, slider->label, {bounds.x, bounds.y - 88, bounds.width, 28}, 24, cream);
            const std::string valueText = slider->slider.Value() == 0.0f && !slider->zeroText.empty()
                ? slider->zeroText : std::to_string(static_cast<int>(std::round(slider->slider.Value() * slider->displayScale))) + slider->unit;
            DrawTextFit(resources, valueText, {bounds.x, bounds.y - 52, bounds.width, 30}, 26, RAYWHITE);
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
            DrawRectangleRec(box, button.checked ? button.fill : theme::background);
            DrawRectangleLinesEx(box, 2.0f, focused ? theme::gold : theme::border);
            if (button.checked)
            {
                DrawLineEx({box.x + 11, box.y + 24}, {box.x + 21, box.y + 34}, 4, theme::background);
                DrawLineEx({box.x + 21, box.y + 34}, {box.x + 38, box.y + 14}, 4, theme::background);
            }
            DrawTextFit(resources, button.label, {bounds.x + 70, bounds.y + (bounds.height - button.fontSize) / 2, bounds.width - 70, button.fontSize},
                        button.fontSize, focused ? cream : RAYWHITE);
            continue;
        }
        if (!resources.DrawArt(ArtId::Button, bounds, ArtFit::Stretch)) DrawRectangleRec(bounds, button.fill);
        DrawRectangleLinesEx(bounds, focused ? 3.0f : 1.0f, focused ? theme::mint : theme::border);
        if (focused) DrawRectangle(static_cast<int>(bounds.x), static_cast<int>(bounds.y), 6, static_cast<int>(bounds.height), theme::mint);
        const float fit = std::min(button.fontSize, button.fontSize * (bounds.width - 30) /
            std::max(1.0f, resources.MeasureText(button.label.c_str(), button.fontSize, 1).x));
        const Vector2 size = resources.MeasureText(button.label.c_str(), fit, 1);
        resources.DrawText(button.label.c_str(), {bounds.x + (bounds.width - size.x) / 2, bounds.y + (bounds.height - size.y) / 2},
                   fit, 1, RAYWHITE);
    }
}
}
