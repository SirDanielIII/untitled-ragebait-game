#include "levels/level_2/level_2.h"

#include <cmath>
#include <utility>

namespace rage {

    const LevelDefinition &Level2Definition() {
        static const LevelDefinition definition{
            .id = LevelId::Level2,
            .title = "Level 2",
            .description = "Climb the stairwell to the top-floor classroom.",
            .plannedContent = "Climb exposed wall bricks; watch for loose masonry. Full stairwell layout is still to be designed.",
            .intro = {
                .text = "The classroom is upstairs. The stairs appear to have other plans.\nThose bricks look perfectly load-bearing. I am almost certain.",
                .speaker = "The Entity",
                .speedMultiplier = 1.0f,
                .startDelay = 0.0f,
                .fontSize = 24.0f,
                .allowSkip = true,
                .allowReplay = true
            },
            .accent = {177, 143, 210, 255},
            .backdrop = ArtId::Level2,
            .world = {0, -120, 1000, 1570},
            .spawn = {180, 1406},

            .objects = {
                // STREET LEVEL
                {.id = "floor", .type = ObjectType::Static, .collision = CollisionType::Solid,
                 .position = {0, 1450}, .size = {1000, 50}, .assetId = "platform"},

                // LAMPPOST 1
                {.id = "lamppost-1", .type = ObjectType::Static, .collision = CollisionType::Solid,
                 .position = {120, 950}, .size = {45, 500}, .assetId = "platform"},

                // STREET AWNINGS
                {.id = "awning-1", .type = ObjectType::Static, .collision = CollisionType::Solid,
                 .position = {220, 1300}, .size = {120, 20}, .assetId = "platform"},

                {.id = "awning-2", .type = ObjectType::Static, .collision = CollisionType::Solid,
                 .position = {380, 1190}, .size = {110, 20}, .assetId = "platform"},

                {.id = "awning-3", .type = ObjectType::Static, .collision = CollisionType::Solid,
                 .position = {220, 1080}, .size = {100, 20}, .assetId = "platform"},

                // WINDOW LEDGES
                {.id = "window-ledge-1", .type = ObjectType::Static, .collision = CollisionType::Solid,
                 .position = {400, 1030}, .size = {90, 20}, .assetId = "platform"},

                {.id = "window-ledge-2", .type = ObjectType::Static, .collision = CollisionType::Solid,
                 .position = {530, 920}, .size = {90, 20}, .assetId = "platform"},

                // FIRE ESCAPE
                {.id = "fire-escape-1", .type = ObjectType::Static, .collision = CollisionType::Solid,
                 .position = {680, 940}, .size = {150, 20}, .assetId = "platform"},

                {.id = "fire-escape-2", .type = ObjectType::Static, .collision = CollisionType::Solid,
                 .position = {680, 800}, .size = {150, 20}, .assetId = "platform"},

                {.id = "fire-escape-3", .type = ObjectType::Static, .collision = CollisionType::Solid,
                 .position = {680, 660}, .size = {150, 20}, .assetId = "platform"},

                // CONNECTING WINDOW LEDGES
                {.id = "window-ledge-3", .type = ObjectType::Static, .collision = CollisionType::Solid,
                 .position = {490, 810}, .size = {100, 20}, .assetId = "platform"},

                {.id = "window-ledge-4", .type = ObjectType::Static, .collision = CollisionType::Solid,
                 .position = {480, 670}, .size = {100, 20}, .assetId = "platform"},

                // AIR CONDITIONING UNITS
                {.id = "ac-unit-1", .type = ObjectType::Static, .collision = CollisionType::Solid,
                 .position = {850, 870}, .size = {90, 30}, .assetId = "platform"},

                {.id = "ac-unit-2", .type = ObjectType::Static, .collision = CollisionType::Solid,
                 .position = {850, 560}, .size = {90, 30}, .assetId = "platform"},

                // LAMPPOST 2
                {.id = "lamppost-2", .type = ObjectType::Static, .collision = CollisionType::Solid,
                 .position = {900, 350}, .size = {40, 1100}, .assetId = "platform"},

                // UPPER LEDGES
                {.id = "upper-ledge-1", .type = ObjectType::Static, .collision = CollisionType::Solid,
                 .position = {730, 520}, .size = {100, 20}, .assetId = "platform"},

                {.id = "upper-ledge-2", .type = ObjectType::Static, .collision = CollisionType::Solid,
                 .position = {590, 420}, .size = {100, 20}, .assetId = "platform"},

                {.id = "upper-ledge-3", .type = ObjectType::Static, .collision = CollisionType::Solid,
                 .position = {740, 320}, .size = {100, 20}, .assetId = "platform"},

                // ROOFTOP CLIMB
                {.id = "roof-ledge-1", .type = ObjectType::Static, .collision = CollisionType::Solid,
                 .position = {560, 240}, .size = {100, 20}, .assetId = "platform"},

                {.id = "roof-ledge-2", .type = ObjectType::Static, .collision = CollisionType::Solid,
                 .position = {700, 150}, .size = {100, 20}, .assetId = "platform"},

                {.id = "rooftop", .type = ObjectType::Static, .collision = CollisionType::Solid,
                 .position = {350, 70}, .size = {200, 30}, .assetId = "platform"},
            },
            // Asset IDs for the objects above: level_2_sprites.cpp.
            .sprites = Level2Sprites(),
        };

        return definition;
    }

    Level2::Level2() : Level(Level2Definition()), birds{
        Bird(PatrolFlight{{250, 1200}, 150, 200, 520, 1.5f}),
        Bird(PatrolFlight{{650, 1080}, 170, 500, 850, 2.0f, false}),
        Bird(PatrolFlight{{400, 1000}, 160, 250, 650, 2.5f}),
        Bird(PatrolFlight{{500, 900}, 180, 350, 750, 2.5f}),
        Bird(PatrolFlight{{750, 760}, 190, 600, 900, 2.0f, false}),
        Bird(PatrolFlight{{300, 620}, 200, 200, 550, 2.5f}),
        Bird(PatrolFlight{{800, 580}, 210, 650, 950, 3.0f, false}),
        Bird(PatrolFlight{{700, 480}, 210, 550, 900, 3.0f, false}),
        Bird(PatrolFlight{{450, 350}, 220, 350, 750, 2.5f}),
        Bird(PatrolFlight{{650, 180}, 240, 500, 900, 3.5f, false})
    } {
        for (auto &event : CreateEvents()) {
            Events().RegisterEvent(std::move(event));
        }
    }

    void Level2::OnReset() {
        for (auto& bird : birds) bird.Reset();
    }

    void Level2::OnUpdate(float dt, const InputFrame &) {
        if (!std::isfinite(dt) || dt <= 0.0f) return;
        const Rectangle playerBounds = GetPlayer().Bounds();
        for (auto& bird : birds) {
            bird.Update(dt, ElapsedSeconds(), playerBounds);
            const Rectangle bounds = bird.Bounds();
            if (ScriptContact("patrol-bird-" + std::to_string(&bird - birds.data()), bounds, CollisionType::Trigger) && bird.OnContact()) {
                const float playerCenter = playerBounds.x + playerBounds.width / 2.0f;
                const float birdCenter = bounds.x + bounds.width / 2.0f;
                const float direction = playerCenter < birdCenter ? -1.0f : 1.0f;
                MutablePlayer().ApplyKnockback({direction * 420.0f, -280.0f});
            }
        }
    }

    void Level2::SubmitSprites(RenderQueue& queue, const Resources& resources) const {
        Level::SubmitSprites(queue, resources);
        for (const auto& bird : birds) bird.SubmitSprites(queue, resources.Assets(), Camera().zoom);
    }

    std::unique_ptr<Level> MakeLevel2() {
        return std::make_unique<Level2>();
    }

}
