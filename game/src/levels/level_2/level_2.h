#pragma once

#include "levels/level.h"
#include <array>

namespace rage {

    class Level2 final : public Level {
    public:
        Level2();

    private:
        struct Bird {
            Rectangle bounds;
            float speed;
            float patrolLeft;
            float patrolRight;
            float homeY;
            float swoopCooldown;
            float swoopTimer;
            float attackCooldown;
            bool facingRight;
            bool swooping;
        };

        void OnReset() override;
        void OnUpdate(float dt, const InputFrame& input) override;
        void DrawWorld(const Resources& resources) const override;

        std::vector<Event> CreateEvents();

        std::array<Bird, 10> birds = {};
    };

}