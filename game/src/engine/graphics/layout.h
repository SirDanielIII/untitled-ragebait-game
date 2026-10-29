#pragma once

#include "raylib.h"
#include <algorithm>

namespace rage {
    inline constexpr int canvasWidth = 1600;
    inline constexpr int canvasHeight = 900;
    inline constexpr Rectangle worldViewport = {0, 140, 1600, 520};

    inline Vector2 WindowPixelScale() {
        // GetScreenWidth/Height are truncated at fractional DPI scales. Their ratio
        // to the framebuffer is not the matrix Raylib applies during BeginDrawing.
        if (IsWindowState(FLAG_WINDOW_HIGHDPI) && !IsWindowFullscreen()) {
            const auto dpi = GetWindowScaleDPI();
            return {std::max(1.0f, dpi.x), std::max(1.0f, dpi.y)};
        }
        return {1, 1};
    }

    inline Rectangle CanvasDestination(float width, float height) {
        const float scale = std::max(0.0f, std::min(width / canvasWidth, height / canvasHeight));
        return {
            (width - canvasWidth * scale) / 2, (height - canvasHeight * scale) / 2,
            canvasWidth * scale, canvasHeight * scale
        };
    }

    inline Vector2 CanvasCoordinates(Vector2 point, Rectangle destination) {
        if (destination.width <= 0 || destination.height <= 0) return {-1, -1};
        return {
            (point.x - destination.x) * canvasWidth / destination.width,
            (point.y - destination.y) * canvasHeight / destination.height
        };
    }
}
