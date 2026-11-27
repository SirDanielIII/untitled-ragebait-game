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
        void Reset(Vector2 spawn, const GameConfig &config);
        void Bounce(Vector2 launchVelocity);

        void Update(float dt, const InputFrame &input, const GameConfig &config,
                    std::span<const Rectangle> platforms, Rectangle world,
                    std::span<const Rectangle> oneWayPlatforms = {});

        Rectangle Bounds() const { return {position.x, position.y, standingSize.x, bodyHeight}; }
        Rectangle StandingBounds() const {
            return {position.x, position.y + bodyHeight - standingSize.y, standingSize.x, standingSize.y};
        }
        Vector2 Position() const { return position; }
        Vector2 Velocity() const { return velocity; }
        bool Grounded() const { return grounded; }
        bool Climbing() const { return climbing; }
        bool Crouching() const { return crouching; }
        bool Exhausted() const { return exhausted; }
        bool LowStamina(const GameConfig &config) const { return StaminaPercent() < config.lowStaminaThreshold; }
        float StaminaPercent() const { return StaminaPercent(staminaCapacity); }
        float StaminaPercent(float maxStamina) const { return maxStamina > 0.0f ? stamina / maxStamina : 0.0f; }
        void TakeDamage(int amount);
        void ApplyKnockback(Vector2 force);
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
        bool crouching = false;
        bool exhausted = false;
        Vector2 standingSize = {GameConfig{}.standingWidth, GameConfig{}.standingHeight};
        float bodyHeight = GameConfig{}.standingHeight;
        float climbLockSeconds = 0.0f;
        float bounceClimbLock = GameConfig{}.bounceClimbLock;
        float staminaCapacity = GameConfig{}.maxStamina;
        float stamina = GameConfig{}.maxStamina;
        int maxHealth = 1;
        int health = 1;
        float moveInput = 0.0f;
    };
}
