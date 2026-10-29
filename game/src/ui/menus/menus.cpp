#include "ui/menus/menus.h"
#include "engine/graphics/placeholder_art.h"
#include "ui/theme.h"
#include "ui/text/text.h"

namespace rage {
    std::vector<MenuItem> MainMenuItems() {
        return {
            Button{"START GAME", {ActionKind::StartLevel, 0}, {84, 438, 540, 82}, {73, 57, 108, 255}, ButtonStyle::Filled, false, 30},
            Button{"SETTINGS", {ActionKind::OpenSettings}, {84, 542, 258, 66}, theme::panel, ButtonStyle::Filled, false, 24},
            Button{"DEV LEVEL SELECT", {ActionKind::OpenLevels}, {366, 542, 258, 66}, theme::panel, ButtonStyle::Filled, false, 18},
            Button{"QUIT", {ActionKind::Quit}, {84, 630, 540, 60}, {57, 32, 47, 255}, ButtonStyle::Filled, false, 22}
        };
    }

    void DrawMainMenu(const Resources &resources) {
        using namespace theme;
        art::MenuBackdrop(resources, ArtId::MainMenu);
        DrawRectangle(84, 77, 8, 26, danger);
        DrawTextFit(resources, "MAIN MENU", {110, 80, 535, 26}, 23, gold);
        DrawTextFit(resources, "UNTITLED", {80, 149, 555, 78}, 68, paper);
        DrawTextFit(resources, "RAGEBAIT", {80, 227, 555, 78}, 68, mint);
        DrawTextFit(resources, "GAME", {84, 307, 540, 57}, 50, paper);
        DrawParagraph(resources, "A platformer about a student on the way to class.", {86, 378, 530, 52}, 22, muted);
        DrawTextFit(resources, "FOUR LEVELS", {730, 74, 780, 25}, 20, muted);
        DrawTextFit(resources, "GET FROM HOME TO THE LECTURE HALL.", {745, 650, 745, 24}, 18, gold);

        DrawRectangleRec({710, 704, 810, 128}, panel);
        DrawRectangleRec({710, 704, 4, 128}, danger);
        DrawTextFit(resources, "ABOUT THE GAME", {736, 722, 740, 24}, 21, mint);
        DrawParagraph(resources, "Navigate obstacles with help from the Entity,\nan unreliable narrator.", {736, 758, 750, 64}, 23, paper);
        DrawTextFit(resources, "A/D: MOVE   SPACE: JUMP   ESC: PAUSE", {86, 750, 535, 24}, 19, muted);
        DrawTextFit(resources, "ARROWS / MOUSE TO SELECT. ENTER TO CHOOSE.", {86, 791, 535, 23}, 17, muted);
        DrawTextFit(resources, "CISC 320 / GROUP 7", {86, 856, 535, 20}, 17, muted);
        DrawTextFit(resources, "HOUSE  >  STREETS  >  STAIRWELL  >  CLASS", {710, 856, 810, 23}, 18, muted);
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
        DrawTextFit(resources, "DEV LEVEL SELECT", {84, 70, 1380, 60}, 48, paper);
        DrawTextFit(resources, "Open any level directly for development and testing.", {86, 150, 1400, 30}, 23, muted);
        DrawRectangleRec({790, 252, 730, 496}, panel);
        if (selected >= 0 && selected < static_cast<int>(LevelCatalog().size())) {
            const auto &level = LevelCatalog()[selected];
            DrawRectangleRec({790, 252, 730, 4}, level.accent);
            DrawTextFit(resources, "SELECTED LEVEL", {824, 280, 650, 24}, 21, gold);
            DrawTextFit(resources, level.title, {824, 328, 650, 42}, 32, paper);
            DrawParagraph(resources, level.description, {824, 400, 650, 96}, 25, muted);
            DrawTextFit(resources, "DEVELOPMENT NOTES", {824, 528, 650, 25}, 21, mint);
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
            Button{"RESTART STAGE", {ActionKind::Restart}, {84, 346, 630, 70}, theme::panel},
            Button{"SETTINGS", {ActionKind::OpenSettings}, {84, 440, 630, 70}, theme::panel},
            Button{"DEV LEVEL SELECT", {ActionKind::OpenLevels}, {84, 534, 630, 70}, theme::panel},
            Button{"MAIN MENU", {ActionKind::MainMenu}, {84, 628, 630, 70}, {57, 32, 47, 255}}
        };
    }

    void DrawPauseMenu(const Resources &resources, const LevelDefinition &level) {
        using namespace theme;
        if (!resources.DrawArt(ArtId::Pause, {0, 0, 1600, 900}, ArtFit::Stretch))
            DrawRectangle(0, 0, 1600, 900, {17, 17, 27, 242});
        DrawTextFit(resources, "PAUSED", {84, 70, 1400, 60}, 48, paper);
        DrawTextFit(resources, level.title, {86, 150, 1400, 30}, 24, muted);
        art::Entity(resources, {1030, 245, 200, 245});
        DrawTextFit(resources, "GAME PAUSED", {840, 552, 650, 26}, 22, mint);
        DrawParagraph(resources, "Resume playing, restart this level,\nor return to the main menu.", {840, 603, 640, 112}, 25, paper);
        DrawTextFit(resources, "PAUSED / ESC TO RETURN", {84, 845, 1430, 28}, 21, muted);
    }

    std::vector<MenuItem> SettingsMenuItems(const GameConfig &config) {
        using namespace theme;
        return {
            Button{"BACK", {ActionKind::Back}, {70, 64, 150, 54}, panel, ButtonStyle::Filled, false, 24},
            SliderItem{
                "FRAME RATE CAP", ActionKind::SetTargetFps,
                Slider({110, 385, 390, 52}, minTargetFps, maxTargetFps, 1, static_cast<float>(config.targetFps)), " FPS", 1, {}
            },
            Button{"UNLIMITED FPS", {ActionKind::ToggleUnlimitedFps}, {110, 484, 390, 54}, mint, ButtonStyle::Checkbox, config.unlimitedFps, 25},
            Button{"SHOW FPS", {ActionKind::ToggleFps}, {110, 564, 390, 54}, mint, ButtonStyle::Checkbox, config.showFps, 25},
            SliderItem{
                "TYPEWRITER SPEED", ActionKind::SetDialogueSpeed,
                Slider({605, 385, 390, 52}, 0, 240, 5, config.charactersPerSecond), " chars/sec", 1, "Instant"
            },
            Button{"AUDIO ENABLED", {ActionKind::ToggleMute}, {1100, 292, 390, 54}, mint, ButtonStyle::Checkbox, !config.muted, 25},
            SliderItem{
                "MASTER VOLUME", ActionKind::SetVolume,
                Slider({1100, 503, 390, 52}, 0, 1, 0.01f, config.masterVolume), "%", 100, {}
            }
        };
    }

    void DrawSettingsMenu(const Resources &resources, const GameConfig &config, const std::string &status) {
        using namespace theme;
        art::MenuBackdrop(resources, ArtId::Settings);
        DrawTextFit(resources, "SETTINGS", {270, 65, 1200, 44}, 44, paper);
        DrawTextFit(resources, "Adjust display, dialogue, and audio settings.", {272, 123, 1200, 23}, 23, muted);
        for (int i = 0; i < 3; ++i) {
            const float x = 70.0f + i * 495;
            DrawRectangleRec({x, 214, 460, 502}, panel);
            DrawRectangleRec({x, 214, 460, 4}, i == 1 ? gold : mint);
            DrawTextFit(resources, i == 0 ? "DISPLAY" : i == 1 ? "DIALOGUE" : "AUDIO", {x + 40, 246, 390, 24}, 24, muted);
        }
        DrawTextFit(resources, config.unlimitedFps ? "ACTIVE: UNLIMITED" : "ACTIVE: CAPPED", {110, 653, 390, 22}, 22, mint);
        DrawTextFit(resources, "Unlimited bypasses the saved cap.", {110, 682, 390, 19}, 19, muted);
        DrawTextFit(resources, "Text reveal speed.", {605, 492, 390, 23}, 23, gold);
        DrawTextFit(resources, "Set to 0 for instant text.", {605, 541, 390, 22}, 22, paper);
        DrawTextFit(resources, "ENTER reveals a message in play.", {605, 582, 390, 20}, 20, muted);
        DrawTextFit(resources, "T replays the current message.", {605, 617, 390, 20}, 20, muted);
        DrawTextFit(resources, "Volume is kept while muted.", {1100, 605, 390, 21}, 21, paper);
        DrawTextFit(resources, "Adjust the master audio volume.", {1100, 646, 390, 20}, 20, muted);
        DrawTextFit(resources, status.c_str(), {74, 764, 1200, 22}, 22, mint);
        DrawLine(70, 819, 1530, 819, border);
        DrawTextFit(resources, "DRAG / LEFT / RIGHT: ADJUST   UP / DOWN / TAB: SELECT   HOME / END: LIMITS   ESC: BACK",
                    {74, 849, 1450, 24}, 19, muted);
    }
}
