#include "levels/level.h"

namespace rage {
    // Edit this file for this stage: layout, narration, events and custom behavior.
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




            .platforms = {
                // FLOOR
        {0, 1450, 1000, 50},

        // SECTION 1 - FIRST PILLAR
        {280, 1120, 90, 330},
        {400, 1110, 70, 20},
        {150, 1280, 75, 20},
        {390, 1250, 65, 20},

        // SECTION 2 - SECOND PILLAR
        {620, 850, 90, 300},
        {450, 950, 70, 20},
        {760, 1020, 70, 20},
        {470, 820, 65, 20},

        // SECTION 3 - THIRD PILLAR
        {300, 570, 90, 280},
        {160, 700, 70, 20},
        {430, 650, 65, 20},
        {160, 540, 65, 20},

        // SECTION 4 - FOURTH PILLAR
        {650, 300, 90, 270},
        {480, 440, 70, 20},
        {790, 480, 65, 20},
        {480, 290, 65, 20},

        // SECTION 5 - FINAL CLIMB
        {300, 80, 90, 220},
        {160, 180, 65, 20},
        {430, 140, 70, 20},
        {480, 40, 65, 20},

        // EXTRA SMALL PLATFORMS
        {850, 1180, 55, 20},
        {100, 1020, 55, 20},
        {850, 760, 55, 20},
        {100, 430, 55, 20},
        {850, 200, 55, 20},
    },



        };
        return definition;
    }

    namespace {
        class Level2 final : public Level {
        public:
            Level2() : Level(Level2Definition()) {
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

    std::unique_ptr<Level> MakeLevel2() { return std::make_unique<Level2>(); }
}
