#include "objects/moving_object.h"
#include <algorithm>

namespace rage {
    MovingObject::MovingObject(GameObjectDefinition definition) : GameObject(definition) {
    }

    void MovingObject::Update(float dt) {
        if (!active) {
            return;
        }
        position.x += velocity.x * dt;
        position.y += velocity.y * dt;
        animator.Update(dt);
        // Reverse at each end of the range, and clamp so the object
        // never drifts past it over many frames.
        if (definition.range.x > 0) {
            if (position.x > initialPosition.x + definition.range.x) {
                position.x = initialPosition.x + definition.range.x;
                velocity.x = -std::abs(velocity.x);
            } else if (position.x < initialPosition.x) {
                position.x = initialPosition.x;
                velocity.x = std::abs(velocity.x);
            }
        }
        if (definition.range.y > 0) {
            if (position.y > initialPosition.y + definition.range.y) {
                position.y = initialPosition.y + definition.range.y;
                velocity.y = -std::abs(velocity.y);
            } else if (position.y < initialPosition.y) {
                position.y = initialPosition.y;
                velocity.y = std::abs(velocity.y);
            }
        }
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

    void FallingObject::Reset() {
        GameObject::Reset();
        falling = false;
        landed = false;
    }
}
