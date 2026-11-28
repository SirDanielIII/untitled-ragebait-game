#include "ui/hud/gameplay_hud.h"

#include "engine/resources/resources.h"
#include "player/player.h"
#include "ui/text/text.h"
#include "ui/theme.h"

#include <algorithm>
#include <cmath>

namespace rage {
    void DrawHudPanel(Rectangle bounds, Color border, Color fill)
    {
        const int x = static_cast<int>(std::round(bounds.x));
        const int y = static_cast<int>(std::round(bounds.y));
        const int w = static_cast<int>(std::round(bounds.width));
        const int h = static_cast<int>(std::round(bounds.height));
        DrawRectangle(x + 8, y + 8, w - 8, h, theme::background);
        DrawRectangle(x + 4, y + h, w - 8, 4, theme::background);
        DrawRectangle(x + 4, y, w - 8, h, border);
        DrawRectangle(x, y + 4, w, h - 8, border);
        DrawRectangle(x + 4, y + 4, w - 8, h - 8, fill);
    }

    void DrawPlayerCard(const Resources &resources, const Player &player, const GameConfig &config)
    {
        DrawHudPanel({24, 24, 324, 140}, theme::border, theme::paper);
        DrawRectangle(28, 28, 316, 28, theme::mint);
        resources.DrawText("STUDENT ID", {40, 32}, 20, 1, theme::background, FontRole::Heading);
        // A tiny punched slot and a photo make this read as a campus ID.
        DrawRectangle(304, 36, 24, 8, theme::border);
        DrawRectangle(40, 68, 60, 80, theme::border);
        DrawRectangle(44, 72, 52, 72, theme::panel);
        const auto &assets = resources.Assets();
        const auto *portrait = assets.Find("player_character", "idle");
        if (!portrait) portrait = assets.Find("player_character", "");
        if (portrait && assets.HasTexture(*portrait)) {
            assets.DrawFitted(*portrait, 0, {44, 76, 52, 64});
        } else {
            // A 4-pixel-grid student portrait until character art is supplied.
            DrawRectangle(56, 84, 28, 32, theme::gold);
            DrawRectangle(52, 80, 32, 12, theme::background);
            DrawRectangle(56, 96, 8, 4, theme::background);
            DrawRectangle(72, 96, 8, 4, theme::background);
            DrawRectangle(56, 100, 8, 4, theme::danger);
            DrawRectangle(72, 100, 8, 4, theme::danger);
            DrawRectangle(52, 116, 36, 24, theme::mint);
            DrawRectangle(64, 116, 8, 12, theme::paper);
        }

        // This is the existing health count; respawn rules stay with gameplay.
        const Color heart = player.IsDead() ? theme::border : theme::danger;
        DrawRectangle(116, 72, 8, 4, heart);
        DrawRectangle(132, 72, 8, 4, heart);
        DrawRectangle(112, 76, 32, 8, heart);
        DrawRectangle(116, 84, 24, 4, heart);
        DrawRectangle(120, 88, 16, 4, heart);
        DrawRectangle(124, 92, 8, 4, heart);
        DrawTextFit(resources, TextFormat("LIVES %d", player.GetHealth()), {156, 72, 176, 24}, 24, theme::background);
        resources.DrawText("STAMINA", {116, 108}, 16, 1, theme::border);

        const float stamina = std::clamp(player.StaminaPercent(), 0.0f, 1.0f);
        const Color fill = player.LowStamina(config) ? theme::danger
            : stamina < 0.6f ? theme::gold : Color{116, 174, 151, 255};
        DrawRectangle(112, 128, 220, 20, theme::border);
        // Partial cells track the real value; each filled step is two canvas pixels.
        for (int i = 0; i < 10; ++i) {
            const int x = 116 + i * 21;
            DrawRectangle(x, 132, 18, 12, theme::panel);
            const float fraction = std::clamp(stamina * 10 - i, 0.0f, 1.0f);
            const int width = static_cast<int>(std::ceil(fraction * 9)) * 2;
            if (width > 0) DrawRectangle(x, 132, width, 12, fill);
        }
    }
}
