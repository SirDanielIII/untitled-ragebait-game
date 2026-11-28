#pragma once

#include "objects/game_object.h"

namespace rage {
    // A platform or obstacle that moves back and forth within its
    // definition range, reversing direction at each end.
    class MovingObject final : public GameObject {
    public:
        explicit MovingObject(GameObjectDefinition definition);
        void Update(float dt) override;
    };

    // An object that stays put until StartFall is called (by a trigger
    // or an event), then falls straight down by its definition range
    // and stops. An anvil reaching the ground is the intended use.
    // Landing marks the action complete; Level posts
    // objectEvents::actionCompleted for it once.
    class FallingObject final : public GameObject {
    public:
        explicit FallingObject(GameObjectDefinition definition);
        void Update(float dt) override;
        void StartFall();
        void OnBlocked(const CollisionResult &result) override;
        bool HasLanded() const { return landed; }
        void Reset() override;

    private:
        bool falling = false;
        bool landed = false;
    };
}
