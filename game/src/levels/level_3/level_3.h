#pragma once

#include "levels/level.h"

namespace rage {
    class Level3 final : public Level {
    public:
        Level3();

    private:
        std::vector<Event> CreateEvents();
    };
}
