#include "objects/moving_object.h"
#include <algorithm>
#include <cmath>

namespace rage {
    MovingObject::MovingObject(GameObjectDefinition definition) : GameObject(definition) {
    }

    void MovingObject::Update(float dt) {
        if (!active) {
            return;
        }
        if (!std::isfinite(dt) || dt <= 0) return;
        const auto advance = [dt](float &value, float &speed, float start, float range) {
            if (range <= 0) { value += speed * dt; return; }
            const float offset = value - start;
            const float phase = speed >= 0 ? offset : 2 * range - offset;
            const float next = std::fmod(phase + std::abs(speed) * dt, 2 * range);
            value = start + (next < range ? next : 2 * range - next);
            speed = std::copysign(std::abs(speed), next < range ? 1.0f : -1.0f);
        };
        advance(position.x, velocity.x, initialPosition.x, definition.range.x);
        advance(position.y, velocity.y, initialPosition.y, definition.range.y);
        animator.Update(dt);
    }

    FallingObject::FallingObject(GameObjectDefinition definition) : GameObject(definition) {
    }

    void FallingObject::StartFall() {
        if (landed) {
            return;
        }
        falling = true;
        animator.SetState("fall");
    }

    void FallingObject::Update(float dt) {
        if (!active || !falling || landed) {
            return;
        }
        position.y += velocity.y * dt;
        animator.Update(dt);
        const float ground = initialPosition.y + definition.range.y;
        if (position.y >= ground) {
            position.y = ground;
            velocity.y = 0;
            falling = false;
            landed = true;
            triggered = true;
            actionCompleted = true;
            animator.SetState("land");
        }
    }

    void FallingObject::OnBlocked(const CollisionResult &result) {
        if (!falling || !result.grounded) return;
        velocity = {};
        falling = false;
        landed = true;
        triggered = true;
        actionCompleted = true;
        animator.SetState("land");
    }

    void FallingObject::Reset() {
        GameObject::Reset();
        falling = false;
        landed = false;
    }
}
