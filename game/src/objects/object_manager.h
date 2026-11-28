#pragma once

#include "engine/assets/asset_manager.h"
#include "engine/assets/render_queue.h"
#include "objects/game_object.h"
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace rage {
    // Owns every object in the active level. Levels and events talk to
    // objects through this, never by holding raw pointers across a
    // level unload — Clear() and Remove() invalidate pointers returned
    // by Create() and Get(). Look objects up again by id after either.
    //
    // This is the interface the EventManager side uses: events call
    // Activate / Deactivate / Spawn / Remove / StartFall / SetState
    // and read state back with IsActive / IsTriggered, without
    // touching object internals. Object-specific behaviour (how a
    // falling object falls, how a moving one ping-pongs) stays in the
    // object classes, never here.
    class ObjectManager {
    public:
        // Creates an object from a definition. Returns nullptr if an
        // object with the same id already exists. The concrete class
        // comes from the definition: Moving objects and falling objects
        // get their own behaviour; everything else uses GameObject.
        GameObject *Create(const GameObjectDefinition &definition);
        // Spawning during gameplay is the same as creating: a new
        // instance from a reusable definition.
        GameObject *Spawn(const GameObjectDefinition &definition) { return Create(definition); }

        // Lookup by unique instance id. Returns nullptr if not found.
        GameObject *Get(const std::string &id);
        const GameObject *Get(const std::string &id) const;

        // Removes an object. Returns false if the id was not found.
        bool Remove(const std::string &id);

        // Event-facing state changes. All return false if the id was
        // not found, so an event can check the result.
        bool Activate(const std::string &id);
        bool Deactivate(const std::string &id);
        bool SetState(const std::string &id, std::string_view state);
        // Starts a falling object's fall (e.g. an anvil). Returns
        // false for a missing id or a non-falling object.
        bool StartFall(const std::string &id);
        // Condition checks for events.
        bool IsActive(const std::string &id) const;
        bool IsTriggered(const std::string &id) const;

        void UpdateAll(float dt);
        std::vector<Collider> Colliders(std::size_t extraCapacity = 0) const;
        // Queues every visible object's sprite on its Game Data layer.
        // `placeholder` is passed through to each object (see
        // GameObject::SubmitSprites).
        void SubmitSprites(RenderQueue &queue, const AssetManager &assets,
                           const PlaceholderDrawer &placeholder = {}) const;
        void ResetAll();
        void Clear();

        // Solid collision bounds, for the Player. Rebuilt on every call
        // so moving objects update their bounds as they move.
        std::vector<Rectangle> SolidBounds(CollisionType collision = CollisionType::Solid) const;

        // Objects of the given collision type overlapping bounds.
        // Used by Level for hazards (damage / respawn) and triggers
        // (checkpoints, exits, scripted events).
        std::vector<GameObject *> Overlapping(Rectangle bounds, CollisionType collision);

        const std::vector<std::unique_ptr<GameObject>> &Objects() const { return objects; }

    private:
        std::vector<std::unique_ptr<GameObject>> objects;
        std::unordered_map<std::string, Rectangle> previousBounds;
        bool removedSinceSnapshot = false;
    };
}
