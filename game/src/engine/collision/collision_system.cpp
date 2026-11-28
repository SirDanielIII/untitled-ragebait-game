#include "engine/collision/collision_system.h"
#include "engine/resources/resources.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace rage {
    namespace {
        constexpr float epsilon = 0.001f;
        bool Matches(CollisionFilter a, CollisionFilter b) {
            return (a.mask & b.layer) != 0 && (b.mask & a.layer) != 0;
        }
        Rectangle At(const Collider &c, float t) {
            return {std::lerp(c.previous.x, c.bounds.x, t), std::lerp(c.previous.y, c.bounds.y, t),
                    c.bounds.width, c.bounds.height};
        }
        bool Top(Rectangle a, Rectangle b) {
            return a.x < b.x + b.width && a.x + a.width > b.x &&
                   std::abs(a.y + a.height - b.y) <= 0.01f;
        }
        struct Hit { float time = 2; Vector2 normal{}; };
        Hit Sweep(Rectangle a, Rectangle b, Vector2 delta) {
            float entry = -std::numeric_limits<float>::infinity();
            float leave = std::numeric_limits<float>::infinity();
            Vector2 normal{};
            for (int axis = 0; axis < 2; ++axis) {
                const float low = axis == 0 ? a.x : a.y;
                const float high = low + (axis == 0 ? a.width : a.height);
                const float otherLow = axis == 0 ? b.x : b.y;
                const float otherHigh = otherLow + (axis == 0 ? b.width : b.height);
                const float d = axis == 0 ? delta.x : delta.y;
                if (std::abs(d) < 1e-8f) {
                    if (high <= otherLow || low >= otherHigh) return {};
                    continue;
                }
                const float near = (d > 0 ? otherLow - high : otherHigh - low) / d;
                const float far = (d > 0 ? otherHigh - low : otherLow - high) / d;
                if (near > entry) {
                    entry = near;
                    normal = axis == 0 ? Vector2{d > 0 ? -1.0f : 1.0f, 0} : Vector2{0, d > 0 ? -1.0f : 1.0f};
                }
                leave = std::min(leave, far);
            }
            if (entry < -epsilon || entry > 1 || entry > leave || leave < 0) return {};
            return {std::max(0.0f, entry), normal};
        }
        void Add(CollisionResult &result, const Collider &c, Vector2 normal = {}) {
            if (std::ranges::none_of(result.contacts, [&](const auto &v) { return v.objectId == c.id; }))
                result.contacts.push_back({c.id, c.type, normal});
        }
    }

    bool CollisionSystem::Overlaps(Rectangle a, Rectangle b) { return CheckCollisionRecs(a, b); }

    bool CollisionSystem::TouchesDuringMove(Rectangle from, Rectangle to, Rectangle otherFrom, Rectangle otherTo) {
        return Overlaps(from, otherFrom) || Overlaps(to, otherTo) ||
            Sweep(from, otherFrom, {to.x - from.x - otherTo.x + otherFrom.x,
                                    to.y - from.y - otherTo.y + otherFrom.y}).time <= 1;
    }

    void CollisionSystem::SetStaticScene(std::span<const Rectangle> solids, Rectangle world,
                                          std::span<const Rectangle> oneWay) {
        colliders.clear();
        for (auto r : solids) colliders.push_back({"solid-" + std::to_string(colliders.size()), r, r});
        for (auto r : oneWay) colliders.push_back({"one-way-" + std::to_string(colliders.size()), r, r, CollisionType::OneWay});
        const Rectangle edges[] = {{world.x - 100, world.y - 100, 100, world.height + 200},
            {world.x + world.width, world.y - 100, 100, world.height + 200},
            {world.x, world.y - 100, world.width, 100}};
        for (auto r : edges) colliders.push_back({"boundary-" + std::to_string(colliders.size()), r, r});
    }

    CollisionSystem CollisionSystem::Slice(float from, float to) const {
        CollisionSystem result;
        for (const auto &c : colliders) {
            auto slice = c;
            slice.previous = At(c, from);
            slice.bounds = At(c, to);
            result.colliders.push_back(std::move(slice));
        }
        return result;
    }

    bool CollisionSystem::Supported(Rectangle body, CollisionFilter filter, bool previous) const {
        return std::ranges::any_of(colliders, [&](const auto &c) {
            return Matches(filter, c.filter) && (c.type == CollisionType::Solid || c.type == CollisionType::OneWay)
                && Top(body, previous ? c.previous : c.bounds);
        });
    }
    bool CollisionSystem::CanOccupy(Rectangle body, CollisionFilter filter, bool previous) const {
        return std::ranges::none_of(colliders, [&](const auto &c) {
            return c.type == CollisionType::Solid && Matches(filter, c.filter) && Overlaps(body, previous ? c.previous : c.bounds);
        });
    }
    bool CollisionSystem::SnapToWall(Rectangle &body, float distance, CollisionFilter filter) const {
        for (const auto &c : colliders) {
            const auto p = c.previous;
            if (c.type != CollisionType::Solid || !Matches(filter, c.filter) || body.y >= p.y + p.height || body.y + body.height <= p.y) continue;
            if (std::abs(body.x + body.width - p.x) <= distance) { body.x = p.x - body.width; return true; }
            if (std::abs(body.x - p.x - p.width) <= distance) { body.x = p.x + p.width; return true; }
        }
        return false;
    }

    CollisionResult CollisionSystem::Move(Rectangle &body, Vector2 &velocity, float dt,
                                           CollisionFilter filter, const std::string &self) const {
        CollisionResult result;
        if (!std::isfinite(dt) || dt < 0) return result;
        const auto eligible = [&](const Collider &c) { return c.id != self && Matches(filter, c.filter) && c.type != CollisionType::None; };
        Vector2 delta{velocity.x * dt, velocity.y * dt};
        // Carry only a supported, non-jumping body. The carrier's displacement is
        // resolved against other solids too (including ceilings and side walls).
        if (velocity.y >= 0) for (const auto &c : colliders) {
            if (eligible(c) && (c.type == CollisionType::Solid || c.type == CollisionType::OneWay) && Top(body, c.previous)) {
                delta.x += c.bounds.x - c.previous.x;
                delta.y += c.bounds.y - c.previous.y;
                break;
            }
        }
        // Recover spawned/teleported bodies with minimum translations, even at rest.
        const auto separate = [&](bool end) {
            for (int pass = 0; pass < 16; ++pass) {
                bool changed = false;
                for (const auto &c : colliders) {
                    if (!eligible(c) || c.type != CollisionType::Solid) continue;
                    const auto b = end ? c.bounds : c.previous;
                    if (!Overlaps(body, b)) continue;
                    const float shifts[] = {b.x - body.x - body.width, b.x + b.width - body.x,
                                            b.y - body.y - body.height, b.y + b.height - body.y};
                    int best = 0;
                    for (int i = 1; i < 4; ++i) if (std::abs(shifts[i]) < std::abs(shifts[best])) best = i;
                    Vector2 n{};
                    if (best < 2) { body.x += shifts[best]; n.x = best == 0 ? -1.0f : 1.0f; velocity.x = 0; }
                    else { body.y += shifts[best]; n.y = best == 2 ? -1.0f : 1.0f; velocity.y = 0; }
                    Add(result, c, n);
                    changed = true;
                }
                if (!changed) break;
            }
        };
        separate(false);
        float time = 0;
        for (int iteration = 0; iteration < 16 && time < 1; ++iteration) {
            Hit nearest;
            const Collider *obstacle = nullptr;
            for (const auto &c : colliders) {
                if (!eligible(c) || (c.type != CollisionType::Solid && c.type != CollisionType::OneWay)) continue;
                const auto b = At(c, time);
                const Vector2 relative{delta.x - (c.bounds.x - b.x), delta.y - (c.bounds.y - b.y)};
                const auto hit = Sweep(body, b, relative);
                if (c.type == CollisionType::OneWay && (hit.normal.y != -1 || body.y + body.height > b.y + epsilon)) continue;
                if (hit.time < nearest.time) { nearest = hit; obstacle = &c; }
            }
            const float fraction = obstacle ? nearest.time : 1;
            Rectangle next = body;
            next.x += delta.x * fraction;
            next.y += delta.y * fraction;
            const float nextTime = time + (1 - time) * fraction;
            for (const auto &c : colliders) {
                if (eligible(c) && (c.type == CollisionType::Hazard || c.type == CollisionType::Trigger) &&
                    TouchesDuringMove(body, next, At(c, time), At(c, nextTime))) Add(result, c);
            }
            body = next;
            time = nextTime;
            if (!obstacle) break;
            Add(result, *obstacle, nearest.normal);
            delta.x *= 1 - fraction;
            delta.y *= 1 - fraction;
            if (nearest.normal.x != 0) { delta.x = (obstacle->bounds.x - obstacle->previous.x) * (1 - time); velocity.x = 0; }
            if (nearest.normal.y != 0) { delta.y = (obstacle->bounds.y - obstacle->previous.y) * (1 - time); velocity.y = 0; }
        }
        // Opposing moving constraints can leave no feasible displacement. Report
        // the squeeze before overlap recovery can eject the body through a wall.
        result.crushed = time < 1;
        separate(true);
        for (const auto &c : colliders) {
            if (!eligible(c)) continue;
            if (c.type == CollisionType::Solid && Overlaps(body, c.bounds)) result.crushed = true;
            if ((c.type == CollisionType::Solid || c.type == CollisionType::OneWay) && Top(body, c.bounds) && velocity.y >= 0) {
                result.grounded = true;
                velocity.y = 0;
                Add(result, c, {0, -1});
            }
            if (c.type == CollisionType::Solid) {
                auto expanded = body;
                expanded.x -= epsilon; expanded.y -= epsilon;
                expanded.width += 2 * epsilon; expanded.height += 2 * epsilon;
                if (Overlaps(expanded, c.bounds)) Add(result, c);
            }
        }
        return result;
    }

    std::vector<CollisionEvent> CollisionSystem::TrackContacts(const std::string &bodyId, const std::vector<CollisionContact> &contacts) {
        auto &previous = contactsByBody[bodyId];
        std::vector<CollisionEvent> events;
        const auto contains = [](const auto &list, const auto &c) {
            return std::ranges::any_of(list, [&](const auto &v) { return v.objectId == c.objectId && v.type == c.type; });
        };
        for (const auto &c : contacts) if (!contains(previous, c)) events.push_back({c, ContactPhase::Enter});
        for (const auto &c : previous) if (!contains(contacts, c)) events.push_back({c, ContactPhase::Exit});
        previous = contacts;
        return events;
    }

    void CollisionSystem::DrawDebug(const Resources &resources) const {
        for (const auto &c : colliders) {
            const Color color = c.type == CollisionType::Hazard ? RED : c.type == CollisionType::Trigger ? YELLOW :
                                c.type == CollisionType::None ? GRAY : c.type == CollisionType::OneWay ? SKYBLUE : GREEN;
            DrawRectangleLinesEx(c.bounds, 1, color);
            resources.DrawText(c.id, {c.bounds.x, c.bounds.y - 10}, 8, 1, color);
        }
    }
}
