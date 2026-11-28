#pragma once

#include "engine/assets/animator.h"
#include "engine/assets/render_queue.h"

#include <variant>

namespace rage {
    // Level 1: loop around a perch using the level clock, including the initial phase.
    struct PoleFlight {
        Vector2 perch;
        float rhythm;
        float phase;
    };

    // Level 2: patrol horizontally and swoop when the player is nearby.
    struct PatrolFlight {
        Vector2 position;
        float speed;
        float left;
        float right;
        float swoopDelay;
        bool facingRight = true;
    };

    // Shared bird presentation and flight state. Levels retain their own hit response.
    // Collision sizes belong to the flight modes and never depend on the sprite sheet.
    class Bird {
    public:
        explicit Bird(PoleFlight flight);
        explicit Bird(PatrolFlight flight);

        void Reset();
        void Update(float dt, float levelSeconds, Rectangle playerBounds);
        // Patrol birds end their swoop on contact and can hit again after one second.
        bool TryAttack(Rectangle playerBounds);
        bool OnContact(); // Response after shared collision detection.
        void SubmitSprites(RenderQueue &queue, const AssetManager &assets, float pixelsPerUnit) const;

        Rectangle Bounds() const { return bounds; }
        Facing Direction() const { return facing; }
        const Animator &Animation() const { return animator; }

    private:
        void UpdatePole(float levelSeconds);
        void UpdatePatrol(float dt, Rectangle playerBounds);

        std::variant<PoleFlight, PatrolFlight> flight;
        Rectangle bounds{};
        Facing facing = Facing::Right;
        Animator animator;
        float swoopCooldown = 0;
        float swoopTimer = 0;
        float attackCooldown = 0;
        bool swooping = false;
    };
}
