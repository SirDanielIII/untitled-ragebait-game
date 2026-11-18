#include "objects/object_manager.h"
#include "objects/moving_object.h"
#include <algorithm>

namespace rage {
    GameObject *ObjectManager::Create(const GameObjectDefinition &definition) {
        if (Get(definition.id) != nullptr) {
            return nullptr;  // Duplicate id — ids must be unique per level.
        }
        std::unique_ptr<GameObject> object;
        if (definition.falls) {
            object = std::make_unique<FallingObject>(definition);
        } else if (definition.type == ObjectType::Moving) {
            object = std::make_unique<MovingObject>(definition);
        } else {
            object = std::make_unique<GameObject>(definition);
        }
        GameObject *raw = object.get();
        objects.push_back(std::move(object));
        return raw;
    }

    GameObject *ObjectManager::Get(const std::string &id) {
        for (auto &object : objects) {
            if (object->Id() == id) {
                return object.get();
            }
        }
        return nullptr;
    }

    const GameObject *ObjectManager::Get(const std::string &id) const {
        for (const auto &object : objects) {
            if (object->Id() == id) {
                return object.get();
            }
        }
        return nullptr;
    }

    bool ObjectManager::Remove(const std::string &id) {
        const auto it = std::remove_if(objects.begin(), objects.end(),
                                       [&id](const std::unique_ptr<GameObject> &object) {
                                           return object->Id() == id;
                                       });
        if (it == objects.end()) {
            return false;
        }
        objects.erase(it, objects.end());
        return true;
    }

    bool ObjectManager::Activate(const std::string &id) {
        GameObject *object = Get(id);
        if (!object) {
            return false;
        }
        object->SetActive(true);
        return true;
    }

    bool ObjectManager::Deactivate(const std::string &id) {
        GameObject *object = Get(id);
        if (!object) {
            return false;
        }
        object->SetActive(false);
        return true;
    }

    bool ObjectManager::SetState(const std::string &id, std::string_view state) {
        GameObject *object = Get(id);
        if (!object) {
            return false;
        }
        object->SetState(state);
        return true;
    }

    bool ObjectManager::StartFall(const std::string &id) {
        GameObject *object = Get(id);
        if (!object) {
            return false;
        }
        auto *falling = dynamic_cast<FallingObject *>(object);
        if (!falling) {
            return false;
        }
        falling->StartFall();
        return true;
    }

    bool ObjectManager::IsActive(const std::string &id) const {
        const GameObject *object = Get(id);
        return object != nullptr && object->IsActive();
    }

    bool ObjectManager::IsTriggered(const std::string &id) const {
        const GameObject *object = Get(id);
        return object != nullptr && object->IsTriggered();
    }

    void ObjectManager::UpdateAll(float dt) {
        for (auto &object : objects) {
            if (object->IsActive()) {
                object->Update(dt);
            }
        }
    }

    void ObjectManager::SubmitSprites(RenderQueue &queue, const AssetManager &assets,
                                      const PlaceholderDrawer &placeholder) const {
        for (const auto &object : objects) {
            object->SubmitSprites(queue, assets, placeholder);
        }
    }

    void ObjectManager::ResetAll() {
        for (auto &object : objects) {
            object->Reset();
        }
    }

    void ObjectManager::Clear() {
        objects.clear();
    }

    std::vector<Rectangle> ObjectManager::SolidBounds() const {
        std::vector<Rectangle> bounds;
        for (const auto &object : objects) {
            if (object->IsActive() && object->Collision() == CollisionType::Solid) {
                bounds.push_back(object->CollisionBounds());
            }
        }
        return bounds;
    }

    std::vector<GameObject *> ObjectManager::Overlapping(Rectangle bounds, CollisionType collision) {
        std::vector<GameObject *> found;
        for (auto &object : objects) {
            if (!object->IsActive() || object->Collision() != collision) {
                continue;
            }
            if (CheckCollisionRecs(bounds, object->CollisionBounds())) {
                found.push_back(object.get());
            }
        }
        return found;
    }
}
