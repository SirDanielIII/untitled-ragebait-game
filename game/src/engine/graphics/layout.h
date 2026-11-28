#pragma once

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <algorithm>
#include <cmath>

namespace rage {
    inline constexpr int canvasWidth = 1600;
    inline constexpr int canvasHeight = 900;
    inline constexpr Rectangle worldViewport = {0, 0, 1600, 660};

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

    // Logical coordinates stay unchanged. Apply the viewport AFTER the camera and
    // local sprite transforms, directly in the framebuffer's physical pixels.
    inline Matrix ViewportMatrix(Rectangle physical) {
        return MatrixMultiply(MatrixScale(physical.width / canvasWidth, physical.height / canvasHeight, 1),
                              MatrixTranslate(physical.x, physical.y, 0));
    }

    // Keep the viewport in modelview, not rlPushMatrix's per-vertex transform.
    // Raylib's BeginMode2D resets modelview and would discard the viewport.
    // Flush on entry/exit so queued geometry uses the correct camera/viewport.
    class ScopedModelview {
    public:
        explicit ScopedModelview(Matrix matrix) : previous(rlGetMatrixModelview()) {
            rlDrawRenderBatchActive();
            rlSetMatrixModelview(matrix);
        }
        ~ScopedModelview() {
            rlDrawRenderBatchActive();
            rlSetMatrixModelview(previous);
        }
        ScopedModelview(const ScopedModelview &) = delete;
        ScopedModelview &operator=(const ScopedModelview &) = delete;
    private:
        Matrix previous;
    };

    inline Matrix CameraModelview(Camera2D camera, Matrix viewport) {
        return MatrixMultiply(GetCameraMatrix2D(camera), viewport);
    }

    inline Vector2 CanvasCoordinates(Vector2 point, Rectangle destination) {
        if (destination.width <= 0 || destination.height <= 0) return {-1, -1};
        return {
            (point.x - destination.x) * canvasWidth / destination.width,
            (point.y - destination.y) * canvasHeight / destination.height
        };
    }
}
