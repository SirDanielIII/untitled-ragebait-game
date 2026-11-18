#pragma once

#include "levels/level.h"

namespace rage {
    class Level2 final : public Level {
    public:
        Level2();

    private:
        std::vector<Event> CreateEvents();
    };
}
