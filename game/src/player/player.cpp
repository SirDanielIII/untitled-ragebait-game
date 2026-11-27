#include "player/player.h"

#include <algorithm>
#include <cmath>

namespace rage {
    namespace {
        bool Overlaps(Rectangle a, Rectangle b) {
            return a.x < b.x + b.width && a.x + a.width > b.x && a.y < b.y + b.height && a.y + a.height > b.y;
        }
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
        crouching = false;
        exhausted = false;
        bodyHeight = standingSize.y;
        climbLockSeconds = 0.0f;
        stamina = staminaCapacity;
        health = maxHealth;
        moveInput = 0.0f;
    }

    void Player::Reset(Vector2 spawn, const GameConfig &config) {
        standingSize = {config.standingWidth, config.standingHeight};
        staminaCapacity = config.maxStamina;
        bounceClimbLock = config.bounceClimbLock;
        Reset(spawn);
    }

    void Player::Bounce(Vector2 launchVelocity) {
        velocity = launchVelocity;
        grounded = false;
        climbing = false;
        climbLockSeconds = bounceClimbLock;
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
        standingSize = {config.standingWidth, config.standingHeight};
        staminaCapacity = config.maxStamina;
        bounceClimbLock = config.bounceClimbLock;
        stamina = std::clamp(stamina, 0.0f, staminaCapacity);
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
        // Input, physics and stamina share small steps, including takeoffs after landing.
        float remaining = std::min(dt, 0.1f);
        while (remaining > 0.0f) {
            const float step = std::min(remaining, 1.0f / 120.0f);
            remaining -= step;

            const bool wasCrouching = crouching;
            const float height = config.standingHeight * (input.duck ? config.crouchHeightMultiplier : 1.0f);
            const Rectangle resized = {position.x, position.y + bodyHeight - height, standingSize.x, height};
            // Keep the feet fixed. A low ceiling keeps the player crouched after release.
            const bool blocked = height > bodyHeight && (resized.y < world.y ||
                std::any_of(platforms.begin(), platforms.end(), [&](Rectangle p) { return Overlaps(resized, p); }));
            if (!blocked) {
                position.y = resized.y;
                bodyHeight = height;
                crouching = input.duck;
            }
            // Exact contact also establishes support immediately after spawning/resetting.
            grounded = settleOnSupport();
            const bool wasClimbing = climbing;
            climbing = false;
            climbLockSeconds = std::max(0.0f, climbLockSeconds - step);
            if (input.climb && climbLockSeconds == 0.0f && stamina > 0.0f && !exhausted) {
                const Rectangle body = Bounds();
                for (const Rectangle platform: platforms) {
                    if (body.y >= platform.y + platform.height || body.y + body.height <= platform.y) continue;
                    if (std::abs(body.x + body.width - platform.x) <= config.climbContactDistance) {
                        position.x = platform.x - body.width;
                        climbing = true;
                        break;
                    }
                    if (std::abs(body.x - platform.x - platform.width) <= config.climbContactDistance) {
                        position.x = platform.x + platform.width;
                        climbing = true;
                        break;
                    }
                }
            }
            if (wasClimbing && !climbing) velocity.y = 0.0f;

            const bool crouchJump = crouching || wasCrouching;
            const float jumpCost = crouchJump ? config.crouchJumpStaminaCost : config.jumpStaminaCost;
            if (input.jump && grounded && !climbing && stamina >= jumpCost && !exhausted) {
                velocity.y = -config.jumpSpeed * (crouchJump ? config.crouchJumpMultiplier : 1.0f);
                stamina = std::max(0.0f, stamina - jumpCost);
                if (stamina == 0.0f) exhausted = true;
                grounded = false;
            }
            const float speedMultiplier = (crouching ? config.crouchSpeedMultiplier : 1.0f)
                * (exhausted || LowStamina(config) ? config.exhaustedSpeedMultiplier : 1.0f);
            const float targetSpeed = moveInput * config.moveSpeed * speedMultiplier;
            if (climbing) {
                velocity.x = 0.0f;
                velocity.y = -config.climbSpeed;
                grounded = false;
            } else {
                // Crouching and low stamina limit existing momentum as well as the target.
                if (speedMultiplier < 1.0f) {
                    const float limit = config.moveSpeed * speedMultiplier;
                    velocity.x = std::clamp(velocity.x, -limit, limit);
                }
                const float initialSpeed = std::min(config.startSpeed * speedMultiplier, std::abs(targetSpeed));
                if (moveInput != 0.0f && std::abs(velocity.x) < initialSpeed && velocity.x * moveInput >= 0.0f)
                    velocity.x = std::copysign(initialSpeed, moveInput);
                const float rate = moveInput == 0.0f ? config.groundFriction
                    : (velocity.x * moveInput < 0.0f ? config.counterStrafeAcceleration : config.groundAcceleration);
                velocity.x = MoveToward(velocity.x, targetSpeed, rate * step);
            }
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

            const float drain = (crouching ? config.crouchStaminaDrain : 0.0f)
                + (climbing ? config.climbStaminaDrain : 0.0f);
            if (drain > 0.0f) stamina -= drain * step;
            // Walking and coasting recover stamina, including while exhaustion slows movement.
            else if (grounded && !crouching && !climbing) stamina += config.staminaRegen * step;
            stamina = std::clamp(stamina, 0.0f, staminaCapacity);
            if (stamina == 0.0f) exhausted = true;
            else if (stamina >= staminaCapacity * config.exhaustionRecoveryThreshold) exhausted = false;
            if (exhausted && climbing) {
                climbing = false;
                velocity.y = 0.0f;
            }
        }
    }
    void Player::ApplyKnockback(Vector2 force) {
        if (IsDead()) return;

        velocity.x = force.x;
        velocity.y = force.y;

        grounded = false;
        climbing = false;
    }
}
