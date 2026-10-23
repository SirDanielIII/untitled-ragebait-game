#include "levels/level.h"

namespace rage
{
// Edit this file for this stage: layout, narration, events and custom behavior.
const LevelDefinition& Level0Definition()
{
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
        .platforms = {
            {0, 650, 1500, 50}, // Floor
            {350, 550, 180, 24},
            {630, 470, 180, 24},
            {920, 550, 180, 24},
        },
    };
    return definition;
}

namespace
{
class Level0 final : public Level
{
public:
    Level0() : Level(Level0Definition()) {}
private:
    void OnReset() override
    {
        // TODO: Reset this level's event flags, timers and hazards here.
    }
    void OnUpdate(float dt, const InputFrame& input) override
    {
        // TODO: Implement your event system here. No scheduling rules are imposed.
        // Available: GetPlayer(), Dialogue(), ElapsedSeconds(), Say(...).
        (void)dt;
        (void)input;
    }

};
}
std::unique_ptr<Level> MakeLevel0() { return std::make_unique<Level0>(); }
}
