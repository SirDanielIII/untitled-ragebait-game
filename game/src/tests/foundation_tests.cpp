#include "../core/application.h"
#include "../dialogue/dialogue_box.h"

#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace rage
{
namespace
{
void Check(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

bool Near(float a, float b, float tolerance = 0.05f) { return std::abs(a - b) < tolerance; }

struct TestDirectory
{
    std::filesystem::path path = std::filesystem::temp_directory_path() /
        ("ragebait_foundation_test_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    TestDirectory() { std::filesystem::create_directories(path); }
    ~TestDirectory()
    {
        // This path is constructed solely beneath the system temp directory for this test.
        std::error_code error;
        std::filesystem::remove_all(path, error);
    }
};

InputFrame ClickAction(const GameSession& session, ActionKind kind, int value = 0)
{
    for (const auto& item : session.Navigation().Items())
    {
        const auto* button = std::get_if<Button>(&item);
        if (button && button->action.kind == kind && button->action.value == value)
        {
            InputFrame input;
            input.click = true;
            input.mouse = {button->bounds.x + button->bounds.width / 2, button->bounds.y + button->bounds.height / 2};
            return input;
        }
    }
    throw std::runtime_error("Requested menu action is missing");
}

InputFrame ClickSlider(const GameSession& session, ActionKind kind, float fraction)
{
    for (const auto& item : session.Navigation().Items())
    {
        const auto* slider = std::get_if<SliderItem>(&item);
        if (slider && slider->action == kind)
        {
            const auto bounds = slider->slider.Bounds();
            InputFrame input;
            input.click = true;
            input.mouseDown = true;
            input.mouse = {bounds.x + 20 + fraction * (bounds.width - 40), bounds.y + bounds.height / 2};
            return input;
        }
    }
    throw std::runtime_error("Requested slider is missing");
}

void TestSliders()
{
    const auto destination = CanvasDestination(960, 720);
    Check(Near(destination.y, 90) && Near(destination.width, 960) && Near(destination.height, 540), "Letterbox sizing failed");
    const auto mapped = CanvasCoordinates({480, 360}, destination);
    Check(Near(mapped.x, 800) && Near(mapped.y, 450), "Mouse-to-canvas mapping failed");
    Slider slider({100, 100, 400, 60}, 0, 1, 0.01f, 0.5f);
    InputFrame click;
    click.click = true;
    click.mouseDown = true;
    click.mouse = {slider.PositionFromValue(0.25f), 130};
    Check(slider.Update(click, true) && Near(slider.Value(), 0.25f) && slider.IsDragging(), "Slider track click failed");
    InputFrame drag;
    drag.mouseDown = true;
    drag.mouse = {900, 300};
    Check(slider.Update(drag, true) && Near(slider.Value(), 1), "Slider outside-track drag/clamp failed");
    slider.Update({}, true);
    Check(!slider.IsDragging(), "Slider release did not clear pointer capture");
    InputFrame keyboard;
    keyboard.decrease = true;
    Check(slider.Update(keyboard, true) && Near(slider.Value(), 0.99f, 0.001f), "Slider keyboard step failed");
    Check(!slider.Update(keyboard, false), "Unfocused slider responded to keyboard");
    keyboard = {};
    keyboard.minimum = true;
    slider.Update(keyboard, true);
    Check(slider.Value() == 0, "Slider minimum shortcut failed");
    keyboard = {};
    keyboard.maximum = true;
    slider.Update(keyboard, true);
    Check(slider.Value() == 1, "Slider maximum shortcut failed");
    Slider text({0, 0, 400, 60}, 0, 240, 5, 40);
    text.SetValue(37);
    Check(text.Value() == 37, "Slider changed a valid programmatic configuration value");
    Check(Near(text.ValueFromPosition(text.PositionFromValue(120)), 120), "Generic slider range mapping failed");
    GameConfig config;
    GameSession game(config);
    game.Update(0, ClickAction(game, ActionKind::OpenSettings));
    game.Update(0, ClickSlider(game, ActionKind::SetVolume, 0.3f));
    Check(Near(config.masterVolume, 0.3f, 0.001f), "Volume slider did not update config");
    drag.mouse = {1500, 585};
    game.Update(0, drag);
    Check(config.masterVolume > 0.9f, "Slider capture was lost when settings changed");
    game.Update(0, {});
    game.Update(0, ClickSlider(game, ActionKind::SetDialogueSpeed, 0.5f));
    game.Update(0, {});
    Check(config.charactersPerSecond == 120, "Dialogue slider did not use its independent range");
    game.Update(0, ClickAction(game, ActionKind::SetTargetFps, 75));
    Check(config.targetFps == 75, "Frame-rate selection failed");
    std::cout << "PASS: reusable sliders, track clicks, captured/clamped dragging, keyboard limits, settings and frame-rate choices\n";
}

void TestDialogue()
{
    Typewriter writer;
    Check(writer.IsComplete(), "Empty dialogue should be complete");
    writer.SetSpeed(8);
    writer.Replace("First\r\nSecond\n\nLast\n");
    writer.Update(0.5f);
    Check(writer.VisibleText() == "Firs", "Typewriter rate is incorrect");
    Typewriter partitioned;
    partitioned.SetSpeed(8);
    partitioned.Replace(writer.Text());
    for (int i = 0; i < 4; ++i) partitioned.Update(0.125f);
    Check(partitioned.VisibleText() == writer.VisibleText(), "Reveal depends on frame partition");
    writer.Complete();
    Check(writer.VisibleText() == "First\nSecond\n\nLast\n", "Multiline/CRLF reveal failed");
    writer.Reset();
    Check(writer.VisibleText().empty(), "Reset retained previous characters");
    writer.Update(0.125f);
    writer.Replace("New");
    Check(writer.VisibleText().empty(), "Replacement retained old state");
    writer.Update(-1);
    Check(writer.VisibleText().empty(), "Negative time advanced text");
    writer.SetSpeed(0);
    Check(writer.VisibleText() == "New", "Zero speed should reveal instantly");
    writer.Replace("");
    Check(writer.IsComplete(), "Empty replacement is not complete");
    writer.SetSpeed(8);
    writer.Replace("A\xC3\xA9" "B");
    writer.Update(0.25f);
    Check(writer.VisibleBytes() == 3, "UTF-8 character was split");
    const TextMeasure measure = [](std::string_view text) { return static_cast<float>(text.size()); };
    const auto lines = WrapText("one two\n\nabcdef\n", 4, measure);
    Check(lines.size() == 6, "Word wrap/explicit blank lines are incorrect");
    Check(lines[0].begin == 0 && lines[0].end == 3 && lines[1].begin == 4 && lines[1].end == 7, "Word wrapping offsets are incorrect");
    Check(lines[2].begin == lines[2].end && lines.back().begin == lines.back().end, "Blank/trailing lines were lost");
    Check(WrapText("abcdef", 0, measure).size() == 6, "Narrow layout failed to split a long word");
    const auto unicode = WrapText("\xC3\xA9\xC3\xA9", 2, measure);
    Check(unicode.size() == 2 && unicode[0].end == 2 && unicode[1].begin == 2, "Wrapping split a UTF-8 character");
    std::cout << "PASS: multiline reveal, timing, reset/replace/skip, wrapping and UTF-8 boundaries\n";
}

void TestConfig()
{
    TestDirectory directory;
    ConfigStore store;
    Check(store.values.windowWidth == 1600 && store.values.windowHeight == 900, "Default resolution is incorrect");
    Check(!store.Load(directory.path / "missing.yaml") && store.values.moveSpeed == 260, "Missing config defaults failed");
    const auto file = directory.path / "input.yaml";
    {
        std::ofstream output(file);
        output << "window:\n  width: 1280\n  target_fps: 120\n  show_fps: true\n"
               << "player:\n  move_speed: 310 # pixels per second\n  gravity: broken\n  jump_speed: -3\n"
               << "audio:\n  master_volume: 0.7\n  muted: FALSE\n"
               << "dialogue:\n  characters_per_second: 0\nsession:\n  last_level: 3\n";
    }
    Check(!store.Load(file), "Malformed config should report ignored values");
    Check(store.values.windowWidth == 1280 && store.values.targetFps == 120 && store.values.showFps, "Valid display settings were lost");
    Check(store.values.moveSpeed == 310 && store.values.gravity == 1500 && store.values.jumpSpeed == 620, "Player config fallback failed");
    Check(Near(store.values.masterVolume, 0.7f) && !store.values.muted && store.values.charactersPerSecond == 0 && store.values.lastLevel == 3, "Valid audio/dialogue/session config failed");
    const auto saved = directory.path / "preferences/settings.yaml";
    Check(store.Save(saved), "Settings save failed");
    ConfigStore reloaded;
    Check(reloaded.Load(saved) && reloaded.values.windowWidth == 1280 && reloaded.values.moveSpeed == 310 && reloaded.values.lastLevel == 3, "Settings round trip failed");
    {
        std::ofstream output(file);
        output << "window:\n  width: 900\n  width: 1000\n  height: 99999\n\tbad: true\n  width: 1100\n"
               << "audio:\n  master_volume: nan\nunknown:\n  move_speed: 600\n";
    }
    Check(!reloaded.Load(file) && reloaded.values.windowWidth == 900 && reloaded.values.windowHeight == 900 && reloaded.values.moveSpeed == 310, "Duplicate/unknown/indentation validation failed");
    std::cout << "PASS: defaults, valid/malformed YAML subset, range checks and preferences round trip\n";
}

void TestMovement()
{
    GameConfig config;
    const Rectangle world = {0, 0, 2000, 700};
    const std::vector<Rectangle> platforms = {{0, 650, 2000, 50}};
    InputFrame right;
    right.move = 1;
    Player player;
    player.Reset({100, 606});
    player.Update(1.0f / 60, {}, config, platforms, world);
    Check(player.Grounded(), "Initial floor contact failed");
    for (int i = 0; i < 60; ++i) player.Update(1.0f / 60, right, config, platforms, world);
    Check(Near(player.Position().x, 360, 0.15f), "Movement is not in pixels per second");
    Player slowFrames;
    slowFrames.Reset({100, 606});
    for (int i = 0; i < 30; ++i) slowFrames.Update(1.0f / 30, right, config, platforms, world);
    Check(Near(player.Position().x, slowFrames.Position().x, 0.15f), "Horizontal movement depends on frame rate");
    InputFrame jump;
    jump.jump = true;
    player.Update(1.0f / 60, jump, config, platforms, world);
    Check(player.Position().y < 606 && !player.Grounded(), "Jump did not leave the floor");
    for (int i = 0; i < 120; ++i) player.Update(1.0f / 60, {}, config, platforms, world);
    Check(player.Grounded() && Near(player.Position().y, 606), "Landing/reset of vertical velocity failed");
    player.Reset({1955, 606});
    player.Update(0.1f, right, config, platforms, world);
    Check(Near(player.Position().x, 1970), "World boundary clamp failed");
    player.Reset({100, 606});
    const std::vector<Rectangle> wall = {{0, 650, 2000, 50}, {150, 550, 30, 100}};
    player.Update(0.1f, right, config, wall, world);
    Check(Near(player.Position().x, 120), "Side collision failed");
    player.Reset({100, 606});
    Check(player.Velocity().x == 0 && player.Velocity().y == 0 && !player.Grounded(), "Player reset retained motion");
    for (const float irregularDt : {1.0f / 60 + 0.00001f, 0.017f, 0.00834f, 0.01668f})
    {
        Player irregular;
        irregular.Reset({100, 606});
        for (int i = 0; i < 4; ++i) irregular.Update(irregularDt, {}, config, platforms, world);
        const float before = irregular.Position().y;
        irregular.Update(irregularDt, jump, config, platforms, world);
        Check(irregular.Position().y < before, "Irregular frame interval lost floor contact and rejected jump");
    }
    player.Reset({100, 606});
    player.Update(0.017f, jump, config, platforms, world);
    Check(player.Position().y < 606, "Jump immediately after spawn/reset failed");
    std::cout << "PASS: frame-independent walking, jump/landing, collision, bounds and reset\n";
}

void TestSession()
{
    GameConfig config;
    GameSession game(config);
    InputFrame down;
    down.down = true;
    game.Update(0, down);
    game.Update(0, down);
    InputFrame confirm;
    confirm.confirm = true;
    game.Update(0, confirm);
    Check(game.Screen() == ScreenId::LevelSelect, "Keyboard menu selection failed");
    for (const auto& level : LevelCatalog())
    {
        game.Update(0, ClickAction(game, ActionKind::StartLevel, static_cast<int>(level.id)));
        Check(game.Screen() == ScreenId::Playing && game.Scene()->Definition().id == level.id, "Stage selection failed");
        InputFrame move;
        move.move = 1;
        game.Update(0.05f, move);
        const auto position = game.Scene()->Student().Position();
        const auto bytes = game.Scene()->Dialogue().VisibleBytes();
        InputFrame pause;
        pause.pause = true;
        game.Update(0, pause);
        game.Update(0.05f, move);
        Check(game.Screen() == ScreenId::Paused && game.Scene()->Student().Position().x == position.x && game.Scene()->Dialogue().VisibleBytes() == bytes, "Pause advanced scene state");
        game.Update(0, ClickAction(game, ActionKind::OpenSettings));
        game.Update(0, ClickAction(game, ActionKind::ToggleFps));
        Check(game.Screen() == ScreenId::Settings && game.TakeSettingsDirty(), "Settings mutation failed");
        game.Update(0, ClickAction(game, ActionKind::Back));
        Check(game.Screen() == ScreenId::Paused && game.Scene()->Student().Position().x == position.x, "Settings lost paused scene");
        game.Update(0, ClickAction(game, ActionKind::Resume));
        Check(game.Screen() == ScreenId::Playing, "Resume failed");
        game.Update(0, pause);
        game.Update(0, ClickAction(game, ActionKind::Restart));
        Check(game.Scene()->Student().Position().x == level.spawn.x && game.Scene()->Dialogue().VisibleBytes() == 0, "Scene restart failed");
        game.Update(0, pause);
        game.Update(0, ClickAction(game, ActionKind::OpenLevels));
        Check(!game.Scene(), "Leaving a level retained the scene");
    }
    InputFrame back;
    back.back = true;
    game.Update(0, back);
    Check(game.Screen() == ScreenId::MainMenu, "Selection back navigation failed");
    game.Update(0, ClickAction(game, ActionKind::Quit));
    Check(game.WantsQuit(), "Quit action failed");
    std::cout << "PASS: keyboard/mouse menus, all four scenes, pause/settings/resume, restart and quit\n";
}
}

int RunSelfTests()
{
    try
    {
        TestDialogue();
        TestConfig();
        TestMovement();
        TestSliders();
        TestSession();
        std::cout << "All foundation self-tests passed (no window/audio context required).\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}

int Application::RunSmokeTest(const std::filesystem::path& captureDirectory)
{
    auto click = [&](ActionKind kind, int value = 0) { Frame(0, ClickAction(session, kind, value)); };
    auto capture = [&](const char* name)
    {
        if (captureDirectory.empty()) return;
        std::filesystem::create_directories(captureDirectory);
        Image image = LoadImageFromTexture(resources.Canvas().texture);
        ImageFlipVertical(&image);
        const auto path = captureDirectory / (std::string(name) + ".png");
        const bool saved = ExportImage(image, path.string().c_str());
        UnloadImage(image);
        Check(saved, "Could not capture a rendered smoke-test frame");
    };
    Frame(0, {});
    capture("main_menu");
    click(ActionKind::OpenLevels);
    capture("level_select");
    for (const auto& level : LevelCatalog())
    {
        click(ActionKind::StartLevel, static_cast<int>(level.id));
        Check(session.Scene() && session.Scene()->Definition().id == level.id, "Graphical stage entry failed");
        Frame(1.0f / 60, {});
        const float start = session.Scene()->Student().Position().x;
        InputFrame right;
        right.move = 1;
        for (int i = 0; i < 30; ++i) Frame(1.0f / 60, right);
        Check(session.Scene()->Student().Position().x > start, "Graphical movement failed");
        InputFrame jump;
        jump.jump = true;
        const float beforeJump = session.Scene()->Student().Position().y;
        Frame(1.0f / 60, jump);
        Check(session.Scene()->Student().Position().y < beforeJump, "Graphical jump failed");
        InputFrame complete;
        complete.completeDialogue = true;
        Frame(0, complete);
        Check(session.Scene()->Dialogue().IsComplete(), "Graphical dialogue skip failed");
        capture(("stage_" + std::to_string(static_cast<int>(level.id))).c_str());
        InputFrame pause;
        pause.pause = true;
        Frame(0, pause);
        const auto position = session.Scene()->Student().Position();
        for (int i = 0; i < 5; ++i) Frame(1.0f / 60, right);
        Check(session.Scene()->Student().Position().x == position.x, "Graphical pause failed");
        capture("pause");
        click(ActionKind::OpenSettings);
        click(ActionKind::ToggleFps);
        click(ActionKind::ToggleMute);
        Frame(0, ClickSlider(session, ActionKind::SetVolume, 0.7f));
        Frame(0, {});
        Frame(0, ClickSlider(session, ActionKind::SetDialogueSpeed, 0.5f));
        Frame(0, {});
        Check(Near(config.values.masterVolume, 0.7f, 0.001f) && config.values.charactersPerSecond == 120, "Graphical sliders failed");
        capture("settings");
        click(ActionKind::Back);
        click(ActionKind::Resume);
        Frame(0, pause);
        click(ActionKind::Restart);
        Check(session.Scene()->Student().Position().x == level.spawn.x, "Graphical scene reset failed");
        Frame(0, pause);
        click(ActionKind::OpenLevels);
    }
    click(ActionKind::MainMenu);
    SetWindowSize(960, 720);
    Frame(0, {});
    const auto destination = Destination();
    Check(Near(destination.width, 960) && Near(destination.height, 540) && Near(destination.y, 90), "Resized render destination failed");
    InputFrame resizedClick = ClickAction(session, ActionKind::OpenSettings);
    const Vector2 windowPoint = {destination.x + resizedClick.mouse.x * destination.width / canvasWidth,
                                destination.y + resizedClick.mouse.y * destination.height / canvasHeight};
    resizedClick.mouse = CanvasCoordinates(windowPoint, destination);
    Frame(0, resizedClick);
    Check(session.Screen() == ScreenId::Settings, "Resized menu hit testing failed");
    Frame(0, {}); // Fill both swap buffers before reading the window screenshot.
    if (!captureDirectory.empty())
    {
        Image resized = LoadImageFromScreen();
        const auto path = captureDirectory / "resized_options.png";
        const bool saved = ExportImage(resized, path.string().c_str());
        UnloadImage(resized);
        Check(saved, "Resized screenshot failed");
    }
    click(ActionKind::Back);
    SetWindowSize(config.values.windowWidth, config.values.windowHeight);
    click(ActionKind::Quit);
    Check(session.WantsQuit(), "Graphical quit failed");
    std::cout << "PASS: real Raylib window/render/audio lifecycle, rendered menus and all four scenes, movement/jump, pause/resume/settings/restart, dialogue and quit.\n";
    std::cout << "Smoke test uses synthetic input frames and leaves user preferences untouched.\n";
    return 0;
}
}
