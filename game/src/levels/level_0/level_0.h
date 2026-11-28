#pragma once

#include "levels/level.h"

namespace rage {
    // Asset IDs for Level 0's things, one organized list: level_0_sprites.cpp.
    std::vector<SpriteAssignment> Level0Sprites();

    class Level0 final : public Level {
    public:
        Level0();

    private:
        std::vector<Event> CreateEvents();
        void OnReset() override;
        void OnUpdate(float dt, const InputFrame &input) override;

        DialogueMessageId idleCue = 0;
        double stillSeconds = 0;
        Vector2 previousPosition = {};
        bool movedThisFrame = false;

        DialogueMessageId hallwayCue = 0;
        float followUpTime = 0.0f;
    };
}
