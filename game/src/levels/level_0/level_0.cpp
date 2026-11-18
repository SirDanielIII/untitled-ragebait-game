#include "levels/level_0/level_0.h"

#include <utility>

namespace rage {
    const LevelDefinition &Level0Definition() {
        static const LevelDefinition definition{
            .id = LevelId::Level0,
            .title = "Level 0",
            .description = "Leave the house through the front door.",
            .plannedContent = "House hallucinations: falling lights, flooded rooms, false doors and a fireplace monster.",
            .intro = {
                .text = "Good morning. You barely slept, and class has already started.\nI would check the front door. Ignore whatever the furniture is doing.",
                .speaker = "The Entity",
                .speedMultiplier = 1.0f,
                .startDelay = 0.0f,
                .fontSize = 24.0f,
                .allowSkip = true,
                .allowReplay = true
            },
            .accent = {108, 181, 170, 255},
            .backdrop = ArtId::Level0,
            .world = {0, 0, 1500, 700},
            .spawn = {100, 606},
            .objects = {
                {.id = "floor", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {0, 650}, .size = {1500, 50}, .assetId = "platform"},
                {.id = "step-1", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {350, 550}, .size = {180, 24}, .assetId = "platform"},
                {.id = "step-2", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {630, 470}, .size = {180, 24}, .assetId = "platform"},
                {.id = "step-3", .type = ObjectType::Static, .collision = CollisionType::Solid, .position = {920, 550}, .size = {180, 24}, .assetId = "platform"},
            },
        };
        return definition;
    }

    Level0::Level0() : Level(Level0Definition()) {
        for (auto &event : CreateEvents()) {
            Events().RegisterEvent(std::move(event));
        }
    }

    void Level0::OnReset() {
        idleCue = 0;
        stillSeconds = 0;
        previousPosition = GetPlayer().Position();
        movedThisFrame = false;
        hallwayCue = 0;
        followUpTime = 0.0f;
    }

    void Level0::OnUpdate(float dt, const InputFrame &) {
        const auto position = GetPlayer().Position();
        movedThisFrame = position.x != previousPosition.x || position.y != previousPosition.y;
        stillSeconds = movedThisFrame ? 0 : stillSeconds + dt;
        previousPosition = position;
    }

    std::unique_ptr<Level> MakeLevel0() { return std::make_unique<Level0>(); }
}
