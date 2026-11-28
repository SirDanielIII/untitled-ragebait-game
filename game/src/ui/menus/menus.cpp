#include "ui/menus/menus.h"
#include "engine/graphics/placeholder_art.h"
#include "ui/theme.h"
#include "ui/text/text.h"

#include <initializer_list>
#include <string_view>

namespace rage {
    std::vector<MenuItem> MainMenuItems() {
        return {
            Button{"START GAME", {ActionKind::StartLevel, 0}, {84, 438, 540, 82}, {73, 57, 108, 255}, ButtonStyle::Filled, false, 30, theme::menuPlay},
            Button{"SETTINGS", {ActionKind::OpenSettings}, {84, 542, 258, 66}, theme::panel, ButtonStyle::Filled, false, 24, theme::menuDisplay},
            Button{"LEVEL SELECT", {ActionKind::OpenLevels}, {366, 542, 258, 66}, theme::panel, ButtonStyle::Filled, false, 18, theme::menuLevels},
            Button{"QUIT", {ActionKind::Quit}, {84, 630, 540, 60}, {57, 32, 47, 255}, ButtonStyle::Filled, false, 22, theme::danger}
        };
    }

    void DrawMainMenu(const Resources &resources) {
        using namespace theme;
        art::MenuBackdrop(resources, "menu_main");
        DrawTextFit(resources, "UNTITLED", {80, 149, 555, 78}, 68, paper, FontRole::Heading);
        DrawTextFit(resources, "RAGEBAIT", {80, 227, 555, 78}, 68, danger, FontRole::Heading);
        DrawTextFit(resources, "GAME", {84, 307, 540, 57}, 50, paper, FontRole::Heading);
        DrawParagraph(resources, "University simulator ahhh.", {86, 378, 530, 52}, 22, muted);
    }

    std::vector<MenuItem> LevelSelectItems() {
        std::vector<MenuItem> items;
        for (const auto &level: LevelCatalog())
            items.emplace_back(Button{
                level.title, {ActionKind::StartLevel, static_cast<int>(level.id)},
                {84, 220.0f + items.size() * 116.0f, 630, 84}, theme::panel, ButtonStyle::Filled, false, 28, level.accent
            });
        items.emplace_back(Button{"BACK TO MAIN MENU", {ActionKind::MainMenu}, {84, 708, 630, 72}, theme::panel, ButtonStyle::Filled, false, 24, theme::muted});
        return items;
    }

    namespace {
        // Player-facing previews, separate from the short in-game objective text.
        std::string_view LevelPreviewDescription(LevelId id) {
            switch (id) {
                case LevelId::Level0:
                    return "Late for class and barely awake, escape a house full of hallucinations while a mysterious Entity offers dubious advice.";
                case LevelId::Level1:
                    return "Your walk to school takes a surreal turn. Dodge traffic, cross electrified wires and leap through the clouds to reach campus.";
                case LevelId::Level2:
                    return "The stairs are gone, but your class is on the top floor. Scale the walls and find a way up before you lose focus.";
                case LevelId::Level3:
                    return "The lecture hall is within reach, but the Entity stands in your way. Survive its increasingly chaotic obstacles to get to class.";
            }
            return {};
        }
    }

    void DrawLevelSelect(const Resources &resources, int selected) {
        using namespace theme;
        art::MenuBackdrop(resources, "menu_level_select");
        DrawTextFit(resources, "LEVEL SELECT", {84, 70, 1380, 60}, 48, paper, FontRole::Heading);
        constexpr Rectangle detailPanel{790, 220, 730, 560};
        // Keep the 16:9 artwork and description in one fixed content column.
        constexpr float verticalPadding = 56;
        constexpr Rectangle preview{detailPanel.x + (detailPanel.width - 576) / 2,
                                    detailPanel.y + detailPanel.height - verticalPadding - 324, 576, 324};
        constexpr float descriptionTop = detailPanel.y + verticalPadding;
        constexpr float descriptionGap = 32;
        // Reserve a few pixels for rasterized glyph overhang at fractional scales.
        constexpr float textInset = 4;
        constexpr Rectangle description{preview.x + textInset, descriptionTop, preview.width - 2 * textInset,
                                        preview.y - descriptionGap - descriptionTop};
        constexpr float descriptionSize = 20;
        DrawRectangleRec(detailPanel, panel);
        if (selected >= 0 && selected < static_cast<int>(LevelCatalog().size())) {
            const auto &level = LevelCatalog()[selected];
            DrawRectangleRec({detailPanel.x, detailPanel.y, detailPanel.width, 4}, level.accent);
            DrawParagraph(resources, LevelPreviewDescription(level.id), description, descriptionSize, paper, FontRole::Typewriter);

            // Preview art is independent of world backdrops.
            DrawRectangleRec(preview, background);
            const auto previewId = [](LevelId id) -> std::string_view {
                switch (id) {
                    case LevelId::Level0: return "menu_preview_level_0";
                    case LevelId::Level1: return "menu_preview_level_1";
                    case LevelId::Level2: return "menu_preview_level_2";
                    case LevelId::Level3: return "menu_preview_level_3";
                }
                return {}; // A newly registered level can use the placeholder until art is assigned.
            };
            art::DrawMenuArt(resources, previewId(level.id), preview);
            DrawRectangleLinesEx(preview, 1, border);
        } else {
            DrawParagraph(resources, "Return to the main menu.", description, descriptionSize, paper, FontRole::Typewriter);
        }
    }

    std::vector<MenuItem> PauseMenuItems() {
        return {
            Button{"RESUME", {ActionKind::Resume}, {84, 252, 630, 70}, {73, 57, 108, 255}, ButtonStyle::Filled, false, 28, theme::menuPlay},
            Button{"RESTART", {ActionKind::Restart}, {84, 346, 630, 70}, theme::panel, ButtonStyle::Filled, false, 28, theme::menuAudio},
            Button{"SETTINGS", {ActionKind::OpenSettings}, {84, 440, 630, 70}, theme::panel, ButtonStyle::Filled, false, 28, theme::menuDisplay},
            Button{"LEVEL SELECT", {ActionKind::OpenLevels}, {84, 534, 630, 70}, theme::panel, ButtonStyle::Filled, false, 28, theme::menuLevels},
            Button{"MAIN MENU", {ActionKind::MainMenu}, {84, 628, 630, 70}, {57, 32, 47, 255}, ButtonStyle::Filled, false, 28, theme::danger}
        };
    }

    void DrawPauseMenu(const Resources &resources, const LevelDefinition &level) {
        using namespace theme;
        art::MenuBackdrop(resources, "menu_pause");
        DrawTextFit(resources, "PAUSED", {84, 70, 1400, 60}, 48, paper, FontRole::Heading);
        DrawTextFit(resources, level.title, {86, 150, 1400, 30}, 24, level.accent, FontRole::Heading);
        // Optional transparent illustration; leave the right side empty until exported.
        art::DrawMenuArt(resources, "menu_pause_sprite", {820, 225, 680, 510});
    }

    namespace {
        constexpr float settingsPanelX = 75;
        constexpr float settingsPanelY = 214;
        constexpr float settingsPanelWidth = 460;
        constexpr float settingsPanelHeight = 626;
        constexpr float settingsPanelStep = 495;
        constexpr float displayX = settingsPanelX + 35;
        constexpr float audioX = displayX + settingsPanelStep;
        constexpr float controlsX = displayX + 2 * settingsPanelStep;

        // Native vector keycaps; their Jersey10 glyphs use actual ink bounds
        // for centering, rather than the font's advance box or baseline.
        void DrawKeycap(const Resources &resources, std::string_view key, Rectangle bounds) {
            DrawRectangleRec(bounds, theme::background);
            DrawRectangleLinesEx(bounds, 1.5f, theme::border);
            constexpr auto role = FontRole::Typewriter;
            float size = 16;
            while (size > 10 && resources.MeasureText(key, size, 1, role).x > bounds.width - 8)
                size -= 1;
            resources.DrawCenteredText(key, bounds, size, 1, theme::paper, role);
        }

        constexpr float KeycapWidth(std::string_view key) {
            return key.size() <= 1 ? 30 : 22 + 9 * static_cast<float>(key.size());
        }

        void DrawControlRow(const Resources &resources, float y, std::string_view action,
                            std::initializer_list<std::string_view> keys) {
            // Right-align native keycaps. '+' and '/' are separators, not keys.
            float total = 0;
            for (auto key : keys) total += (key == "+" || key == "/" ? 12 : KeycapWidth(key)) + 4;
            float x = controlsX + 390 - total + 4;
            DrawTextFit(resources, action, {controlsX, y + 8, x - controlsX - 10, 23}, 16, theme::muted);
            for (auto key : keys) {
                const bool separator = key == "+" || key == "/";
                const float width = separator ? 12 : KeycapWidth(key);
                if (separator) resources.DrawCenteredText(key, {x, y, width, 34}, 15, 1, theme::muted, FontRole::Typewriter);
                else DrawKeycap(resources, key, {x, y, width, 34});
                x += width + 4;
            }
        }

        void DrawControls(const Resources &resources) {
            float y = 283;
            const auto row = [&](std::string_view action, std::initializer_list<std::string_view> keys) {
                DrawControlRow(resources, y, action, keys);
                y += 44;
            };
            row("MOVE", {"A", "D", "/", "LEFT", "RIGHT"});
            row("JUMP", {"SPACE", "W", "UP"});
            row("CROUCH", {"S", "DOWN", "SHIFT"});
            row("CLIMB", {"W", "UP"});
            row("PAUSE", {"ESC", "P"});
            row("SKIP DIALOGUE", {"ENTER"});
            row("REPLAY DIALOGUE", {"T"});
            row("RESET PLAYER", {"R"});
            row("MENU NAV", {"W", "S", "UP", "DOWN", "TAB"});
            row("MENU CONFIRM", {"ENTER"});
            row("MENU BACK", {"ESC", "BKSP"});
#if !defined(PLATFORM_WEB)
            row("FULLSCREEN", {"ALT", "+", "ENTER", "/", "F11"});
#endif
        }
    }

    std::vector<MenuItem> SettingsMenuItems(const GameConfig &config) {
        using namespace theme;
        return {
            Button{"BACK", {ActionKind::Back}, {70, 64, 150, 54}, panel, ButtonStyle::Filled, false, 24, muted},
            SliderItem{
                "FRAME RATE CAP", ActionKind::SetTargetFps,
                Slider({displayX, 385, 390, 52}, minTargetFps, maxTargetFps, 1, static_cast<float>(config.targetFps)), " FPS", 1, {}, menuDisplay
            },
            Button{"UNLIMITED FPS", {ActionKind::ToggleUnlimitedFps}, {displayX, 484, 390, 54}, menuDisplay, ButtonStyle::Checkbox, config.unlimitedFps, 25, menuDisplay},
            Button{"SHOW FPS", {ActionKind::ToggleFps}, {displayX, 564, 390, 54}, menuDisplay, ButtonStyle::Checkbox, config.showFps, 25, menuDisplay},
#if !defined(PLATFORM_WEB)
            Button{"FULLSCREEN", {ActionKind::ToggleFullscreen}, {displayX, 644, 390, 54}, menuDisplay, ButtonStyle::Checkbox, config.fullscreen, 25, menuDisplay},
#endif
            // The audio checkbox doubles as the column heading.
            Button{"AUDIO ENABLED", {ActionKind::ToggleMute}, {audioX, 236, 390, 54}, menuAudio, ButtonStyle::Checkbox, !config.muted, 25, menuAudio},
            SliderItem{
                "MASTER VOLUME", ActionKind::SetVolume,
                Slider({audioX, 388, 390, 52}, 0, 1, 0.01f, config.masterVolume), "%", 100, {}, menuAudio
            },
            SliderItem{
                "MUSIC", ActionKind::SetMusicVolume,
                Slider({audioX, 538, 390, 52}, 0, 1, 0.01f, config.musicVolume), "%", 100, {}, menuAudio
            },
            SliderItem{
                "SOUND EFFECTS", ActionKind::SetSfxVolume,
                Slider({audioX, 688, 390, 52}, 0, 1, 0.01f, config.sfxVolume), "%", 100, {}, menuAudio
            }
        };
    }

    void DrawSettingsMenu(const Resources &resources, const GameConfig &) {
        using namespace theme;
        art::MenuBackdrop(resources, "menu_settings");
        DrawTextFit(resources, "SETTINGS", {270, 65, 1200, 44}, 44, paper, FontRole::Heading);
        DrawTextFit(resources, "Display, audio and controls.", {272, 123, 1200, 23}, 23, muted);
        for (int i = 0; i < 3; ++i) {
            const float x = settingsPanelX + i * settingsPanelStep;
            DrawRectangleRec({x, settingsPanelY, settingsPanelWidth, settingsPanelHeight}, panel);
            DrawRectangleRec({x, settingsPanelY, settingsPanelWidth, 4}, i == 0 ? menuDisplay : i == 1 ? menuAudio : menuPlay);
            if (i == 0 || i == 2)
                DrawTextFit(resources, i == 0 ? "DISPLAY" : "CONTROLS", {x + 35, 246, 390, 24}, 24, muted, FontRole::Heading);
        }
        DrawControls(resources);
    }
}
