#pragma once

#include "engine/config/config.h"
#include "engine/input/input.h"
#include "raylib.h"
#include <span>

namespace rage {
    class Player {
    public:
        void Reset(Vector2 spawn);

        void Update(float dt, const InputFrame &input, const GameConfig &config,
                    std::span<const Rectangle> platforms, Rectangle world);

        Rectangle Bounds() const { return {position.x, position.y, 30.0f, 44.0f}; }
        Vector2 Position() const { return position; }
        Vector2 Velocity() const { return velocity; }
        bool Grounded() const { return grounded; }
        void TakeDamage(int amount);
        void Kill();
        int GetHealth() const { return health; }
        int GetMaxHealth() const { return maxHealth; }
        bool IsDead() const { return health <= 0; }

    private:
        Vector2 position = {};
        Vector2 velocity = {};
        bool grounded = false;
        int maxHealth = 1;
        int health = 1;
    };
}
