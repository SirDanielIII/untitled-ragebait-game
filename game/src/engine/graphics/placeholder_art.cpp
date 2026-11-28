#include "engine/graphics/placeholder_art.h"
#include "ui/theme.h"

#include <algorithm>
#include <cmath>

namespace rage::art {
    void Bird(Rectangle b, bool facingRight, float seconds) {
        const float direction = facingRight ? 1.0f : -1.0f;
        const float centerX = b.x + b.width / 2;
        const float centerY = b.y + b.height / 2;
        DrawEllipse(static_cast<int>(centerX), static_cast<int>(centerY), b.width * 0.42f, b.height * 0.4f, DARKGRAY);
        DrawEllipse(static_cast<int>(centerX - direction * b.width * 0.12f),
                    static_cast<int>(centerY + std::sin(seconds * 22) * b.height * 0.15f),
                    b.width * 0.26f, b.height * 0.18f, GRAY);
        DrawCircleV({centerX + direction * b.width * 0.3f, centerY - b.height * 0.2f}, b.height * 0.25f, BLACK);
        DrawCircleV({centerX + direction * b.width * 0.34f, centerY - b.height * 0.25f}, b.height * 0.07f, RAYWHITE);
        DrawLineEx({centerX + direction * b.width * 0.4f, centerY - b.height * 0.12f},
                   {centerX + direction * b.width * 0.6f, centerY - b.height * 0.05f}, b.height * 0.14f, ORANGE);
    }

    void Student(const Resources &resources, Rectangle b) {
        if (resources.DrawArt(ArtId::Student, b)) return;
        const auto rect = [&](float x, float y, float w, float h, Color c) {
            DrawRectangleRec({b.x + x * b.width, b.y + y * b.height, w * b.width, h * b.height}, c);
        };
        rect(0.2f, 0, 0.64f, 0.38f, {203, 165, 143, 255});
        rect(0.15f, 0, 0.74f, 0.1f, theme::background);
        rect(0.26f, 0.19f, 0.17f, 0.04f, theme::background);
        rect(0.6f, 0.19f, 0.17f, 0.04f, theme::background);
        rect(0.26f, 0.25f, 0.17f, 0.04f, {126, 101, 111, 255});
        rect(0.6f, 0.25f, 0.17f, 0.04f, {126, 101, 111, 255});
        rect(0.16f, 0.39f, 0.68f, 0.4f, {113, 126, 164, 255});
        rect(0, 0.43f, 0.18f, 0.3f, {161, 110, 102, 255});
        rect(0.21f, 0.79f, 0.2f, 0.21f, theme::paper);
        rect(0.61f, 0.79f, 0.2f, 0.21f, theme::paper);
    }

    void Entity(const Resources &resources, Rectangle b) {
        if (resources.DrawArt(ArtId::Entity, b)) return;
        const float cx = b.x + b.width / 2;
        DrawEllipse(static_cast<int>(cx), static_cast<int>(b.y + b.height * 0.45f), b.width * 0.42f, b.height * 0.42f, {11, 10, 20, 255});
        DrawTriangle({cx - b.width * 0.3f, b.y + b.height * 0.3f}, {cx - b.width * 0.13f, b.y + b.height * 0.3f}, {cx - b.width * 0.38f, b.y}, theme::background);
        DrawTriangle({cx + b.width * 0.13f, b.y + b.height * 0.3f}, {cx + b.width * 0.3f, b.y + b.height * 0.3f}, {cx + b.width * 0.38f, b.y}, theme::background);
        DrawRectangleRec({cx - b.width * 0.26f, b.y + b.height * 0.4f, b.width * 0.15f, b.height * 0.035f}, theme::danger);
        DrawRectangleRec({cx + b.width * 0.11f, b.y + b.height * 0.4f, b.width * 0.15f, b.height * 0.035f}, theme::danger);
        DrawLineEx({cx - b.width * 0.17f, b.y + b.height * 0.6f}, {cx + b.width * 0.17f, b.y + b.height * 0.63f}, 3, theme::mint);
    }

    void Platform(const Resources &resources, Rectangle bounds, Color accent) {
        if (resources.DrawArt(ArtId::Platform, bounds, ArtFit::Stretch)) return;
        DrawRectangleRec(bounds, {60, 59, 76, 255});
        DrawRectangleRec({bounds.x, bounds.y, bounds.width, 4}, accent);
    }

    void LevelBackdrop(const Resources &resources, ArtId backdrop, Rectangle world, Color accent) {
        if (resources.DrawArt(backdrop, world, ArtFit::Stretch)) return;
        DrawRectangleRec(world, {28, 28, 42, 255});
        // These are scenery only. Authored platform rectangles remain the collision source.
        if (backdrop == ArtId::Level0) {
            for (int x = 80; x < world.width; x += 280) {
                DrawRectangle(x, 120, 145, 180, {44, 43, 65, 255});
                DrawRectangleLinesEx({static_cast<float>(x), 120, 145, 180}, 1.0f, theme::border);
                DrawLineEx({static_cast<float>(x + 72), 120}, {static_cast<float>(x + 72), 300}, 1.0f, theme::border);
            }
            DrawRectangle(1260, 410, 110, 240, theme::background);
            DrawRectangleLinesEx({1260, 410, 110, 240}, 1.0f, accent);
        } else if (backdrop == ArtId::Level1) {
            for (int x = 0; x < world.width; x += 220) {
                const int height = 160 + (x % 3) * 70;
                DrawRectangle(x, 650 - height, 170, height, {38, 39, 57, 255});
                DrawRectangle(x + 30, 670 - height, 30, 40, {92, 78, 91, 255});
            }
            DrawCircle(1400, 140, 65, {66, 56, 86, 255});
        } else if (backdrop == ArtId::Level2) {
            for (int y = 70; y < world.height; y += 180) {
                DrawRectangle(80, y, 840, 5, theme::border);
                DrawRectangleLinesEx({410, static_cast<float>(y + 35), 180, 90}, 1.0f, theme::border);
            }
        } else {
            DrawRectangle(1200, 300, 280, 350, {52, 49, 65, 255});
            DrawRectangleLinesEx({1200, 300, 280, 350}, 1.0f, accent);
            DrawRectangle(1300, 530, 70, 120, theme::background);
            Entity(resources, {1020, 420, 140, 180});
        }
    }

    bool DrawMenuArt(const Resources &resources, std::string_view assetId, Rectangle bounds) {
        const auto &assets = resources.Assets();
        const auto *image = assets.Find(assetId, "idle");
        if (!image || !assets.HasTexture(*image)) return false;
        // UI art may be much larger than its logical slot. Unlike world-sprite
        // FitScale, this must allow fractional downscaling to keep it inside.
        const float scale = std::min(bounds.width / image->frameWidth, bounds.height / image->frameHeight);
        const float width = image->frameWidth * scale;
        const float height = image->frameHeight * scale;
        float pivotX = 0, pivotY = 0;
        PivotOffset(image->pivot, width, height, pivotX, pivotY);
        const Vector2 position{bounds.x + (bounds.width - width) / 2 + pivotX,
                               bounds.y + (bounds.height - height) / 2 + pivotY};
        return assets.Draw(*image, 0, position, Facing::None, {}, scale) == DrawResult::Texture;
    }

    void MenuBackdrop(const Resources &resources, std::string_view assetId) {
        // ClearBackground would also erase the native framebuffer letterbox.
        // Opaque solid color is the default for every menu. Optional artwork
        // sits behind the UI; transparent areas retain the same solid color.
        DrawRectangle(0, 0, canvasWidth, canvasHeight, theme::background);
        DrawMenuArt(resources, assetId, {0, 0, canvasWidth, canvasHeight});
    }
}
