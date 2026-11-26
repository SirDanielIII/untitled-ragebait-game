#pragma once

#include "levels/level.h"

#include <array>
#include <cstdint>

namespace rage {
    class Level1 final : public Level {
    public:
        Level1();

    private:
        struct Vehicle {
            Rectangle bounds;
            float speed;
            Color color;
        };

        struct FallingStar {
            float x;
            float triggerX;
            float y;
            float warningSeconds;
            bool warned;
            bool falling;
            bool finished;
        };

        void OnReset() override;
        void OnUpdate(float dt, const InputFrame& input) override;
        void DrawPileup() const;
        void DrawWorld(const Resources& resources) const override;
        float PlayerDuckBlend() const override { return duckBlend; }
        std::vector<Event> CreateEvents();
        void Respawn(const char* notification);

        std::array<Vehicle, 3> traffic = {};
        Rectangle pig = {};
        bool pigStarted = false;
        bool pigFinished = false;
        bool crashImpact = false;
        std::array<FallingStar, 2> stars = {};

        Vector2 checkpoint = {};
        float collisionCooldown = 0.0f;
        float duckBlend = 0.0f;
        float pileupSeconds = 0.0f;
        float slideProgress = 0.0f;
        float chickenX = 1460.0f;
        int collisionCount = 0;
        bool roadCheckpointReached = false;
        bool skyCheckpointReached = false;
        bool pileupStarted = false;
        bool pileupSettled = false;
        bool sliding = false;
        bool slideFinished = false;
        bool campusReached = false;
    };
}
