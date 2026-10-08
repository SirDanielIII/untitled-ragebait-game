#include "game_session.h"
#include "../core/layout.h"
#include "../dialogue/dialogue_box.h"

#include <algorithm>
#include <string>
#include <utility>

namespace rage
{
namespace
{
constexpr Color cream = {253, 249, 175, 255};
constexpr Color green = {0, 194, 88, 255};
constexpr Color blue = {8, 137, 177, 255};
constexpr Color red = {252, 79, 83, 255};
constexpr Color cyan = {0, 174, 213, 255};

void CenteredText(Font font, const char* text, float y, float size, Color color)
{
    const Vector2 measured = MeasureTextEx(font, text, size, 2);
    DrawTextEx(font, text, {(canvasWidth - measured.x) / 2, y}, size, 2, color);
}

void Paragraph(Font font, const std::string& text, Vector2 position, float width, float size = 26)
{
    const auto lines = WrapText(text, width, [&](std::string_view value)
    {
        return MeasureTextEx(font, std::string(value).c_str(), size, 1).x;
    });
    for (std::size_t i = 0; i < lines.size(); ++i)
        DrawTextEx(font, text.substr(lines[i].begin, lines[i].end - lines[i].begin).c_str(),
                   {position.x, position.y + i * (size + 10)}, size, 1, RAYWHITE);
}

void DrawCharacterPlaceholders(Font font)
{
    // Plain shapes stand in for the student and Entity, in the reference's character area.
    DrawRectangle(990, 578, 105, 155, Color{231, 204, 129, 255});
    DrawRectangleLines(990, 578, 105, 155, BLACK);
    DrawLineEx({1010, 612}, {1034, 612}, 5, BLACK);
    DrawLineEx({1052, 612}, {1076, 612}, 5, BLACK);
    DrawLineEx({1042, 658}, {1064, 658}, 4, BLACK);
    DrawLineEx({1010, 733}, {985, 778}, 7, Color{15, 15, 15, 255});
    DrawLineEx({1076, 733}, {1104, 778}, 7, Color{15, 15, 15, 255});
    DrawTextEx(font, "STUDENT", {976, 808}, 24, 1, cream);
    DrawCircle(1350, 610, 98, Color{20, 20, 20, 255});
    DrawCircleLines(1350, 610, 98, Color{183, 104, 198, 255});
    DrawRectangle(1298, 586, 30, 12, red);
    DrawRectangle(1370, 586, 30, 12, red);
    DrawLineEx({1314, 646}, {1388, 646}, 5, red);
    DrawTextEx(font, "THE ENTITY", {1270, 808}, 24, 1, cream);
}
}

GameSession::GameSession(GameConfig& config) : config(config) { BuildMenu(); }

void GameSession::Enter(ScreenId next)
{
    screen = next;
    if (screen == ScreenId::MainMenu || screen == ScreenId::LevelSelect) scene.reset();
    BuildMenu();
}

void GameSession::BuildMenu(bool preserveFocus)
{
    std::vector<MenuItem> items;
    auto add = [&](std::string label, ActionKind kind, Rectangle bounds, int value = 0,
                   Color fill = blue, ButtonStyle style = ButtonStyle::Filled, bool checked = false, float size = 30)
    {
        items.emplace_back(Button{std::move(label), {kind, value}, bounds, fill, style, checked, size});
    };
    switch (screen)
    {
        case ScreenId::MainMenu:
            add("PLAY GAME", ActionKind::StartLevel, {100, 450, 650, 130}, 0, green, ButtonStyle::Filled, false, 54);
            add("OPTIONS", ActionKind::OpenSettings, {100, 625, 300, 95}, 0, blue, ButtonStyle::Filled, false, 32);
            add("SELECT STAGE", ActionKind::OpenLevels, {450, 625, 300, 95}, 0, blue, ButtonStyle::Filled, false, 30);
            add("QUIT", ActionKind::Quit, {450, 765, 300, 85}, 0, red, ButtonStyle::Filled, false, 38);
            break;
        case ScreenId::LevelSelect:
            for (const auto& level : LevelCatalog())
                add(level.title, ActionKind::StartLevel, {100, 240.0f + items.size() * 94.0f, 650, 68}, static_cast<int>(level.id));
            add("BACK TO MAIN MENU", ActionKind::MainMenu, {100, 640, 650, 68});
            break;
        case ScreenId::Settings:
            add("Back", ActionKind::Back, {24, 24, 70, 70}, 0, cyan, ButtonStyle::BackArrow);
            for (int fps : {30, 60, 75, 120, 165})
                add(std::to_string(fps) + " FPS", ActionKind::SetTargetFps,
                    {100, 340.0f + (items.size() - 1) * 82.0f, 360, 54}, fps, green, ButtonStyle::Checkbox, config.targetFps == fps, 28);
            add("SHOW FPS", ActionKind::ToggleFps, {100, 770, 390, 54}, 0, green, ButtonStyle::Checkbox, config.showFps, 28);
            items.emplace_back(SliderItem{"TEXT SPEED", ActionKind::SetDialogueSpeed,
                Slider({600, 452, 390, 60}, 0, 240, 5, config.charactersPerSecond), " characters/sec", 1, "Instant"});
            add("AUDIO ENABLED", ActionKind::ToggleMute, {1120, 340, 420, 54}, 0, Color{249, 75, 163, 255}, ButtonStyle::Checkbox, !config.muted, 27);
            items.emplace_back(SliderItem{"MASTER VOLUME", ActionKind::SetVolume,
                Slider({1120, 555, 390, 60}, 0, 1, 0.01f, config.masterVolume), "%", 100, {}});
            break;
        case ScreenId::Paused:
            add("RESUME", ActionKind::Resume, {100, 240, 650, 68}, 0, green);
            add("RESET SCENE", ActionKind::Restart, {100, 334, 650, 68});
            add("OPTIONS", ActionKind::OpenSettings, {100, 428, 650, 68});
            add("SELECT STAGE", ActionKind::OpenLevels, {100, 522, 650, 68});
            add("MAIN MENU", ActionKind::MainMenu, {100, 616, 650, 68}, 0, red);
            break;
        case ScreenId::Playing: break;
    }
    menu.SetItems(std::move(items), preserveFocus);
}

bool GameSession::Update(float dt, const InputFrame& input)
{
    if (screen == ScreenId::Playing)
    {
        if (input.pause) { Enter(ScreenId::Paused); return true; }
        if (scene) scene->Update(dt, input, config);
        return false;
    }
    if (screen == ScreenId::Paused && (input.pause || input.back)) { Enter(ScreenId::Playing); return true; }
    if (input.back)
    {
        if (screen == ScreenId::Settings) Enter(settingsReturn);
        else if (screen == ScreenId::LevelSelect) Enter(ScreenId::MainMenu);
        return true;
    }
    const auto action = menu.Update(input);
    if (!action) return false;
    Apply(*action);
    // Continuous slider changes are silent; ordinary button actions play one click.
    return action->kind != ActionKind::SetVolume && action->kind != ActionKind::SetDialogueSpeed;
}

void GameSession::Apply(Action action)
{
    switch (action.kind)
    {
        case ActionKind::StartLevel:
            scene = std::make_unique<LevelScene>(FindLevel(static_cast<LevelId>(action.value)), config);
            config.lastLevel = action.value;
            settingsDirty = true;
            Enter(ScreenId::Playing);
            return;
        case ActionKind::OpenLevels: Enter(ScreenId::LevelSelect); return;
        case ActionKind::OpenSettings: settingsReturn = screen; Enter(ScreenId::Settings); return;
        case ActionKind::Back: Enter(settingsReturn); return;
        case ActionKind::MainMenu: Enter(ScreenId::MainMenu); return;
        case ActionKind::Quit: quit = true; return;
        case ActionKind::Resume: Enter(ScreenId::Playing); return;
        case ActionKind::Restart: if (scene) scene->Reset(config); Enter(ScreenId::Playing); return;
        case ActionKind::ToggleFps: config.showFps = !config.showFps; break;
        case ActionKind::ToggleMute: config.muted = !config.muted; break;
        case ActionKind::SetTargetFps: config.targetFps = action.value; break;
        case ActionKind::SetVolume:
            config.masterVolume = std::clamp(action.scalar, 0.0f, 1.0f);
            settingsDirty = true;
            return; // Retain the widget's drag capture and keyboard focus.
        case ActionKind::SetDialogueSpeed:
            config.charactersPerSecond = std::clamp(action.scalar, 0.0f, 240.0f);
            settingsDirty = true;
            return;
    }
    settingsDirty = true;
    BuildMenu(true);
}

bool GameSession::TakeSettingsDirty()
{
    const bool dirty = settingsDirty;
    settingsDirty = false;
    return dirty;
}

void GameSession::Draw(Font font, const std::string& settingsStatus) const
{
    if (screen == ScreenId::Playing) { if (scene) scene->Draw(font); return; }
    if (screen == ScreenId::MainMenu)
    {
        DrawRectangle(0, 0, canvasWidth, canvasHeight, Color{22, 22, 22, 255});
        DrawRectangle(0, 410, canvasWidth, canvasHeight - 410, Color{50, 50, 50, 255});
        DrawRectangle(300, 64, 1000, 8, cream);
        CenteredText(font, "UNTITLED", 95, 106, green);
        CenteredText(font, "RAGEBAIT GAME", 212, 88, green);
        DrawRectangle(300, 330, 380, 7, cream);
        DrawRectangle(920, 330, 380, 7, cream);
        CenteredText(font, "CISC 320", 318, 30, Color{255, 183, 21, 255});
        CenteredText(font, "LATE FOR CLASS. TRUST YOUR ASSISTANT?", 362, 25, cyan);
        menu.Draw(font);
        Paragraph(font, "A/D or arrows: move\nSpace/W/Up: jump\nEsc/P: pause", {100, 758}, 310, 21);
        DrawCharacterPlaceholders(font);
        DrawTextEx(font, "Group 7 / Placeholder foundation", {100, 875}, 18, 1, cream);
        return;
    }
    if (screen == ScreenId::Settings)
    {
        DrawRectangle(0, 0, canvasWidth, canvasHeight, Color{32, 32, 32, 255});
        CenteredText(font, "OPTIONS", 62, 46, cream);
        DrawTextEx(font, "VIDEO", {100, 232}, 45, 1, cream);
        DrawTextEx(font, "DIALOGUE", {600, 232}, 45, 1, cream);
        DrawTextEx(font, "AUDIO", {1120, 232}, 45, 1, cream);
        menu.Draw(font);
        Paragraph(font, "0 = instant reveal.\nDrag the handle or use\nLeft/Right when selected.", {600, 570}, 410, 24);
        Paragraph(font, "Menu sound effects only.\nVolume is remembered\nwhile audio is muted.", {1120, 675}, 400, 24);
        Paragraph(font, settingsStatus, {600, 752}, 410, 21);
        DrawTextEx(font, "Mouse: click/drag   Up/Down/Tab: select   Left/Right: adjust   Home/End: limits   Esc: back",
                   {100, 866}, 21, 1, cream);
        return;
    }
    if (scene)
    {
        scene->Draw(font);
        DrawRectangle(0, 0, canvasWidth, canvasHeight, Color{24, 24, 24, 242});
    }
    else DrawRectangle(0, 0, canvasWidth, canvasHeight, Color{32, 32, 32, 255});
    const bool paused = screen == ScreenId::Paused;
    DrawTextEx(font, paused ? "PAUSED" : "SELECT STAGE", {100, 65}, 54, 1, cream);
    DrawTextEx(font, paused ? "The player, camera and dialogue are frozen." : "Four outline stages, ready for the team to develop.",
               {100, 151}, 27, 1, RAYWHITE);
    menu.Draw(font);
    if (screen == ScreenId::LevelSelect && menu.Selected() < static_cast<int>(LevelCatalog().size()))
    {
        const auto& level = LevelCatalog()[menu.Selected()];
        DrawTextEx(font, "SCENE PREVIEW", {840, 252}, 30, 1, level.accent);
        Paragraph(font, level.description, {840, 323}, 650);
        Paragraph(font, level.plannedContent, {840, 465}, 650);
        DrawTextEx(font, "Recently selected:", {840, 650}, 24, 1, cream);
        Paragraph(font, FindLevel(static_cast<LevelId>(config.lastLevel)).title, {840, 693}, 650, 24);
    }
    else if (paused)
        Paragraph(font, "Resume this scene, reset it,\nor choose another stage.\n\nGameplay rules and content\nremain placeholders.", {840, 252}, 650);
    DrawTextEx(font, "Mouse or Up/Down/W/S/Tab to navigate. Enter to choose. Esc to go back.", {100, 838}, 25, 1, cream);
}
}
