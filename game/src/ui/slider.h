#pragma once

#include "../core/input.h"

namespace rage
{
// A value widget with no knowledge of configuration, audio or dialogue.
class Slider
{
public:
    Slider(Rectangle bounds, float minimum, float maximum, float step, float value);
    bool Update(const InputFrame& input, bool focused);
    void Draw(bool focused) const;
    void SetValue(float value);
    float Value() const { return value; }
    Rectangle Bounds() const { return bounds; }
    bool IsDragging() const { return dragging; }
    void CancelDrag() { dragging = false; }
    float PositionFromValue(float value) const;
    float ValueFromPosition(float position) const;

private:
    Rectangle bounds;
    float minimum;
    float maximum;
    float step;
    float value;
    bool dragging = false;
};
}
