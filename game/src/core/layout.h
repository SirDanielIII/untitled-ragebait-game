#pragma once

#include "raylib.h"
#include <algorithm>

namespace rage
{
inline constexpr int canvasWidth = 1600;
inline constexpr int canvasHeight = 900;
inline constexpr Rectangle worldViewport = {0, 140, 1600, 520};

inline Rectangle CanvasDestination(float width, float height)
{
    const float scale = std::max(0.0f, std::min(width / canvasWidth, height / canvasHeight));
    return {(width - canvasWidth * scale) / 2, (height - canvasHeight * scale) / 2,
            canvasWidth * scale, canvasHeight * scale};
}

inline Vector2 CanvasCoordinates(Vector2 point, Rectangle destination)
{
    if (destination.width <= 0 || destination.height <= 0) return {-1, -1};
    return {(point.x - destination.x) * canvasWidth / destination.width,
            (point.y - destination.y) * canvasHeight / destination.height};
}
}
