#pragma once

#include "levels/level.h"
#include <array>

namespace rage {

    // Asset IDs for Level 2's things, one organized list: level_2_sprites.cpp.
    std::vector<SpriteAssignment> Level2Sprites();

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
        // Keeps each bird object on its bird.
        void SyncObjects();
        static void DrawBird(const Bird& bird);

        std::vector<Event> CreateEvents();

        std::array<Bird, 10> birds = {};
    };

}