#pragma once

#include "engine/assets/animator.h"
#include "engine/assets/asset_manager.h"
#include "engine/assets/render_queue.h"
#include "raylib.h"
#include <cstdint>
#include <functional>
#include <string>

namespace rage {
    enum class ObjectType {
        Static,
        Moving,
        Hazard,
        Interactive,
        Trigger
    };

    enum class CollisionType {
        None,
        Solid,
        Hazard,
        Trigger
    };

    // Topics objects post to the level's EventManager (see
    // levels/event_manager.h). An event matches a topic and the
    // sourceId below, exactly like the dialogue milestones do.
    namespace objectEvents {
        inline constexpr char triggerEntered[] = "object.trigger_entered";
        inline constexpr char hazardHit[] = "object.hazard_hit";
        inline constexpr char activated[] = "object.activated";
        inline constexpr char actionCompleted[] = "object.action_completed";
    }

    // EventNotification.sourceId for an object id. The EventManager
    // only carries a number, so ids are hashed to one.
    inline std::uint64_t ObjectNotificationId(const std::string &id) {
        return static_cast<std::uint64_t>(std::hash<std::string>{}(id));
    }

    // Data for one object. Levels list these; they do not create a new
    // class for every obstacle. The same definition can be spawned many
    // times with different positions, sizes and speeds.
    struct GameObjectDefinition {
        std::string id;
        ObjectType type = ObjectType::Static;
        CollisionType collision = CollisionType::Solid;
        Vector2 position{0, 0};
        Vector2 size{0, 0};
        Vector2 velocity{0, 0};
        // Movement range from the initial position. A moving object
        // ping-pongs within it; a falling object falls by range.y.
        Vector2 range{0, 0};
        // Set to true for objects that only start moving once triggered
        // (for example, a falling anvil above the player).
        bool falls = false;

        // Collision box, relative to position. If collisionSize is
        // {0, 0} it defaults to size. Collision is independent of art,
        // so replacing artwork never changes gameplay.
        Vector2 collisionOffset{0, 0};
        Vector2 collisionSize{0, 0};

        // Game Data Asset ID (the Asset ID column of the spreadsheet),
        // e.g. "platform", "anvil". The animation for the object's
        // current state is "<assetId>_<state>" (see AssetManager::Find).
        // Empty means intentionally invisible: no sprite is queued, but
        // the object still collides / triggers. A named asset with no
        // Game Data row yet draws the placeholder instead.
        std::string assetId;
        // Animation state to start in, e.g. "idle". Animator keeps the
        // current state; SetState changes it.
        std::string initialState = "idle";

        bool active = true;
        bool visible = true;
        // Triggers and one-shot hazards deactivate after firing once.
        bool triggerOnce = false;
    };

    // Shared base for every game object. Objects only carry the
    // behaviour they need: a static platform uses the base Update
    // (velocity {0, 0}, so it does nothing), moving and falling
    // objects override it in moving_object.*.
    class GameObject {
    public:
        explicit GameObject(GameObjectDefinition definition);
        virtual ~GameObject() = default;

        virtual void Update(float dt);
        // Queues the object's sprite on its Game Data layer; nothing
        // is drawn here, the RenderQueue flushes it in layer order.
        // `placeholder` is drawn by the AssetManager when the object's
        // texture is missing, and for objects with no Game Data row
        // yet. Rendering is separate from behaviour: Update never
        // draws, and this never moves the object.
        virtual void SubmitSprites(RenderQueue &queue, const AssetManager &assets,
                                   const PlaceholderDrawer &placeholder = {}) const;
        virtual void Reset();

        // Called when the player enters a trigger, or a trigger is
        // fired by an event. Marks the object triggered and, for
        // one-shot objects, deactivates it.
        virtual void OnTrigger();
        // Clears the "player is inside" flag when the player leaves a
        // repeatable trigger, so it can fire again. One-shot objects
        // are inactive after OnTrigger, so this never re-arms them.
        void ClearTrigger() { triggered = false; }

        // Set by object-specific behaviour when an action finishes
        // (e.g. a falling anvil reaches the ground). Level consumes
        // it once and posts objectEvents::actionCompleted.
        bool TakeActionCompleted() {
            const bool done = actionCompleted;
            actionCompleted = false;
            return done;
        }

        const std::string &Id() const { return definition.id; }
        ObjectType Type() const { return definition.type; }
        CollisionType Collision() const { return definition.collision; }

        Vector2 Position() const { return position; }
        void SetPosition(Vector2 value) { position = value; }
        Vector2 Size() const { return definition.size; }
        Vector2 Velocity() const { return velocity; }
        void SetVelocity(Vector2 value) { velocity = value; }
        Vector2 Range() const { return definition.range; }
        bool Falls() const { return definition.falls; }
        const std::string &AssetId() const { return definition.assetId; }
        const std::string &State() const { return animator.State(); }
        void SetState(std::string_view state) { animator.SetState(state); }

        // Visual bounds, at the current position.
        Rectangle Bounds() const;
        // Collision bounds, at the current position.
        Rectangle CollisionBounds() const;

        bool IsActive() const { return active; }
        void SetActive(bool value) { active = value; }
        bool IsVisible() const { return visible; }
        void SetVisible(bool value) { visible = value; }
        bool IsTriggered() const { return triggered; }

    protected:
        GameObjectDefinition definition;
        Vector2 position;
        Vector2 velocity;
        Animator animator;
        bool active;
        bool visible;
        bool triggered = false;
        bool actionCompleted = false;
        Vector2 initialPosition;
        Vector2 initialVelocity;
    };
}
