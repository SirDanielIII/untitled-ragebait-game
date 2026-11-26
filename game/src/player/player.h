#pragma once

#include "engine/config/config.h"
#include "engine/input/input.h"
#include "raylib.h"
#include <span>

namespace rage {
    enum class PlayerState { Idle, Walking, Sliding, Airborne, Dead };
    class Player {
    public:
        void Reset(Vector2 spawn);

        void Update(float dt, const InputFrame &input, const GameConfig &config,
                    std::span<const Rectangle> platforms, Rectangle world,
                    std::span<const Rectangle> oneWayPlatforms = {});

        Rectangle Bounds() const { return {position.x, position.y, 30.0f, 44.0f}; }
        Vector2 Position() const { return position; }
        Vector2 Velocity() const { return velocity; }
        bool Grounded() const { return grounded; }
        bool Climbing() const { return climbing; }
        float StaminaPercent() const { return stamina / 100.0f; }
        float StaminaPercent(float maxStamina) const { return maxStamina > 0.0f ? stamina / maxStamina : 0.0f; }
        void TakeDamage(int amount);
        void Kill();
        int GetHealth() const { return health; }
        int GetMaxHealth() const { return maxHealth; }
        bool IsDead() const { return health <= 0; }
        PlayerState State() const;

    private:
        Vector2 position = {};
        Vector2 velocity = {};
        bool grounded = false;
        bool climbing = false;
        float stamina = 100.0f;
        int maxHealth = 1;
        int health = 1;
        float moveInput = 0.0f;
    };
}
