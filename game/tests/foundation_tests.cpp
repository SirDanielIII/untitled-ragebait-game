#include "engine/window/game_window.h"
#include "ui/dialogue/dialogue.h"

#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace rage
{
void TestLevelAuthoring();
#if defined(_WIN32) && defined(RAGE_DESKTOP_GLFW)
void TestWindowResize(void* handle);
#endif
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

InputFrame ClickAction(const Game& session, ActionKind kind, int value = 0)
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

InputFrame ClickSlider(const Game& session, ActionKind kind, float fraction)
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
    Game game(config);
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
    game.Update(0, ClickSlider(game, ActionKind::SetTargetFps, 0.5f));
    Check(config.targetFps == 195, "FPS slider did not apply its range");
    drag.mouse = {2000, 100};
    game.Update(0, drag);
    Check(config.targetFps == maxTargetFps, "FPS slider lost drag capture outside its bounds");
    game.Update(0, {});
    InputFrame minimum;
    minimum.minimum = true;
    game.Update(0, minimum);
    Check(config.targetFps == minTargetFps, "FPS slider keyboard minimum failed");
    InputFrame increase;
    increase.increase = true;
    game.Update(0, increase);
    Check(config.targetFps == minTargetFps + 1, "FPS slider keyboard step failed");
    game.Update(0, ClickAction(game, ActionKind::ToggleUnlimitedFps));
    Check(config.unlimitedFps && config.EffectiveTargetFps() == 0, "Unlimited did not remove the frame cap");
    game.Update(0, ClickSlider(game, ActionKind::SetTargetFps, 1));
    game.Update(0, {});
    Check(config.targetFps == maxTargetFps && config.EffectiveTargetFps() == 0, "Unlimited failed to retain a changed cap");
    game.Update(0, ClickAction(game, ActionKind::ToggleUnlimitedFps));
    Check(config.EffectiveTargetFps() == maxTargetFps, "Disabling Unlimited failed to restore the cap");
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
    store.values.unlimitedFps = true;
    store.values.targetFps = maxTargetFps;
    const auto saved = directory.path / "preferences/settings.yaml";
    Check(store.Save(saved), "Settings save failed");
    ConfigStore reloaded;
    Check(reloaded.Load(saved) && reloaded.values.windowWidth == 1280 && reloaded.values.moveSpeed == 310 && reloaded.values.lastLevel == 3, "Settings round trip failed");
    Check(reloaded.values.unlimitedFps && reloaded.values.EffectiveTargetFps() == 0 && reloaded.values.targetFps == maxTargetFps, "Unlimited/cap preferences round trip failed");
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

void TestCanvasPresentation(const Resources& resources, Rectangle destination)
{
    Image canvas = LoadImageFromTexture(resources.Canvas().texture);
    ImageFlipVertical(&canvas);
    Image screen = LoadImageFromScreen();
    const auto dpi = WindowPixelScale();
    Check(canvas.width == canvasWidth && canvas.height == canvasHeight,
          "Resizing changed the fixed logical canvas");
    // Sample solid regions of the real menu, away from filtered text and shape edges.
    for (const auto point : {Vector2{20, 20}, Vector2{700, 700}, Vector2{730, 730}, Vector2{1500, 800}})
    {
        const Color expected = GetImageColor(canvas, static_cast<int>(point.x), static_cast<int>(point.y));
        const Color actual = GetImageColor(screen,
            static_cast<int>((destination.x + point.x * destination.width / canvasWidth) * dpi.x),
            static_cast<int>((destination.y + point.y * destination.height / canvasHeight) * dpi.y));
        Check(std::abs(expected.r - actual.r) <= 2 && std::abs(expected.g - actual.g) <= 2 &&
              std::abs(expected.b - actual.b) <= 2, "The scaled canvas is misplaced or discolored");
    }
    if (destination.x * dpi.x >= 1)
        Check(ColorIsEqual(GetImageColor(screen, 0, screen.height / 2), BLACK), "Side letterbox is not black");
    if (destination.y * dpi.y >= 1)
        Check(ColorIsEqual(GetImageColor(screen, screen.width / 2, 0), BLACK), "Top letterbox is not black");
    UnloadImage(canvas);
    UnloadImage(screen);
}

void TestSession()
{
    GameConfig config;
    Game game(config);
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
        Check(game.Screen() == ScreenId::Playing && game.ActiveLevel()->Definition().id == level.id, "Stage selection failed");
        InputFrame move;
        move.move = 1;
        game.Update(0.05f, move);
        const auto position = game.ActiveLevel()->GetPlayer().Position();
        const auto bytes = game.ActiveLevel()->Dialogue().VisibleBytes();
        const float elapsed = game.ActiveLevel()->ElapsedSeconds();
        InputFrame pause;
        pause.pause = true;
        game.Update(0, pause);
        game.Update(0.05f, move);
        Check(game.Screen() == ScreenId::Paused && game.ActiveLevel()->GetPlayer().Position().x == position.x && game.ActiveLevel()->Dialogue().VisibleBytes() == bytes, "Pause advanced scene state");
        Check(game.ActiveLevel()->ElapsedSeconds() == elapsed, "Pause advanced level timing");
        game.Update(0, ClickAction(game, ActionKind::OpenSettings));
        game.Update(0.1f, {});
        Check(game.ActiveLevel()->ElapsedSeconds() == elapsed, "Settings advanced paused level timing");
        game.Update(0, ClickAction(game, ActionKind::ToggleFps));
        Check(game.Screen() == ScreenId::Settings && game.TakeSettingsDirty(), "Settings mutation failed");
        game.Update(0, ClickAction(game, ActionKind::Back));
        Check(game.Screen() == ScreenId::Paused && game.ActiveLevel()->GetPlayer().Position().x == position.x, "Settings lost paused scene");
        game.Update(0, ClickAction(game, ActionKind::Resume));
        Check(game.Screen() == ScreenId::Playing, "Resume failed");
        game.Update(0, pause);
        game.Update(0, ClickAction(game, ActionKind::Restart));
        Check(game.ActiveLevel()->GetPlayer().Position().x == level.spawn.x && game.ActiveLevel()->Dialogue().VisibleBytes() == 0, "ActiveLevel restart failed");
        game.Update(0, pause);
        game.Update(0, ClickAction(game, ActionKind::OpenLevels));
        Check(!game.ActiveLevel(), "Leaving a level retained the scene");
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
        TestLevelAuthoring();
        std::cout << "All foundation self-tests passed (no window/audio context required).\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}

int GameWindow::RunSmokeTest(const std::filesystem::path& captureDirectory)
{
#if !defined(PLATFORM_WEB)
    const int monitor = GetCurrentMonitor();
    Check(GetRenderWidth() <= GetMonitorWidth(monitor) && GetRenderHeight() <= GetMonitorHeight(monitor),
          "The initial window extends beyond the display after DPI scaling");
#endif
    // Exercise the actual asset loader and renderer with an isolated temporary export.
    // No real source assets or user preferences are changed.
    if (FileExists((FindResourceDirectory() / uiFontPath).string().c_str()))
        Check(resources.HasCustomFont(), "The configured font exists but did not load");
    {
        TestDirectory fixture;
        const auto spritePath = fixture.path / artPaths[static_cast<std::size_t>(ArtId::Student)];
        std::filesystem::create_directories(spritePath.parent_path());
        const Color marker{24, 188, 73, 255};
        Image sprite = GenImageColor(8, 16, marker);
        const bool exported = ExportImage(sprite, spritePath.string().c_str());
        UnloadImage(sprite);
        Check(exported, "Could not create artwork fixture");
        Resources fixtureResources(fixture.path);
        Check(!fixtureResources.HasCustomFont() && IsFontValid(fixtureResources.UiFont()), "Missing font did not fall back safely");
        BeginTextureMode(fixtureResources.Canvas());
        ClearBackground(BLACK);
        const bool drewSprite = fixtureResources.DrawArt(ArtId::Student, {0,0,80,80});
        const bool drewMissing = fixtureResources.DrawArt(ArtId::Entity, {100,0,80,80});
        EndTextureMode();
        Check(drewSprite && !drewMissing, "Optional artwork selection/fallback failed");
        Image rendered = LoadImageFromTexture(fixtureResources.Canvas().texture);
        ImageFlipVertical(&rendered);
        const Color center = GetImageColor(rendered, 40, 40);
        const Color margin = GetImageColor(rendered, 5, 40);
        UnloadImage(rendered);
        Check(center.r == marker.r && center.g == marker.g && center.b == marker.b && margin.r == 0,
              "Artwork did not render or preserve its aspect ratio");
    }
    std::cout << "PASS: custom font, missing-font fallback and optional texture replacement\n";
    auto click = [&](ActionKind kind, int value = 0) { Frame(0, ClickAction(game, kind, value)); };
    auto capture = [&](const char* name, bool screen = false)
    {
        if (captureDirectory.empty()) return;
        std::filesystem::create_directories(captureDirectory);
        Image image = screen ? LoadImageFromScreen() : LoadImageFromTexture(resources.Canvas().texture);
        if (!screen) ImageFlipVertical(&image);
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
        Check(game.ActiveLevel() && game.ActiveLevel()->Definition().id == level.id, "Graphical stage entry failed");
        Frame(1.0f / 60, {});
        const float start = game.ActiveLevel()->GetPlayer().Position().x;
        InputFrame right;
        right.move = 1;
        for (int i = 0; i < 30; ++i) Frame(1.0f / 60, right);
        Check(game.ActiveLevel()->GetPlayer().Position().x > start, "Graphical movement failed");
        InputFrame jump;
        jump.jump = true;
        const float beforeJump = game.ActiveLevel()->GetPlayer().Position().y;
        Frame(1.0f / 60, jump);
        Check(game.ActiveLevel()->GetPlayer().Position().y < beforeJump, "Graphical jump failed");
        InputFrame complete;
        complete.completeDialogue = true;
        Frame(0, complete);
        Check(game.ActiveLevel()->Dialogue().IsComplete(), "Graphical dialogue skip failed");
        capture(("stage_" + std::to_string(static_cast<int>(level.id))).c_str());
        InputFrame pause;
        pause.pause = true;
        Frame(0, pause);
        const auto position = game.ActiveLevel()->GetPlayer().Position();
        for (int i = 0; i < 5; ++i) Frame(1.0f / 60, right);
        Check(game.ActiveLevel()->GetPlayer().Position().x == position.x, "Graphical pause failed");
        capture("pause");
        click(ActionKind::OpenSettings);
        click(ActionKind::ToggleFps);
        click(ActionKind::ToggleMute);
        Frame(0, ClickSlider(game, ActionKind::SetTargetFps, 0.5f));
        Frame(0, {});
        Check(config.values.targetFps == 195, "Graphical FPS slider failed");
        click(ActionKind::ToggleUnlimitedFps);
        Check(config.values.EffectiveTargetFps() == 0, "Graphical Unlimited toggle failed");
        Frame(0, ClickSlider(game, ActionKind::SetVolume, 0.7f));
        Frame(0, {});
        Frame(0, ClickSlider(game, ActionKind::SetDialogueSpeed, 0.5f));
        Frame(0, {});
        Check(Near(config.values.masterVolume, 0.7f, 0.001f) && config.values.charactersPerSecond == 120, "Graphical sliders failed");
        capture("settings");
        click(ActionKind::ToggleUnlimitedFps);
        Check(config.values.EffectiveTargetFps() == 195, "Graphical cap restore failed");
        click(ActionKind::Back);
        click(ActionKind::Resume);
        Frame(0, pause);
        click(ActionKind::Restart);
        Check(game.ActiveLevel()->GetPlayer().Position().x == level.spawn.x, "Graphical scene reset failed");
        Frame(0, pause);
        click(ActionKind::OpenLevels);
    }
    click(ActionKind::MainMenu);
#if defined(RAGE_DESKTOP_GLFW)
    MaximizeWindow();
    PollInputEvents();
    Frame(0, {});
    Frame(0, {});
    Check(IsWindowMaximized(), "Maximize did not release the window aspect ratio");
    capture("maximized", true);
    TestCanvasPresentation(resources, Destination());
    RestoreWindow();
    PollInputEvents();
#if defined(_WIN32)
    TestWindowResize(GetWindowHandle());
#endif
#endif
    SetWindowSize(960, 540);
    Frame(0, {});
    const auto destination = Destination();
    const auto dpi = WindowPixelScale();
    Check(Near(destination.width * dpi.x, GetRenderWidth()) &&
          Near(destination.height * dpi.y, GetRenderHeight()) && Near(destination.x, 0) && Near(destination.y, 0),
          "A normal window has letterbox bars");
    InputFrame resizedClick = ClickAction(game, ActionKind::OpenSettings);
    const Vector2 windowPoint = {destination.x + resizedClick.mouse.x * destination.width / canvasWidth,
                                destination.y + resizedClick.mouse.y * destination.height / canvasHeight};
    resizedClick.mouse = CanvasCoordinates(windowPoint, destination);
    Frame(0, resizedClick);
    Check(game.Screen() == ScreenId::Settings, "Resized menu hit testing failed");
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
    config.values.showFps = false;
    for (const auto size : {Vector2{960, 540}, Vector2{1280, 720}, Vector2{1366, 768}, Vector2{1600, 900}, Vector2{1920, 1080}})
    {
        SetWindowSize(static_cast<int>(size.x), static_cast<int>(size.y));
        Frame(0, {});
        Frame(0, {});
        TestCanvasPresentation(resources, Destination());
        const auto suffix = std::to_string(static_cast<int>(size.x)) + "x" + std::to_string(static_cast<int>(size.y));
        capture(("main_" + suffix).c_str());
        click(ActionKind::OpenSettings);
        Frame(0, {});
        capture(("settings_" + suffix).c_str());
        click(ActionKind::Back);
        click(ActionKind::StartLevel);
        InputFrame reveal;
        reveal.completeDialogue = true;
        Frame(0, reveal);
        capture(("level_" + suffix).c_str());
        InputFrame pause;
        pause.pause = true;
        Frame(0, pause);
        click(ActionKind::MainMenu);
    }
#if defined(RAGE_DESKTOP_GLFW)
    // Exclusive fullscreen invalidates hidden-window readback on some drivers,
    // so check the mode and viewport after completing the windowed pixel checks.
    ToggleFullscreen();
    PollInputEvents();
    Frame(0, {});
    Check(IsWindowFullscreen(), "Fullscreen did not release the window aspect ratio");
    const auto fullscreen = Destination();
    Check(Near(fullscreen.width / fullscreen.height, 16.0f / 9) &&
          fullscreen.x >= -0.01f && fullscreen.y >= -0.01f &&
          fullscreen.x + fullscreen.width <= GetRenderWidth() + 0.01f &&
          fullscreen.y + fullscreen.height <= GetRenderHeight() + 0.01f,
          "Fullscreen did not fit the 16:9 canvas within the display");
    ToggleFullscreen();
    PollInputEvents();
    Check(!IsWindowFullscreen(), "Fullscreen did not restore the window");
#endif
    std::cout << "PASS: fixed canvas, scaled presentation, native resizing, maximize/fullscreen and menus at five window sizes\n";
    SetWindowSize(config.values.windowWidth, config.values.windowHeight);
    click(ActionKind::Quit);
    Check(game.WantsQuit(), "Graphical quit failed");
    std::cout << "PASS: real Raylib window/render/audio lifecycle, rendered menus and all four scenes, movement/jump, pause/resume/settings/restart, dialogue and quit.\n";
    std::cout << "Smoke test uses synthetic input frames and leaves user preferences untouched.\n";
    return 0;
}
}
