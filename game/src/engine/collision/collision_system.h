#pragma once

#include "raylib.h"
#include <cstdint>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace rage {
    class Resources;
    enum class CollisionType { None, Solid, Hazard, Trigger, OneWay };
    namespace collisionLayers {
        inline constexpr std::uint32_t player = 1, environment = 2, all = ~0u;
    }
    struct CollisionFilter {
        std::uint32_t layer = collisionLayers::player;
        std::uint32_t mask = collisionLayers::all;
    };
    struct Collider {
        std::string id;
        Rectangle bounds{};
        Rectangle previous{};
        CollisionType type = CollisionType::Solid;
        CollisionFilter filter{collisionLayers::environment, collisionLayers::all};
        // Some non-blocking contacts (birds) are physical obstacles, unlike ordinary triggers.
        bool obstacleSensor = false;

        bool IsDebugObstacle() const {
            return bounds.width > 0 && bounds.height > 0 &&
                   (type == CollisionType::Solid || type == CollisionType::OneWay ||
                    type == CollisionType::Hazard || (type == CollisionType::Trigger && obstacleSensor));
        }
    };
    struct CollisionContact {
        std::string objectId;
        CollisionType type = CollisionType::None;
        Vector2 normal{}; // Points from the obstacle towards the body.
    };
    enum class ContactPhase { Enter, Exit };
    struct CollisionEvent {
        CollisionContact contact;
        ContactPhase phase = ContactPhase::Enter;
    };
    struct CollisionResult {
        bool grounded = false;
        bool crushed = false;
        std::vector<CollisionContact> contacts;
    };

    // Kinematic AABBs. Call once per shared simulation step after objects move.
    // Bounds are world-space rectangles; art and object ownership are independent.
    class CollisionSystem {
    public:
        void SetColliders(std::vector<Collider> value) { colliders = std::move(value); }
        const std::vector<Collider> &Colliders() const { return colliders; }
        void SetStaticScene(std::span<const Rectangle> solids, Rectangle world,
                            std::span<const Rectangle> oneWay = {});
        CollisionResult Move(Rectangle &body, Vector2 &velocity, float dt,
                             CollisionFilter filter = {}, const std::string &self = {}) const;
        CollisionSystem Slice(float from, float to) const;
        bool Supported(Rectangle body, CollisionFilter filter = {}, bool previous = true) const;
        bool CanOccupy(Rectangle body, CollisionFilter filter = {}, bool previous = true) const;
        bool SnapToWall(Rectangle &body, float distance, CollisionFilter filter = {}) const;
        std::vector<CollisionEvent> TrackContacts(const std::string &bodyId,
                                                const std::vector<CollisionContact> &contacts);
        void Reset() { contactsByBody.clear(); colliders.clear(); }
        void DrawDebug(const Resources &resources) const;
        static bool Overlaps(Rectangle a, Rectangle b);
        static bool TouchesDuringMove(Rectangle from, Rectangle to, Rectangle otherFrom, Rectangle otherTo);

    private:
        std::vector<Collider> colliders;
        std::unordered_map<std::string, std::vector<CollisionContact>> contactsByBody;
    };
}
