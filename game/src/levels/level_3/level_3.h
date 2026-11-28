#pragma once

#include "levels/level.h"

namespace rage {
    // Asset IDs for Level 3's things, one organized list: level_3_sprites.cpp.
    std::vector<SpriteAssignment> Level3Sprites();

    class Level3 final : public Level {
    public:
        Level3();

    private:
        std::vector<Event> CreateEvents();
    };
}
