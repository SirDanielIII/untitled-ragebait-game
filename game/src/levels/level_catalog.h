#pragma once

#include "raylib.h"
#include <vector>

namespace rage
{
enum class LevelId { Tutorial, SideToSide, Climbing, Final };

struct LevelDefinition
{
    LevelId id;
    const char* title;
    const char* description;
    const char* plannedContent;
    const char* dialogue;
    Color accent;
    Rectangle world;
    Vector2 spawn;
    std::vector<Rectangle> platforms;
};

const std::vector<LevelDefinition>& LevelCatalog();
const LevelDefinition& FindLevel(LevelId id);
}
