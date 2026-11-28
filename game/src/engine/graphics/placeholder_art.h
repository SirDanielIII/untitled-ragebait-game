#pragma once
#include "engine/resources/resources.h"

namespace rage::art {
    // Optional menu image over an opaque solid-color background.
    void MenuBackdrop(const Resources &resources, ArtId screen);

    void Student(const Resources &resources, Rectangle bounds);

    void Entity(const Resources &resources, Rectangle bounds);

    void Platform(const Resources &resources, Rectangle bounds, Color accent);

    // Shared stand-in for the Game Data bird asset until its PNGs are supplied.
    void Bird(Rectangle bounds, bool facingRight, float seconds);

    void LevelBackdrop(const Resources &resources, ArtId backdrop, Rectangle world, Color accent);
}
