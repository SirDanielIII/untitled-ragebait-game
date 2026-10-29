#include "levels/level.h"

namespace rage {
    // Edit this file for this stage: layout, narration, events and custom behavior.
    const LevelDefinition &Level3Definition() {
        static const LevelDefinition definition{
            .id = LevelId::Level3,
            .title = "Level 3",
            .description = "Face the Entity outside the lecture hall.",
            .plannedContent = "Final encounter ideas: shifting floors, floating furniture, paper airplanes and wall climbs.",
            .intro = {
                .text = "You made it. The lecture hall is right there.\nBefore you go in, there is one small thing we need to discuss.",
                .speaker = "The Entity",
                .speedMultiplier = 1.0f,
                .startDelay = 0.0f,
                .fontSize = 24.0f,
                .allowSkip = true,
                .allowReplay = true
            },
            .accent = {211, 133, 136, 255},
            .backdrop = ArtId::Level3,
            .world = {0, 0, 1600, 700},
            .spawn = {100, 606},
            .platforms = {
                {0, 650, 1600, 50}, // Floor
                {420, 550, 200, 24},
                {750, 470, 180, 24},
            },
        };
        return definition;
    }

    namespace {
        class Level3 final : public Level {
        public:
            Level3() : Level(Level3Definition()) {
            }

        private:
            void OnReset() override {
                // TODO: Reset this level's event flags, timers and hazards here.
            }

            void OnUpdate(float dt, const InputFrame &input) override {
                // TODO: Implement your event system here. No scheduling rules are imposed.
                // Available: GetPlayer(), Dialogue(), ElapsedSeconds(), Say(...).
                (void) dt;
                (void) input;
            }
        };
    }

    std::unique_ptr<Level> MakeLevel3() { return std::make_unique<Level3>(); }
}
