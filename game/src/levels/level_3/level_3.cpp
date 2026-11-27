#include "levels/level_3/level_3.h"

#include <utility>
#include <cmath>
#include "engine/graphics/placeholder_art.h"
#include "levels/level_3/level_3_art.h"

namespace rage {
    const LevelDefinition &Level3Definition() {
        static const LevelDefinition definition{
            .id = LevelId::Level3,
            .title = "Level 3",
            .description = "Face the Entity outside the lecture hall.",
            .plannedContent = "Final encounter ideas: shifting floors, floating furniture, paper airplanes and wall climbs.",
            .intro = {
                .text = "Oh. You're... still here. Huh.\nThat's fine. That's totally fine.",
                .speaker = "The Entity",
                .speedMultiplier = 1.0f,
                .startDelay = 0.0f,
                .fontSize = 24.0f,
                .allowSkip = true,
                .allowReplay = true
            },
            .accent = {211, 133, 136, 255},
            .backdrop = ArtId::Level3,
            .world = {0, 0, 3200, 700},
            .spawn = {100, 606},
            .objects = {
                {.id = "floor", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {0, 650}, .size = {3200, 50}, .assetId = "platform"},
                 {.id = "spikes-1", .type = ObjectType::Hazard, .collision = CollisionType::Hazard, .position = {330, 640}, .size = {1150, 75}, .assetId = "spikes"},
                 {.id = "chair-1", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {470, 470}, .size = {40, 60}, .collisionOffset = {0, 30}, .collisionSize = {40, 30}, .assetId = "chair"},
                {.id = "chair-1-back", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {502, 470}, .size = {8, 30}},
                {.id = "table-1", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {250, 610}, .size = {80, 40}, .assetId = "table"},
                {.id = "chair-2", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {660, 420}, .size = {40, 60}, .collisionOffset = {0, 30}, .collisionSize = {40, 30}, .assetId = "chair"},
                {.id = "chair-2-back", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {692, 420}, .size = {8, 30}},
                {.id = "table-2", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {800, 500}, .size = {80, 40}, .assetId = "table"},
                {.id = "book-1", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {1050, 570}, .size = {10, 14}, .velocity = {0, 500}, .range = {0, 300}, .falls = true, .assetId = "book"},
                {.id = "book-2", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {1225, 520}, .size = {10, 14}, .velocity = {0, 500}, .range = {0, 300}, .falls = true, .assetId = "book"},
                {.id = "book-3", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {1400, 570}, .size = {10, 14}, .velocity = {0, 500}, .range = {0, 300}, .falls = true, .assetId = "book"},
                {.id = "pillar-1", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {1500, 350}, .size = {75, 150}, .assetId = "pillar"},
                {.id = "fall-1", .type = ObjectType::Hazard, .collision = CollisionType::Hazard, .position = {0, 0}, .size = {40, 40}, .velocity = {0, 700}, .range = {0, 610}, .falls = true, .assetId = "anvil", .active = false},
                {.id = "fall-2", .type = ObjectType::Hazard, .collision = CollisionType::Hazard, .position = {0, 0}, .size = {40, 40}, .velocity = {0, 700}, .range = {0, 610}, .falls = true, .assetId = "anvil", .active = false},
                {.id = "fall-3", .type = ObjectType::Hazard, .collision = CollisionType::Hazard, .position = {0, 0}, .size = {40, 40}, .velocity = {0, 700}, .range = {0, 610}, .falls = true, .assetId = "anvil", .active = false},
            }, .cameraZoom = 1.5f,
        };
        return definition;
    }

    Level3::Level3() : Level(Level3Definition()) {
        for (auto &event : CreateEvents()) {
            Events().RegisterEvent(std::move(event));
        }
    }


    namespace {
        constexpr float entityWidth = 100.0f;
        constexpr float entityHeight = 130.0f;
        constexpr Vector2 entityStart = {640.0f, 480.0f};
        constexpr Vector2 entityDoorSpot = {2700.0f, 480.0f};
        constexpr float entityLeaveTime = 4.0f;
        constexpr float entitySpeed = 450.0f;
        constexpr int bookCount = 3;
        constexpr float bookFallDelay = 0.8f;
        constexpr float phase1End = 1500.0f;
        constexpr float fallInterval = 1.1;
        constexpr float fallWarning = 0.8f;
        constexpr float fallObjectSize = 40.0f;
        constexpr float fallDuration = 610.0f / 700.0f;
        constexpr int fallPoolSize = 3;
        constexpr float fallPattern[] = {350, 585, 965, 400, 1167, 750, 1342, 585, 965, 1167};
        constexpr int fallPatternCount = 9;
        std::string FallId(int i) { return "fall-" + std::to_string(i + 1); }
    }

    void Level3::OnReset() {
        entityPosition = entityStart;
        bookTimers.fill(0.0f);
        ResetFallingObjects();
        deaths = 0;
    }

    void Level3::OnUpdate(float dt, const InputFrame &) {
        UpdateBooks(dt);
        UpdateFallingObjects(dt);
        if (ElapsedSeconds() < entityLeaveTime) return;
        const float dx = entityDoorSpot.x - entityPosition.x;
        const float dy = entityDoorSpot.y - entityPosition.y;
        const float distance = std::hypot(dx, dy);
        const float step = entitySpeed * dt;
        if (distance <= step) {
            entityPosition = entityDoorSpot;
        } else {
            entityPosition.x += dx / distance * step;
            entityPosition.y += dy / distance * step;
        }
    }

    void Level3::DrawWorld(const Resources &resources) const {

        const float hallX = Definition().world.width - 400;
        level3art::Hallway(Definition().world.width, hallX, ElapsedSeconds());
        level3art::LectureHall(hallX, ElapsedSeconds());
        const float bob = std::sin(ElapsedSeconds() * 2.0f) * 8.0f;
        art::Entity(resources, {entityPosition.x, entityPosition.y + bob, entityWidth, entityHeight});
    }


    float Level3::BookShake(const std::string &id) const {
        for (int i = 0; i < bookCount; ++i) {
            if (id != "book-" + std::to_string(i + 1)) continue;
            if (bookTimers[i] <= 0.0f || bookTimers[i] >= bookFallDelay) return 0.0f; // not counting down
            return std::sin(ElapsedSeconds() * 70.0f) * 2.0f; // moves between -2 and +2 pixels, very fast
        }
        return 0.0f;
    }

    void Level3::SubmitSprites(RenderQueue &queue, const Resources &resources) const {
        const float time = ElapsedSeconds();
        const Color accent = Definition().accent;
        for (const auto &object : Objects().Objects()) {
            if (!object->IsActive() || !object->IsVisible() || object->AssetId().empty()) continue;

            if (resources.Assets().Find(object->AssetId(), "")) {
                object->SubmitSprites(queue, resources.Assets());
                continue;
            }

            Rectangle bounds = object->Bounds();
            bounds.x += BookShake(object->Id());
            const std::string assetId = object->AssetId();
            const std::string id = object->Id();
            queue.Submit(Layer::Middle, [=, &resources] {
                if (!level3art::Object(assetId, id, bounds, time)) art::Platform(resources, bounds, accent);
            });
        }
    }

    void Level3::UpdateBooks(float dt) {
        const Rectangle body = GetPlayer().Bounds();
        for (int i = 0; i < bookCount; ++i) {
            const std::string id = "book-" + std::to_string(i + 1);
            GameObject *book = Objects().Get(id);
            if (!book) continue;
            const Rectangle top = book->CollisionBounds();
            const bool standing = GetPlayer().Grounded()
                                  && std::abs((body.y + body.height) - top.y) < 2.0f
                                  && body.x + body.width > top.x
                                  && body.x < top.x + top.width;

            if (standing && bookTimers[i] == 0.0f) bookTimers[i] = 0.0001f;
            if (bookTimers[i] > 0.0f) {
                bookTimers[i] += dt;
                if (bookTimers[i] >= bookFallDelay) StartObjectFall(id);
            }
        }
    }

    void Level3::ResetBooks() {
        bookTimers.fill(0.0f);
        for (int i = 0; i < bookCount; ++i) {
            if (GameObject *book = Objects().Get("book-" + std::to_string(i + 1))) book->Reset();
        }
    }

    void Level3::UpdateFallingObjects(float dt) {
        for (int i = 0; i < fallPoolSize; ++i) {
            if (fallAge[i] < 0.0f) continue;
            fallAge[i] += dt;
            if (fallAge[i] >= fallDuration + 0.15f) {
                if (GameObject *object = Objects().Get(FallId(i))) object->SetActive(false);
                fallAge[i] = -1.0f;
            }
        }
        if (GetPlayer().Position().x >= phase1End) return;
        fallTimer += dt;
        if (fallTimer < fallInterval) return;
        fallTimer -= fallInterval;
        const int slot = fallIndex % fallPoolSize;
        if (GameObject *object = Objects().Get(FallId(slot))) {
            object->Reset();
            object->SetPosition({fallPattern[fallIndex % fallPatternCount] - fallObjectSize / 2, object->Position().y});
            object->SetActive(true);
            StartObjectFall(FallId(slot));
            fallAge[slot] = 0.0f;
        }
        ++fallIndex;
    }

    void Level3::ResetFallingObjects() {
        fallTimer = 0.0f;
        fallIndex = 0;
        fallAge.fill(-1.0f);
        for (int i = 0; i < fallPoolSize; ++i) {
            if (GameObject *object = Objects().Get(FallId(i))) object->Reset();
        }
    }
    bool Level3::AnvilWarningShowing() const {
        return GetPlayer().Position().x < phase1End && fallTimer >= fallInterval - fallWarning;
    }

    bool Level3::AnyBookCounting() const {
        for (float timer : bookTimers) if (timer > 0.0f) return true;
        return false;
    }

    bool Level3::AnyBookFallen() const {
        for (float timer : bookTimers) if (timer >= bookFallDelay) return true;
        return false;
    }

    bool Level3::StandingOnChair() const {
        if (!GetPlayer().Grounded()) return false;
        const Rectangle body = GetPlayer().Bounds();
        for (const char *id : {"chair-1", "chair-2"}) {
            const GameObject *chair = Objects().Get(id);
            if (!chair) continue;
            const Rectangle seat = chair->CollisionBounds();
            if (std::abs(body.y + body.height - seat.y) < 2.0f && body.x + body.width > seat.x
                && body.x < seat.x + seat.width) return true;
        }
        return false;
    }

    std::unique_ptr<Level> MakeLevel3() { return std::make_unique<Level3>(); }
}
