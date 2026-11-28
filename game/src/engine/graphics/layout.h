#pragma once

#include "raylib.h"
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

    // How the finished canvas is copied to the window, so pixel art stays crisp at any size.
    //   Exact:        the window is a whole multiple of the canvas (1x, 2x...): nearest-neighbour.
    //   SharpUpscale: enlarge with nearest-neighbour to the next whole multiple (`factor`), then
    //                 shrink that to the window smoothly. Edges stay crisp; at most a one-pixel
    //                 soft seam where an art pixel straddles two screen pixels.
    //   Smooth:       shrinking below 1x, or a size too large to enlarge first: plain bilinear.
    enum class PresentMode { Exact, SharpUpscale, Smooth };

    struct Presentation {
        PresentMode mode = PresentMode::Smooth;
        int factor = 1;
    };

    // Largest intermediate width to allocate. Most GPUs support 8192-pixel textures.
    inline constexpr int maxPresentWidth = 8192;

    // `scale` = real window pixels per canvas pixel (e.g. 1.6 when maximized on a 1440p screen).
    inline Presentation ChoosePresentation(float scale) {
        if (!std::isfinite(scale) || scale <= 0) return {};
        const float nearest = std::round(scale);
        if (nearest >= 1 && std::abs(scale - nearest) < 0.002f)
            return {PresentMode::Exact, static_cast<int>(nearest)};
        if (scale < 1) return {};
        const int factor = static_cast<int>(std::ceil(scale));
        if (canvasWidth * factor > maxPresentWidth) return {};
        return {PresentMode::SharpUpscale, factor};
    }

    inline Vector2 CanvasCoordinates(Vector2 point, Rectangle destination) {
        if (destination.width <= 0 || destination.height <= 0) return {-1, -1};
        return {
            (point.x - destination.x) * canvasWidth / destination.width,
            (point.y - destination.y) * canvasHeight / destination.height
        };
    }
}
