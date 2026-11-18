#pragma once

#include "levels/level.h"

namespace rage {
    class Level1 final : public Level {
    public:
        Level1();

    private:
        std::vector<Event> CreateEvents();
    };
}
