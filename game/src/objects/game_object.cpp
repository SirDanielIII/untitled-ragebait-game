#include "objects/game_object.h"

namespace rage {
    GameObject::GameObject(GameObjectDefinition definition)
        : definition(definition),
          position(definition.position),
          velocity(definition.velocity),
          active(definition.active),
          visible(definition.visible),
          initialPosition(definition.position),
          initialVelocity(definition.velocity) {
        animator.SetState(definition.initialState);
    }

    void GameObject::Update(float dt) {
        if (!active) {
            return;
        }
        position.x += velocity.x * dt;
        position.y += velocity.y * dt;
        animator.Update(dt);
    }

    void GameObject::SubmitSprites(RenderQueue &queue, const AssetManager &assets,
                                   const PlaceholderDrawer &placeholder) const {
        if (!active || !visible || definition.assetId.empty()) {
            return; // Inactive, hidden, or intentionally invisible.
        }
        // Animation by state, falling back to the asset's first row
        // (Find with an empty state). A missing row is not an error:
        // the placeholder below is drawn instead, at the same bounds.
        const AnimationData *animation = assets.Find(definition.assetId, animator.State());
        if (!animation) {
            animation = assets.Find(definition.assetId, "");
        }
        if (!animation) {
            if (!definition.drawPlaceholder) {
                return; // The level draws its own stand-in until the art exists.
            }
            const Rectangle bounds = Bounds();
            queue.Submit(Layer::Middle, [bounds, placeholder] {
                if (placeholder) {
                    placeholder(bounds, false);
                } else {
                    AssetManager::DrawPlaceholderBox(bounds, false);
                }
            });
            return;
        }
        // AssetManager::Draw places the frame's pivot at `position`,
        // so convert the top-left bounds to the pivot point the
        // Game Data row declares. Collision bounds are untouched.
        const Rectangle bounds = Bounds();
        Vector2 pivot = {bounds.x, bounds.y};
        if (animation->pivot == Pivot::BottomCentre) {
            pivot = {bounds.x + bounds.width / 2, bounds.y + bounds.height};
        } else if (animation->pivot == Pivot::Centre) {
            pivot = {bounds.x + bounds.width / 2, bounds.y + bounds.height / 2};
        }
        queue.Submit(assets, *animation, animator.Frame(*animation), pivot, Facing::None, placeholder);
    }

    void GameObject::Reset() {
        position = initialPosition;
        velocity = initialVelocity;
        active = definition.active;
        visible = definition.visible;
        triggered = false;
        actionCompleted = false;
        animator = {};
        animator.SetState(definition.initialState);
    }

    void GameObject::OnTrigger() {
        triggered = true;
        if (definition.triggerOnce) {
            active = false;
        }
    }

    Rectangle GameObject::Bounds() const {
        return Rectangle{position.x, position.y, definition.size.x, definition.size.y};
    }

    Rectangle GameObject::CollisionBounds() const {
        const Vector2 collisionSize =
            (definition.collisionSize.x > 0 && definition.collisionSize.y > 0)
                ? definition.collisionSize
                : definition.size;
        return Rectangle{position.x + definition.collisionOffset.x,
                         position.y + definition.collisionOffset.y,
                         collisionSize.x, collisionSize.y};
    }
}
