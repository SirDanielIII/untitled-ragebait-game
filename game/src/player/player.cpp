
#include "player/player.h"

#include <algorithm>
#include <cmath>

namespace rage {
namespace {

bool Overlaps(Rectangle a, Rectangle b) {
    return a.x < b.x + b.width &&
           a.x + a.width > b.x &&
           a.y < b.y + b.height &&
           a.y + a.height > b.y;
}

} // namespace

void Player::Reset(Vector2 spawn) {
    position = spawn;
    velocity = {};
    grounded = false;
    climbing = false;
    stamina = 100.0f;
}

void Player::Update(float dt, const InputFrame& input,
                    const GameConfig& config,
                    std::span<const Rectangle> platforms,
                    Rectangle world) {
    if (!std::isfinite(dt) || dt <= 0.0f) {
        return;
    }

    auto settleOnSupport = [&]() {
        if (velocity.y < 0.0f) {
            return false;
        }

        const Rectangle body = Bounds();
        const float bottom = body.y + body.height;

        for (const Rectangle& platform : platforms) {
            const bool horizontalOverlap =
                body.x < platform.x + platform.width &&
                body.x + body.width > platform.x;

            if (horizontalOverlap &&
                std::abs(bottom - platform.y) <= 0.01f) {
                position.y = platform.y - body.height;
                velocity.y = 0.0f;
                return true;
            }
        }

        const float worldBottom = world.y + world.height;

        if (std::abs(bottom - worldBottom) <= 0.01f) {
            position.y = worldBottom - body.height;
            velocity.y = 0.0f;
            return true;
        }

        return false;
    };

    grounded = settleOnSupport();

    // Check whether the player is touching the side of a platform.
    // Every existing platform is automatically climbable.
    const bool wasClimbing = climbing;
    climbing = false;

    if (input.climb && stamina > 0.0f) {
        const Rectangle body = Bounds();
        constexpr float CONTACT_DISTANCE = 2.0f;

        for (const Rectangle& platform : platforms) {
            const bool verticalOverlap =
                body.y < platform.y + platform.height &&
                body.y + body.height > platform.y;

            if (!verticalOverlap) {
                continue;
            }

            const float playerRight = body.x + body.width;
            const float platformRight = platform.x + platform.width;

            // Player is touching the left side of the platform.
            if (std::abs(playerRight - platform.x) <= CONTACT_DISTANCE) {
                position.x = platform.x - body.width;
                climbing = true;
                break;
            }

            // Player is touching the right side of the platform.
            if (std::abs(body.x - platformRight) <= CONTACT_DISTANCE) {
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

    // Horizontal movement is disabled while climbing.
    velocity.x = climbing
        ? 0.0f
        : std::clamp(input.move, -1.0f, 1.0f) * config.moveSpeed;

    // Space still works for jumping. W/UP can initiate climbing instead.
    if (input.jump && grounded && !climbing && stamina > 0.0f) {
        velocity.y = -config.jumpSpeed;
        grounded = false;
    }

    if (climbing) {
        velocity.y = -config.climbSpeed;
        grounded = false;
    }

    // Small steps keep axis-separated AABB collisions reliable.
    float remaining = std::min(dt, 0.1f);

    while (remaining > 0.0f) {
        const float step = std::min(remaining, 1.0f / 120.0f);
        remaining -= step;

        // Horizontal movement and collision.
        position.x += velocity.x * step;

        for (const Rectangle& platform : platforms) {
            if (!Overlaps(Bounds(), platform)) {
                continue;
            }

            if (velocity.x > 0.0f) {
                position.x = platform.x - Bounds().width;
            } else if (velocity.x < 0.0f) {
                position.x = platform.x + platform.width;
            }
        }

        position.x = std::clamp(
            position.x,
            world.x,
            world.x + world.width - Bounds().width
        );

        // Gravity is disabled while climbing.
        if (!climbing) {
            velocity.y += config.gravity * step;
        }

        position.y += velocity.y * step;
        grounded = false;

        // Vertical platform collision.
        for (const Rectangle& platform : platforms) {
            if (!Overlaps(Bounds(), platform)) {
                continue;
            }

            if (velocity.y > 0.0f) {
                position.y = platform.y - Bounds().height;
                velocity.y = 0.0f;
                grounded = true;
            } else if (velocity.y < 0.0f) {
                position.y = platform.y + platform.height;
                velocity.y = 0.0f;
            }
        }

        // World boundaries.
        if (position.y < world.y) {
            position.y = world.y;
            velocity.y = 0.0f;
        }

        if (position.y + Bounds().height > world.y + world.height) {
            position.y = world.y + world.height - Bounds().height;
            velocity.y = 0.0f;
            grounded = true;
        }

        if (!grounded && !climbing) {
            grounded = settleOnSupport();
        }
    }

    // Stamina management.
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

} // namespace rage
