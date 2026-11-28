#pragma once

#include "raylib.h"

namespace rage {
    class Resources;
    class Player;
    struct GameConfig;

    // Square pixels, stepped corners and a hard shadow, shared by gameplay overlays.
    void DrawHudPanel(Rectangle bounds, Color border, Color fill);
    void DrawPlayerCard(const Resources &resources, const Player &player, const GameConfig &config);
}
