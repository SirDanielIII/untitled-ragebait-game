#pragma once
#include "engine/resources/resources.h"

namespace rage::art {
    // Each function tries its named art slot, then draws a temporary shape fallback.
    void MenuBackdrop(const Resources &resources, ArtId screen);

    void Student(const Resources &resources, Rectangle bounds);

    void Entity(const Resources &resources, Rectangle bounds);

    void Platform(const Resources &resources, Rectangle bounds, Color accent);

    void LevelBackdrop(const Resources &resources, ArtId backdrop, Rectangle world, Color accent);
}
