#include "objects/bird.h"

#include "engine/graphics/placeholder_art.h"

#include <algorithm>
#include <cmath>

namespace rage {
    Bird::Bird(PoleFlight flight) : flight(flight) { Reset(); }
    Bird::Bird(PatrolFlight flight) : flight(flight) { Reset(); }

    void Bird::Reset() {
        swoopTimer = 0;
        attackCooldown = 0;
        swooping = false;
        animator = {};
        animator.SetState("fly");
        if (const auto *patrol = std::get_if<PatrolFlight>(&flight)) {
            bounds = {patrol->position.x, patrol->position.y, 38, 22};
            facing = patrol->facingRight ? Facing::Right : Facing::Left;
            swoopCooldown = patrol->swoopDelay;
        } else {
            swoopCooldown = 0;
            UpdatePole(0);
        }
    }

    void Bird::Update(float dt, float levelSeconds, Rectangle playerBounds) {
        if (!std::isfinite(dt) || dt < 0 || !std::isfinite(levelSeconds)) return;
        if (std::holds_alternative<PoleFlight>(flight)) {
            UpdatePole(levelSeconds);
        } else if (dt > 0) {
            UpdatePatrol(dt, playerBounds);
        }
        animator.SetState(swooping ? "swoop" : "fly");
        animator.Update(dt);
    }

    void Bird::UpdatePole(float levelSeconds) {
        const auto &pole = std::get<PoleFlight>(flight);
        const float phase = levelSeconds * pole.rhythm + pole.phase;
        bounds = {pole.perch.x + 84 * std::sin(phase),
                  pole.perch.y - 50 * std::abs(std::sin(phase * 2)), 38, 27};
        facing = std::cos(phase) >= 0 ? Facing::Right : Facing::Left;
    }

    void Bird::UpdatePatrol(float dt, Rectangle playerBounds) {
        const auto &patrol = std::get<PatrolFlight>(flight);
        swoopCooldown -= dt;
        attackCooldown = std::max(0.0f, attackCooldown - dt);
        bounds.x += (facing == Facing::Right ? 1.0f : -1.0f) * patrol.speed * dt;
        if (bounds.x >= patrol.right) {
            bounds.x = patrol.right;
            facing = Facing::Left;
        } else if (bounds.x <= patrol.left) {
            bounds.x = patrol.left;
            facing = Facing::Right;
        }

        const float distanceX = std::abs((playerBounds.x + playerBounds.width / 2.0f)
                                       - (bounds.x + bounds.width / 2.0f));
        const float distanceY = std::abs((playerBounds.y + playerBounds.height / 2.0f)
                                       - (bounds.y + bounds.height / 2.0f));
        if (!swooping && swoopCooldown <= 0 && distanceX < 260 && distanceY < 220) {
            swooping = true;
            swoopTimer = 0;
            swoopCooldown = 3.5f;
        }
        if (swooping) {
            swoopTimer += dt;
            const float progress = std::clamp(swoopTimer / 0.8f, 0.0f, 1.0f);
            bounds.y = patrol.position.y + std::sin(progress * PI) * 100;
            if (progress >= 1) {
                swooping = false;
                bounds.y = patrol.position.y;
            }
        }
    }

    bool Bird::TryAttack(Rectangle playerBounds) {
        const auto *patrol = std::get_if<PatrolFlight>(&flight);
        if (!patrol || attackCooldown > 0 || !CheckCollisionRecs(bounds, playerBounds)) return false;
        attackCooldown = 1;
        swooping = false;
        bounds.y = patrol->position.y;
        animator.SetState("fly");
        return true;
    }

    void Bird::SubmitSprites(RenderQueue &queue, const AssetManager &assets, float pixelsPerUnit) const {
        const float seconds = static_cast<float>(animator.Seconds());
        const bool facingRight = facing == Facing::Right;
        const auto placeholder = [seconds, facingRight](Rectangle target, bool) {
            art::Bird(target, facingRight, seconds);
        };
        const AnimationData *animation = assets.Find("bird", animator.State());
        if (!animation) animation = assets.Find("bird", "fly");
        if (!animation) animation = assets.Find("bird", "");
        if (!animation) {
            Rectangle target = bounds;
            target.x = std::round(target.x * pixelsPerUnit) / pixelsPerUnit;
            target.y = std::round(target.y * pixelsPerUnit) / pixelsPerUnit;
            queue.Submit(Layer::Middle, [target, placeholder] { placeholder(target, false); });
            return;
        }
        float pivotX = 0;
        float pivotY = 0;
        PivotOffset(animation->pivot, bounds.width, bounds.height, pivotX, pivotY);
        const Vector2 pivot{std::round((bounds.x + pivotX) * pixelsPerUnit) / pixelsPerUnit,
                            std::round((bounds.y + pivotY) * pixelsPerUnit) / pixelsPerUnit};
        queue.Submit(assets, *animation, animator.Frame(*animation), pivot, facing, placeholder);
    }
}
