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
            .world = {0, -120, 1000, 1570},
            .spawn = {180, 1406},
            .objects = {
                // Floor.
                {.id = "floor", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {0, 1450}, .size = {1000, 50}, .assetId = "platform"},
                // Section 1 - first pillar. The tall pillars are the
                // climbable walls; the small ledges are the steps between them.
                {.id = "pillar-1", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {280, 1120}, .size = {90, 330}, .assetId = "platform"},
                {.id = "ledge-1", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {400, 1110}, .size = {70, 20}, .assetId = "platform"},
                {.id = "ledge-2", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {150, 1280}, .size = {75, 20}, .assetId = "platform"},
                {.id = "ledge-3", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {390, 1250}, .size = {65, 20}, .assetId = "platform"},
                // Section 2 - second pillar.
                {.id = "pillar-2", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {620, 850}, .size = {90, 300}, .assetId = "platform"},
                {.id = "ledge-4", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {450, 950}, .size = {70, 20}, .assetId = "platform"},
                {.id = "ledge-5", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {760, 1020}, .size = {70, 20}, .assetId = "platform"},
                {.id = "ledge-6", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {470, 820}, .size = {65, 20}, .assetId = "platform"},
                // Section 3 - third pillar.
                {.id = "pillar-3", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {300, 570}, .size = {90, 280}, .assetId = "platform"},
                {.id = "ledge-7", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {160, 700}, .size = {70, 20}, .assetId = "platform"},
                {.id = "ledge-8", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {430, 650}, .size = {65, 20}, .assetId = "platform"},
                {.id = "ledge-9", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {160, 540}, .size = {65, 20}, .assetId = "platform"},
                // Section 4 - fourth pillar.
                {.id = "pillar-4", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {650, 300}, .size = {90, 270}, .assetId = "platform"},
                {.id = "ledge-10", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {480, 440}, .size = {70, 20}, .assetId = "platform"},
                {.id = "ledge-11", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {790, 480}, .size = {65, 20}, .assetId = "platform"},
                {.id = "ledge-12", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {480, 290}, .size = {65, 20}, .assetId = "platform"},
                // Section 5 - final climb.
                {.id = "pillar-5", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {300, 80}, .size = {90, 220}, .assetId = "platform"},
                {.id = "ledge-13", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {160, 180}, .size = {65, 20}, .assetId = "platform"},
                {.id = "ledge-14", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {430, 140}, .size = {70, 20}, .assetId = "platform"},
                {.id = "ledge-15", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {480, 40}, .size = {65, 20}, .assetId = "platform"},
                // Extra small platforms.
                {.id = "ledge-16", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {850, 1180}, .size = {55, 20}, .assetId = "platform"},
                {.id = "ledge-17", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {100, 1020}, .size = {55, 20}, .assetId = "platform"},
                {.id = "ledge-18", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {850, 760}, .size = {55, 20}, .assetId = "platform"},
                {.id = "ledge-19", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {100, 430}, .size = {55, 20}, .assetId = "platform"},
                {.id = "ledge-20", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {850, 200}, .size = {55, 20}, .assetId = "platform"},
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
