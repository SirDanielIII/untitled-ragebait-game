#pragma once
#include "engine/resources/resources.h"

namespace rage::art {
    // Optional menu image over an opaque solid-color background.
    void MenuBackdrop(const Resources &resources, std::string_view assetId);

    // Draw a CSV-defined static menu image, contained in the logical slot.
    // Missing rows/textures return false without drawing a debug placeholder.
    bool DrawMenuArt(const Resources &resources, std::string_view assetId, Rectangle bounds);

    void Student(const Resources &resources, Rectangle bounds);

    void Entity(const Resources &resources, Rectangle bounds);

    void Platform(const Resources &resources, Rectangle bounds, Color accent);

    // Shared stand-in for the Game Data bird asset until its PNGs are supplied.
    void Bird(Rectangle bounds, bool facingRight, float seconds);

    void LevelBackdrop(const Resources &resources, ArtId backdrop, Rectangle world, Color accent);
}
