#include "ui/menus/menus.h"
#include "engine/graphics/placeholder_art.h"
#include "ui/theme.h"
#include "ui/text/text.h"

#include <initializer_list>
#include <string_view>

namespace rage {
    std::vector<MenuItem> MainMenuItems() {
        return {
            Button{"START GAME", {ActionKind::StartLevel, 0}, {84, 438, 540, 82}, {73, 57, 108, 255}, ButtonStyle::Filled, false, 30},
            Button{"SETTINGS", {ActionKind::OpenSettings}, {84, 542, 258, 66}, theme::panel, ButtonStyle::Filled, false, 24},
            Button{"LEVEL SELECT", {ActionKind::OpenLevels}, {366, 542, 258, 66}, theme::panel, ButtonStyle::Filled, false, 18},
            Button{"QUIT", {ActionKind::Quit}, {84, 630, 540, 60}, {57, 32, 47, 255}, ButtonStyle::Filled, false, 22}
        };
    }

    void DrawMainMenu(const Resources &resources) {
        using namespace theme;
        art::MenuBackdrop(resources, ArtId::MainMenu);
        DrawTextFit(resources, "UNTITLED", {80, 149, 555, 78}, 68, paper, FontRole::Heading);
        DrawTextFit(resources, "RAGEBAIT", {80, 227, 555, 78}, 68, mint, FontRole::Heading);
        DrawTextFit(resources, "GAME", {84, 307, 540, 57}, 50, paper, FontRole::Heading);
        DrawParagraph(resources, "A platformer about a student on the way to class.", {86, 378, 530, 52}, 22, muted);
    }

    std::vector<MenuItem> LevelSelectItems() {
        std::vector<MenuItem> items;
        for (const auto &level: LevelCatalog())
            items.emplace_back(Button{
                level.title, {ActionKind::StartLevel, static_cast<int>(level.id)},
                {84, 252.0f + items.size() * 100.0f, 630, 76}, theme::panel, ButtonStyle::Filled, false, 28
            });
        items.emplace_back(Button{"BACK TO MAIN MENU", {ActionKind::MainMenu}, {84, 684, 630, 64}, theme::panel, ButtonStyle::Filled, false, 24});
        return items;
    }

    void DrawLevelSelect(const Resources &resources, int selected) {
        using namespace theme;
        art::MenuBackdrop(resources, ArtId::StageSelect);
        DrawTextFit(resources, "LEVEL SELECT", {84, 70, 1380, 60}, 48, paper, FontRole::Heading);
        DrawTextFit(resources, "Open any level directly for development and testing.", {86, 150, 1400, 30}, 23, muted);
        DrawRectangleRec({790, 252, 730, 496}, panel);
        if (selected >= 0 && selected < static_cast<int>(LevelCatalog().size())) {
            const auto &level = LevelCatalog()[selected];
            DrawRectangleRec({790, 252, 730, 4}, level.accent);
            DrawTextFit(resources, "SELECTED LEVEL", {824, 280, 650, 24}, 21, gold, FontRole::Heading);
            DrawTextFit(resources, level.title, {824, 328, 650, 42}, 32, paper, FontRole::Heading);
            DrawParagraph(resources, level.description, {824, 400, 650, 96}, 25, muted);
            DrawTextFit(resources, "DEVELOPMENT NOTES", {824, 528, 650, 25}, 21, mint, FontRole::Heading);
            DrawParagraph(resources, level.plannedContent, {824, 576, 650, 138}, 22, paper);
        } else {
            art::Entity(resources, {1050, 310, 180, 230});
            DrawParagraph(resources, "Return to the main menu.", {824, 600, 650, 104}, 24, paper);
        }
        DrawTextFit(resources, "MOUSE / ARROWS: CHOOSE A STAGE    ENTER: START    ESC: BACK", {84, 845, 1430, 28}, 20, muted);
    }

    std::vector<MenuItem> PauseMenuItems() {
        return {
            Button{"RESUME", {ActionKind::Resume}, {84, 252, 630, 70}, {73, 57, 108, 255}},
            Button{"RESTART", {ActionKind::Restart}, {84, 346, 630, 70}, theme::panel},
            Button{"SETTINGS", {ActionKind::OpenSettings}, {84, 440, 630, 70}, theme::panel},
            Button{"LEVEL SELECT", {ActionKind::OpenLevels}, {84, 534, 630, 70}, theme::panel},
            Button{"MAIN MENU", {ActionKind::MainMenu}, {84, 628, 630, 70}, {57, 32, 47, 255}}
        };
    }

    void DrawPauseMenu(const Resources &resources, const LevelDefinition &level) {
        using namespace theme;
        art::MenuBackdrop(resources, ArtId::Pause);
        DrawTextFit(resources, "PAUSED", {84, 70, 1400, 60}, 48, paper, FontRole::Heading);
        DrawTextFit(resources, level.title, {86, 150, 1400, 30}, 24, muted, FontRole::Heading);
        art::Entity(resources, {1030, 245, 200, 245});
        DrawTextFit(resources, "GAME PAUSED", {840, 552, 650, 26}, 22, mint, FontRole::Heading);
        DrawParagraph(resources, "Resume playing, restart this level,\nor return to the main menu.", {840, 603, 640, 112}, 25, paper);
        DrawTextFit(resources, "PAUSED / ESC TO RETURN", {84, 845, 1430, 28}, 21, muted);
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
            float size = 20;
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
            DrawTextFit(resources, action, {controlsX, y + 8, x - controlsX - 10, 23}, 18, theme::muted);
            for (auto key : keys) {
                const bool separator = key == "+" || key == "/";
                const float width = separator ? 12 : KeycapWidth(key);
                if (separator) resources.DrawCenteredText(key, {x, y, width, 34}, 17, 1, theme::muted, FontRole::Typewriter);
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
            row("CROUCH", {"S", "DOWN"});
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
            Button{"BACK", {ActionKind::Back}, {70, 64, 150, 54}, panel, ButtonStyle::Filled, false, 24},
            SliderItem{
                "FRAME RATE CAP", ActionKind::SetTargetFps,
                Slider({displayX, 385, 390, 52}, minTargetFps, maxTargetFps, 1, static_cast<float>(config.targetFps)), " FPS", 1, {}
            },
            Button{"UNLIMITED FPS", {ActionKind::ToggleUnlimitedFps}, {displayX, 484, 390, 54}, mint, ButtonStyle::Checkbox, config.unlimitedFps, 25},
            Button{"SHOW FPS", {ActionKind::ToggleFps}, {displayX, 564, 390, 54}, mint, ButtonStyle::Checkbox, config.showFps, 25},
#if !defined(PLATFORM_WEB)
            Button{"FULLSCREEN", {ActionKind::ToggleFullscreen}, {displayX, 644, 390, 54}, mint, ButtonStyle::Checkbox, config.fullscreen, 25},
#endif
            // The audio checkbox doubles as the column heading.
            Button{"AUDIO ENABLED", {ActionKind::ToggleMute}, {audioX, 236, 390, 54}, mint, ButtonStyle::Checkbox, !config.muted, 25},
            SliderItem{
                "MASTER VOLUME", ActionKind::SetVolume,
                Slider({audioX, 388, 390, 52}, 0, 1, 0.01f, config.masterVolume), "%", 100, {}
            },
            SliderItem{
                "MUSIC", ActionKind::SetMusicVolume,
                Slider({audioX, 538, 390, 52}, 0, 1, 0.01f, config.musicVolume), "%", 100, {}
            },
            SliderItem{
                "SOUND EFFECTS", ActionKind::SetSfxVolume,
                Slider({audioX, 688, 390, 52}, 0, 1, 0.01f, config.sfxVolume), "%", 100, {}
            }
        };
    }

    void DrawSettingsMenu(const Resources &resources, const GameConfig &) {
        using namespace theme;
        art::MenuBackdrop(resources, ArtId::Settings);
        DrawTextFit(resources, "SETTINGS", {270, 65, 1200, 44}, 44, paper, FontRole::Heading);
        DrawTextFit(resources, "Display, audio and controls.", {272, 123, 1200, 23}, 23, muted);
        for (int i = 0; i < 3; ++i) {
            const float x = settingsPanelX + i * settingsPanelStep;
            DrawRectangleRec({x, settingsPanelY, settingsPanelWidth, settingsPanelHeight}, panel);
            DrawRectangleRec({x, settingsPanelY, settingsPanelWidth, 4}, i == 1 ? gold : mint);
            if (i == 0 || i == 2)
                DrawTextFit(resources, i == 0 ? "DISPLAY" : "CONTROLS", {x + 35, 246, 390, 24}, 24, muted, FontRole::Heading);
        }
        DrawControls(resources);
    }
}
