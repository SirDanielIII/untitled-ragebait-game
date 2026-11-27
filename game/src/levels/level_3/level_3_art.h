#pragma once

#include "raylib.h"
#include <string>


namespace rage::level3art {

    void Hallway(float worldWidth, float hallX, float time);

    void LectureHall(float hallX, float time);


    bool Object(const std::string &assetId, const std::string &id, Rectangle bounds, float time);
}
