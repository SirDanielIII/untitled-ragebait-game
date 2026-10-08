#include "level_catalog.h"

#include <algorithm>
#include <stdexcept>

namespace rage
{
const std::vector<LevelDefinition>& LevelCatalog()
{
    static const std::vector<LevelDefinition> levels = {
        {LevelId::Tutorial, "Tutorial - Hard", "Meet the Entity and test the platformer controls.",
         "Planned: unreliable instructions, difficult jumps, falling platforms.",
         "I'm the Entity, your very dependable personal assistant.\nTry moving and jumping. These platforms are safe... for this demo.",
         {108, 181, 170, 255}, {0, 0, 1500, 700}, {100, 606},
         {{0, 650, 1500, 50}, {350, 550, 180, 24}, {630, 470, 180, 24}, {920, 550, 180, 24}}},
        {LevelId::SideToSide, "Level 1 - Side-to-Side", "A horizontal route toward Queen's University.",
         "Planned: road obstacles, narrator tricks, an absurd interval, fake timer.",
         "You're late for class. I recommend taking this completely ordinary route.\nRoad obstacles and the fake countdown will be added by the team later.",
         {119, 167, 209, 255}, {0, 0, 2600, 700}, {100, 606},
         {{0, 650, 2600, 50}, {400, 545, 250, 24}, {820, 450, 220, 24}, {1300, 545, 260, 24}, {1840, 535, 240, 24}}},
        {LevelId::Climbing, "Level 2 - Climbing", "A vertical scaffold with upward camera movement.",
         "Planned: arm/limb controls, slippery surfaces, falling rocks, moving parts.",
         "Up is usually the direction you want. A fascinating fact, I know.\nUse the shared jump controller here; limb controls and climbing physics are still to come.",
         {177, 143, 210, 255}, {0, 0, 1000, 1500}, {180, 1406},
         {{0, 1450, 1000, 50}, {300, 1345, 200, 24}, {520, 1240, 200, 24}, {300, 1135, 200, 24},
          {520, 1030, 200, 24}, {300, 925, 200, 24}, {520, 820, 200, 24}, {300, 715, 200, 24},
          {520, 610, 200, 24}, {300, 505, 200, 24}, {520, 400, 200, 24}, {300, 295, 200, 24}}},
        {LevelId::Final, "Level 3 - Final", "The lecture hall and the Entity's future encounter.",
         "Planned: narrator reveal, previous traps, boss encounter, farewell/ending.",
         "The lecture hall is right there. Surely nothing dramatic will happen.\nMy final encounter and farewell are placeholders; there is no boss fight in this foundation.",
         {211, 133, 136, 255}, {0, 0, 1600, 700}, {100, 606},
         {{0, 650, 1600, 50}, {420, 550, 200, 24}, {750, 470, 180, 24}}}
    };
    return levels;
}

const LevelDefinition& FindLevel(LevelId id)
{
    const auto& levels = LevelCatalog();
    const auto found = std::find_if(levels.begin(), levels.end(), [id](const auto& level) { return level.id == id; });
    if (found == levels.end()) throw std::invalid_argument("Unknown level id");
    return *found;
}
}
