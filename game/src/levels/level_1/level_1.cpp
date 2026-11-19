#include "levels/level_1/level_1.h"

#include <utility>

namespace rage {
    const LevelDefinition &Level1Definition() {
        static const LevelDefinition definition{
            .id = LevelId::Level1,
            .title = "Level 1",
            .description = "Cross the streets and reach campus.",
            .plannedContent = "Street hallucinations: traffic, cloud jumps, birds, live wires and falling satellites.",
            .intro = {
                .text = "Campus is just across the road. Look both ways. Then up.\nIf the pigs are flying, that is probably a normal weather pattern.",
                .speaker = "The Entity",
                .speedMultiplier = 1.0f,
                .startDelay = 0.0f,
                .fontSize = 24.0f,
                .allowSkip = true,
                .allowReplay = true
            },
            .accent = {119, 167, 209, 255},
            .backdrop = ArtId::Level1,
            .world = {0, 0, 2600, 700},
            .spawn = {100, 606},
            .objects = {
                {.id = "floor", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {0, 650}, .size = {2600, 50}, .assetId = "platform"},
                {.id = "platform-1", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {400, 545}, .size = {250, 24}, .assetId = "platform"},
                {.id = "platform-2", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {820, 450}, .size = {220, 24}, .assetId = "platform"},
                {.id = "platform-3", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {1300, 545}, .size = {260, 24}, .assetId = "platform"},
                {.id = "platform-4", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {1840, 535}, .size = {240, 24}, .assetId = "platform"},
            },
        };
        return definition;
    }

    Level1::Level1() : Level(Level1Definition()) {
        for (auto &event : CreateEvents()) {
            Events().RegisterEvent(std::move(event));
        }
    }

    std::unique_ptr<Level> MakeLevel1() { return std::make_unique<Level1>(); }
}
