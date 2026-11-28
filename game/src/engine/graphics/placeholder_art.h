#pragma once
#include "engine/resources/resources.h"

namespace rage::art {
    // Each function tries its named art slot, then draws a temporary shape fallback.
    void MenuBackdrop(const Resources &resources, ArtId screen);

    void Student(const Resources &resources, Rectangle bounds);

    void Entity(const Resources &resources, Rectangle bounds);

    void Platform(const Resources &resources, Rectangle bounds, Color accent);

    // Shared stand-in for the Game Data bird asset until its PNGs are supplied.
    void Bird(Rectangle bounds, bool facingRight, float seconds);

    void LevelBackdrop(const Resources &resources, ArtId backdrop, Rectangle world, Color accent);
}
