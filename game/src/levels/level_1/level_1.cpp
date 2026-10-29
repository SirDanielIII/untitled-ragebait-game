#include "levels/level.h"

namespace rage {
    // Edit this file for this stage: layout, narration, events and custom behavior.
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
            .platforms = {
                {0, 650, 2600, 50}, // Floor
                {400, 545, 250, 24},
                {820, 450, 220, 24},
                {1300, 545, 260, 24},
                {1840, 535, 240, 24},
            },
        };
        return definition;
    }

    namespace {
        class Level1 final : public Level {
        public:
            Level1() : Level(Level1Definition()) {
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

    std::unique_ptr<Level> MakeLevel1() { return std::make_unique<Level1>(); }
}
