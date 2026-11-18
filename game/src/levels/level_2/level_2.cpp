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
            .platforms = {
                {0, 1450, 1000, 50}, // Floor
                {300, 1345, 200, 24},
                {520, 1240, 200, 24},
                {300, 1135, 200, 24},
                {520, 1030, 200, 24},
                {300, 925, 200, 24},
                {520, 820, 200, 24},
                {300, 715, 200, 24},
                {520, 610, 200, 24},
                {300, 505, 200, 24},
                {520, 400, 200, 24},
                {300, 295, 200, 24},
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
