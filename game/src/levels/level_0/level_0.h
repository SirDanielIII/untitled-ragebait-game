#pragma once

#include "levels/level.h"

namespace rage {
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
