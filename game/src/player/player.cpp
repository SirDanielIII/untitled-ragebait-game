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
        constexpr float startSpeed = 60.0f;

        float MoveToward(float current, float target, float maxDelta){
            if (std::abs(target - current) <= maxDelta) return target;
            return current + (target > current ? maxDelta : -maxDelta);
        }
    }

    void Player::Reset(Vector2 spawn) {
        position = spawn;
        velocity = {};
        grounded = false;
        climbing = false;
        climbLockSeconds = 0.0f;
        stamina = 100.0f;
        health = maxHealth;
        moveInput = 0.0f;
    }

    void Player::Bounce(Vector2 launchVelocity) {
        velocity = launchVelocity;
        grounded = false;
        climbing = false;
        climbLockSeconds = 0.2f;
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
                        std::span<const Rectangle> platforms, Rectangle world,
                        std::span<const Rectangle> oneWayPlatforms) {
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
            // Clouds count as support only when the feet already touch the top.
            for (const Rectangle platform: oneWayPlatforms) {
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
        // Climbing: holding W/UP while touching the side of a platform
        // (within 2 px) latches onto it. Every platform is climbable.
        // The player is snapped to the platform's side and horizontal
        // movement is disabled while climbing.
        const bool wasClimbing = climbing;
        climbing = false;
        climbLockSeconds = std::max(0.0f, climbLockSeconds - std::min(dt, 0.1f));
        if (input.climb && climbLockSeconds == 0.0f && stamina > 0.0f) {
            const Rectangle body = Bounds();
            constexpr float contactDistance = 2.0f;
            for (const Rectangle platform: platforms) {
                const bool verticalOverlap = body.y < platform.y + platform.height
                    && body.y + body.height > platform.y;
                if (!verticalOverlap) continue;
                const float playerRight = body.x + body.width;
                const float platformRight = platform.x + platform.width;
                // Touching the left side of the platform.
                if (std::abs(playerRight - platform.x) <= contactDistance) {
                    position.x = platform.x - body.width;
                    climbing = true;
                    break;
                }
                // Touching the right side of the platform.
                if (std::abs(body.x - platformRight) <= contactDistance) {
                    position.x = platformRight;
                    climbing = true;
                    break;
                }
            }
        }
        // Stop upward momentum when the player releases the climbing key.
        if (wasClimbing && !climbing) {
            velocity.y = 0.0f;
        }
        // One horizontal axis: opposite keys cancel; diagonal normalization is unnecessary.
        const float targetSpeed = climbing ? 0.0f : std::clamp(input.move, -1.0f, 1.0f) * config.moveSpeed;
        const float rate = input.move != 0.0f ? groundAcceleration : groundFriction;
        // Space still works for jumping. W/UP can initiate climbing instead.
        if (input.jump && grounded && !climbing && stamina > 0.0f) {
            velocity.y = -config.jumpSpeed;
            grounded = false;
        }
        if (climbing) {
            velocity.x = 0.0f;
            velocity.y = -config.climbSpeed;
            grounded = false;
        }
        // Small steps keep simple axis-separated AABB collisions reliable at lower frame rates.
        float remaining = std::min(dt, 0.1f);
        if (!climbing && input.move != 0.0f && std::abs(velocity.x) < startSpeed && velocity.x * input.move >= 0.0f) {
            velocity.x = std::clamp(input.move, -1.0f, 1.0f) * startSpeed;
        }
        while (remaining > 0.0f) {
            const float step = std::min(remaining, 1.0f / 120.0f);
            remaining -= step;
            if (!climbing) velocity.x = MoveToward(velocity.x, targetSpeed, rate * step);
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
            // Gravity is disabled while climbing.
            if (!climbing) velocity.y += config.gravity * step;
            const float previousFeet = position.y + Bounds().height;
            const bool descending = velocity.y > 0.0f;
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
            // One-way platforms have no side or underside collision. Catch only
            // downward crossings of their top, choosing the highest crossed top.
            if (descending) {
                const Rectangle body = Bounds();
                const float currentFeet = body.y + body.height;
                float landingY = currentFeet;
                bool landed = false;
                for (const Rectangle cloud: oneWayPlatforms) {
                    if (body.x >= cloud.x + cloud.width || body.x + body.width <= cloud.x) continue;
                    if (previousFeet <= cloud.y + 0.01f && currentFeet >= cloud.y
                        && cloud.y <= landingY) {
                        landingY = cloud.y;
                        landed = true;
                    }
                }
                if (landed) {
                    position.y = landingY - body.height;
                    velocity.y = 0.0f;
                    grounded = true;
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
        // Stamina: standing on the ground regenerates it, climbing
        // drains it, and being airborne drains it at the jump rate.
        if (grounded) {
            stamina += config.staminaRegen * dt;
        } else if (climbing) {
            stamina -= config.climbStaminaDrain * dt;
        } else {
            stamina -= config.jumpStaminaDrain * dt;
        }
        stamina = std::clamp(stamina, 0.0f, config.maxStamina);
        if (stamina <= 0.0f) {
            climbing = false;
            velocity.y = 0.0f;
        }
    }
}
