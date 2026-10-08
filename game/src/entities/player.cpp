#include "player.h"

#include <algorithm>
#include <cmath>

namespace rage
{
namespace
{
bool Overlaps(Rectangle a, Rectangle b)
{
    return a.x < b.x + b.width && a.x + a.width > b.x && a.y < b.y + b.height && a.y + a.height > b.y;
}
}

void Player::Reset(Vector2 spawn)
{
    position = spawn;
    velocity = {};
    grounded = false;
}

void Player::Update(float dt, const InputFrame& input, const GameConfig& config,
                    std::span<const Rectangle> platforms, Rectangle world)
{
    if (!std::isfinite(dt) || dt <= 0.0f) return;
    auto settleOnSupport = [&]()
    {
        if (velocity.y < 0.0f) return false;
        const Rectangle body = Bounds();
        const float bottom = body.y + body.height;
        for (const Rectangle platform : platforms)
        {
            if (body.x < platform.x + platform.width && body.x + body.width > platform.x
                && std::abs(bottom - platform.y) <= 0.01f)
            {
                position.y = platform.y - body.height;
                velocity.y = 0.0f;
                return true;
            }
        }
        if (std::abs(bottom - (world.y + world.height)) <= 0.01f)
        {
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
    velocity.x = std::clamp(input.move, -1.0f, 1.0f) * config.moveSpeed;
    if (input.jump && grounded) { velocity.y = -config.jumpSpeed; grounded = false; }
    // Small steps keep simple axis-separated AABB collisions reliable at lower frame rates.
    float remaining = std::min(dt, 0.1f);
    while (remaining > 0.0f)
    {
        const float step = std::min(remaining, 1.0f / 120.0f);
        remaining -= step;
        position.x += velocity.x * step;
        for (const auto platform : platforms)
        {
            if (!Overlaps(Bounds(), platform)) continue;
            if (velocity.x > 0.0f) position.x = platform.x - Bounds().width;
            else if (velocity.x < 0.0f) position.x = platform.x + platform.width;
        }
        position.x = std::clamp(position.x, world.x, world.x + world.width - Bounds().width);
        velocity.y += config.gravity * step;
        position.y += velocity.y * step;
        grounded = false;
        for (const auto platform : platforms)
        {
            if (!Overlaps(Bounds(), platform)) continue;
            if (velocity.y > 0.0f)
            {
                position.y = platform.y - Bounds().height;
                velocity.y = 0.0f;
                grounded = true;
            }
            else if (velocity.y < 0.0f)
            {
                position.y = platform.y + platform.height;
                velocity.y = 0.0f;
            }
        }
        if (position.y < world.y) { position.y = world.y; velocity.y = 0.0f; }
        if (position.y + Bounds().height > world.y + world.height)
        {
            position.y = world.y + world.height - Bounds().height;
            velocity.y = 0.0f;
            grounded = true;
        }
        if (!grounded) grounded = settleOnSupport();
    }
}

void Player::Draw() const
{
    DrawRectangleRec(Bounds(), Color{245, 210, 123, 255});
    DrawRectangleLinesEx(Bounds(), 2.0f, Color{83, 63, 43, 255});
    DrawRectangle(static_cast<int>(position.x + 6), static_cast<int>(position.y + 9), 5, 5, Color{34, 39, 54, 255});
    DrawRectangle(static_cast<int>(position.x + 19), static_cast<int>(position.y + 9), 5, 5, Color{34, 39, 54, 255});
}
}
