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

        struct Bird {
            Rectangle bounds;
            float speed;
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
        void UpdateScript(float dt, const InputFrame& input);
        // Moves the art-only objects onto the hitboxes the script moved,
        // so each sprite always sits exactly where its collision is.
        void SyncObjects();
        void DrawWorld(const Resources& resources) const override;
        std::vector<Event> CreateEvents();
        void Respawn(const char* notification);

        std::array<Vehicle, 3> traffic = {};
        std::array<Bird, 2> birds = {};
        std::array<FallingStar, 2> stars = {};

        Vector2 checkpoint = {};
        float collisionCooldown = 0.0f;
        float duckSeconds = 0.0f;
        float pileupSeconds = 0.0f;
        float slideProgress = 0.0f;
        float chickenX = 1550.0f;
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
