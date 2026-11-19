#include "player/player.h"

#include <algorithm>
#include <cmath>

namespace rage {
    namespace {
        bool Overlaps(Rectangle a, Rectangle b) {
            return a.x < b.x + b.width && a.x + a.width > b.x && a.y < b.y + b.height && a.y + a.height > b.y;
        }
        constexpr float groundAcceleration = 900.0f; // px/s^2 while pressing a direction
        constexpr float groundFriction = 500.0f;     // px/s^2 while coasting

        float MoveToward(float current, float target, float maxDelta){
            if (std::abs(target - current) <= maxDelta) return target;
            return current + (target > current ? maxDelta : -maxDelta);
        }
    }

    void Player::Reset(Vector2 spawn) {
        position = spawn;
        velocity = {};
        grounded = false;
        health = maxHealth;
        moveInput = 0.0f;
    }

    void Player::TakeDamage(int amount) {
        if (IsDead() || amount <= 0) return;
        health = std::max(0, health - amount);
    }

    void Player::Kill() {
        health = 0;
    }

    PlayerState Player::State() const {
        if (IsDead()) return PlayerState::Dead;
        if (!grounded) return PlayerState::Airborne;
        if (std::abs(velocity.x) < 0.01f) return PlayerState::Idle;
        // Pushing the same way you're moving is walking; coasting or braking is sliding.
        return moveInput * velocity.x > 0.0f ? PlayerState::Walking : PlayerState::Sliding;
    }

    void Player::Update(float dt, const InputFrame &input, const GameConfig &config,
                        std::span<const Rectangle> platforms, Rectangle world) {
        if (!std::isfinite(dt) || dt <= 0.0f) return;
        if (IsDead()) return;
        moveInput = std::clamp(input.move, -1.0f, 1.0f);
        auto settleOnSupport = [&]() {
            if (velocity.y < 0.0f) return false;
            const Rectangle body = Bounds();
            const float bottom = body.y + body.height;
            for (const Rectangle platform: platforms) {
                if (body.x < platform.x + platform.width && body.x + body.width > platform.x
                    && std::abs(bottom - platform.y) <= 0.01f) {
                    position.y = platform.y - body.height;
                    velocity.y = 0.0f;
                    return true;
                }
            }
            if (std::abs(bottom - (world.y + world.height)) <= 0.01f) {
                position.y = world.y + world.height - body.height;
                velocity.y = 0.0f;
                return true;
            }
            return false;
        };
        // Exact edge contact must count even when gravity displacement rounds to zero.
        // Also establishes support before a jump immediately after spawning/resetting.
        grounded = settleOnSupport();
        // One horizontal axis: opposite keys cancel; diagonal normalization is unnecessary.
        const float targetSpeed = std::clamp(input.move, -1.0f, 1.0f) * config.moveSpeed;
        const float rate = input.move != 0.0f ? groundAcceleration : groundFriction;
        if (input.jump && grounded) {
            velocity.y = -config.jumpSpeed;
            grounded = false;
        }
        // Small steps keep simple axis-separated AABB collisions reliable at lower frame rates.
        float remaining = std::min(dt, 0.1f);
        while (remaining > 0.0f) {
            const float step = std::min(remaining, 1.0f / 120.0f);
            remaining -= step;
            velocity.x = MoveToward(velocity.x, targetSpeed, rate * step);
            position.x += velocity.x * step;
            for (const auto platform: platforms) {
                if (!Overlaps(Bounds(), platform)) continue;
                if (velocity.x > 0.0f) position.x = platform.x - Bounds().width;
                else if (velocity.x < 0.0f) position.x = platform.x + platform.width;
                velocity.x = 0.0f;
            }
            const float beforeClamp = position.x;
            position.x = std::clamp(position.x, world.x, world.x + world.width - Bounds().width);
            if (position.x != beforeClamp) velocity.x = 0.0f;
            velocity.y += config.gravity * step;
            position.y += velocity.y * step;
            grounded = false;
            for (const auto platform: platforms) {
                if (!Overlaps(Bounds(), platform)) continue;
                if (velocity.y > 0.0f) {
                    position.y = platform.y - Bounds().height;
                    velocity.y = 0.0f;
                    grounded = true;
                } else if (velocity.y < 0.0f) {
                    position.y = platform.y + platform.height;
                    velocity.y = 0.0f;
                }
            }
            if (position.y < world.y) {
                position.y = world.y;
                velocity.y = 0.0f;
            }
            if (position.y + Bounds().height > world.y + world.height) {
                position.y = world.y + world.height - Bounds().height;
                velocity.y = 0.0f;
                grounded = true;
            }
            if (!grounded) grounded = settleOnSupport();
        }
    }
}
