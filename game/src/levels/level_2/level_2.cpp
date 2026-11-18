#include "levels/level_2/level_2.h"

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
            .world = {0, 0, 1000, 1500},
            .spawn = {180, 1406},
            .objects = {
                {.id = "floor", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {0, 1450}, .size = {1000, 50}, .assetId = "platform"},
                {.id = "stair-1", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {300, 1345}, .size = {200, 24}, .assetId = "platform"},
                {.id = "stair-2", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {520, 1240}, .size = {200, 24}, .assetId = "platform"},
                {.id = "stair-3", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {300, 1135}, .size = {200, 24}, .assetId = "platform"},
                {.id = "stair-4", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {520, 1030}, .size = {200, 24}, .assetId = "platform"},
                {.id = "stair-5", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {300, 925}, .size = {200, 24}, .assetId = "platform"},
                {.id = "stair-6", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {520, 820}, .size = {200, 24}, .assetId = "platform"},
                {.id = "stair-7", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {300, 715}, .size = {200, 24}, .assetId = "platform"},
                {.id = "stair-8", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {520, 610}, .size = {200, 24}, .assetId = "platform"},
                {.id = "stair-9", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {300, 505}, .size = {200, 24}, .assetId = "platform"},
                {.id = "stair-10", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {520, 400}, .size = {200, 24}, .assetId = "platform"},
                {.id = "stair-11", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {300, 295}, .size = {200, 24}, .assetId = "platform"},
            },
        };
        return definition;
    }

    Level2::Level2() : Level(Level2Definition()) {
        for (auto &event : CreateEvents()) {
            Events().RegisterEvent(std::move(event));
        }
    }

    std::unique_ptr<Level> MakeLevel2() { return std::make_unique<Level2>(); }
}
