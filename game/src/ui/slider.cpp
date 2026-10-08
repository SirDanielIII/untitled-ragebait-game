#include "slider.h"

#include <algorithm>
#include <cmath>

namespace rage
{
Slider::Slider(Rectangle bounds, float minimum, float maximum, float step, float value)
    : bounds(bounds), minimum(minimum), maximum(std::max(minimum, maximum)),
      step(std::max(0.0f, step)), value(minimum)
{
    SetValue(value);
}

void Slider::SetValue(float next)
{
    next = std::isfinite(next) ? std::clamp(next, minimum, maximum) : minimum;
    value = std::clamp(next, minimum, maximum);
}

float Slider::PositionFromValue(float next) const
{
    const float fraction = maximum > minimum ? (std::clamp(next, minimum, maximum) - minimum) / (maximum - minimum) : 0.0f;
    return bounds.x + 20.0f + fraction * std::max(0.0f, bounds.width - 40.0f);
}

float Slider::ValueFromPosition(float position) const
{
    const float width = bounds.width - 40.0f;
    const float fraction = width > 0.0f ? std::clamp((position - bounds.x - 20.0f) / width, 0.0f, 1.0f) : 0.0f;
    return minimum + fraction * (maximum - minimum);
}

bool Slider::Update(const InputFrame& input, bool focused)
{
    const float previous = value;
    const bool hit = input.mouse.x >= bounds.x && input.mouse.x <= bounds.x + bounds.width
                  && input.mouse.y >= bounds.y && input.mouse.y <= bounds.y + bounds.height;
    if (input.click && hit) dragging = true;
    if (dragging)
    {
        if (input.mouseDown || input.click)
        {
            float next = ValueFromPosition(input.mouse.x);
            if (step > 0.0f) next = minimum + std::round((next - minimum) / step) * step;
            SetValue(next);
        }
        else dragging = false;
    }
    else if (focused)
    {
        const float increment = step > 0.0f ? step : (maximum - minimum) / 100.0f;
        if (input.decrease) SetValue(value - increment);
        if (input.increase) SetValue(value + increment);
        if (input.minimum) SetValue(minimum);
        if (input.maximum) SetValue(maximum);
    }
    return std::abs(value - previous) > 0.000001f;
}

void Slider::Draw(bool focused) const
{
    const float center = bounds.y + bounds.height / 2.0f;
    const Color cream = {253, 249, 175, 255};
    DrawRectangleRec({bounds.x + 20, center - 6, std::max(0.0f, bounds.width - 40), 12}, RAYWHITE);
    const Rectangle knob = {PositionFromValue(value) - 20, center - 23, 40, 46};
    DrawRectangleRec(knob, Color{252, 79, 83, 255});
    DrawRectangleLinesEx(knob, 4, focused || dragging ? cream : RAYWHITE);
    if (focused) DrawRectangleLinesEx({bounds.x - 6, bounds.y - 5, bounds.width + 12, bounds.height + 10}, 1, cream);
}
}
