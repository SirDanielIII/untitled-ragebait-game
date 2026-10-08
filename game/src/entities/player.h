#pragma once

#include "../core/config.h"
#include "../core/input.h"
#include "raylib.h"
#include <span>

namespace rage
{
class Player
{
public:
    void Reset(Vector2 spawn);
    void Update(float dt, const InputFrame& input, const GameConfig& config,
                std::span<const Rectangle> platforms, Rectangle world);
    void Draw() const;
    Rectangle Bounds() const { return {position.x, position.y, 30.0f, 44.0f}; }
    Vector2 Position() const { return position; }
    Vector2 Velocity() const { return velocity; }
    bool Grounded() const { return grounded; }

private:
    Vector2 position = {};
    Vector2 velocity = {};
    bool grounded = false;
};
}
