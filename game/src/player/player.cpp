#include "player/player.h"

#include <algorithm>
#include <cmath>

namespace rage {
    namespace {
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
        lastCollision = {};
        moveInput = 0.0f;
        crouchJumpOrigin.reset();
    }

    void Player::Reset(Vector2 spawn, const GameConfig &config) {
        standingSize = {config.standingWidth, config.standingHeight};
        collisionOffset = {config.collisionOffsetX, config.collisionOffsetY};
        maxHealth = config.maxHealth;
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

    std::optional<Vector2> Player::TakeCrouchJump() {
        const auto origin = crouchJumpOrigin;
        crouchJumpOrigin.reset();
        return origin;
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
        CollisionSystem collisions;
        collisions.SetStaticScene(platforms, world, oneWayPlatforms);
        Update(dt, input, config, collisions);
    }

    void Player::Update(float dt, const InputFrame &input, const GameConfig &config,
                        CollisionSystem &collisions) {
        if (!std::isfinite(dt) || dt < 0.0f) return;
        if (IsDead()) return;
        if (dt == 0.0f) {
            Rectangle body = Bounds();
            lastCollision = collisions.Move(body, velocity, dt);
            position = {body.x - collisionOffset.x, body.y - collisionOffset.y};
            grounded = lastCollision.grounded;
            return;
        }
        moveInput = std::clamp(input.move, -1.0f, 1.0f);
        standingSize = {config.standingWidth, config.standingHeight};
        collisionOffset = {config.collisionOffsetX, config.collisionOffsetY};
        maxHealth = config.maxHealth;
        staminaCapacity = config.maxStamina;
        bounceClimbLock = config.bounceClimbLock;
        stamina = std::clamp(stamina, 0.0f, std::max(0.0f, staminaCapacity));
        CollisionSystem stepScene;
        const CollisionSystem *scene = &collisions;
        bool beforeMove = true;
        const auto resizeForCrouch = [&](bool duck) {
            const float height = config.standingHeight * (duck ? config.crouchHeightMultiplier : 1.0f);
            const Rectangle resized = {Bounds().x, Bounds().y + bodyHeight - height, standingSize.x, height};
            // Keep the feet fixed. A low ceiling keeps the player crouched after release.
            const bool blocked = height > bodyHeight && !scene->CanOccupy(resized, {}, beforeMove);
            if (!blocked) {
                position.y = resized.y - collisionOffset.y;
                bodyHeight = height;
                crouching = duck;
            }
        };
        // Input, physics and stamina share small steps, including takeoffs after landing.
        float remaining = dt;
        lastCollision = {};
        while (remaining > 0.0f) {
            const float step = std::min(remaining, 1.0f / 120.0f);
            if (dt > 1.0f / 120.0f) {
                stepScene = collisions.Slice((dt - remaining) / dt, (dt - remaining + step) / dt);
                scene = &stepScene;
            }
            remaining -= step;
            beforeMove = true;

            const bool wasCrouching = crouching;
            // Exact contact also establishes support immediately after spawning/resetting.
            grounded = velocity.y >= 0 && scene->Supported(Bounds());
            resizeForCrouch(input.duck && grounded);
            const bool wasClimbing = climbing;
            climbing = false;
            climbLockSeconds = std::max(0.0f, climbLockSeconds - step);
            if (input.climb && climbLockSeconds == 0.0f && stamina > 0.0f && !exhausted) {
                Rectangle body = Bounds();
                climbing = scene->SnapToWall(body, config.climbContactDistance);
                position.x = body.x - collisionOffset.x;
            }
            if (wasClimbing && !climbing) velocity.y = 0.0f;

            const bool crouchJump = crouching || wasCrouching;
            const float jumpCost = crouchJump ? config.crouchJumpStaminaCost : config.jumpStaminaCost;
            if (input.jump && grounded && !climbing && stamina >= jumpCost && !exhausted) {
                velocity.y = -config.jumpSpeed * (crouchJump ? config.crouchJumpMultiplier : 1.0f);
                stamina = std::max(0.0f, stamina - jumpCost);
                if (stamina == 0.0f) exhausted = true;
                grounded = false;
                if (crouchJump) crouchJumpOrigin = Vector2{Bounds().x + standingSize.x / 2, Bounds().y + bodyHeight};
                resizeForCrouch(false);
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
                    const float limit = std::abs(config.moveSpeed * speedMultiplier);
                    velocity.x = std::clamp(velocity.x, -limit, limit);
                }
                const float initialSpeed = std::min(config.startSpeed * speedMultiplier, std::abs(targetSpeed));
                if (moveInput != 0.0f && std::abs(velocity.x) < initialSpeed && velocity.x * moveInput >= 0.0f)
                    velocity.x = std::copysign(initialSpeed, moveInput);
                const float rate = moveInput == 0.0f ? config.groundFriction
                    : (velocity.x * moveInput < 0.0f ? config.counterStrafeAcceleration : config.groundAcceleration);
                velocity.x = MoveToward(velocity.x, targetSpeed, rate * step);
            }
            if (!climbing) velocity.y += config.gravity * step;
            Rectangle body = Bounds();
            const auto result = scene->Move(body, velocity, step);
            position = {body.x - collisionOffset.x, body.y - collisionOffset.y};
            beforeMove = false;
            grounded = result.grounded;
            lastCollision.grounded = grounded;
            lastCollision.crushed = lastCollision.crushed || result.crushed;
            for (const auto &contact : result.contacts) {
                if (std::ranges::none_of(lastCollision.contacts, [&](const auto &c) { return c.objectId == contact.objectId; }))
                    lastCollision.contacts.push_back(contact);
            }
            // Walking off an edge or climbing also ends crouch. Preserve a compact
            // collider only while a low ceiling physically prevents standing up.
            if (!grounded) resizeForCrouch(false);

            const float drain = (crouching ? config.crouchStaminaDrain : 0.0f)
                + (climbing ? config.climbStaminaDrain : 0.0f);
            if (drain > 0.0f) stamina -= drain * step;
            // Walking and coasting recover stamina, including while exhaustion slows movement.
            else if (grounded && !crouching && !climbing) stamina += config.staminaRegen * step;
            stamina = std::clamp(stamina, 0.0f, std::max(0.0f, staminaCapacity));
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
