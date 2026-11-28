#include "engine/window/game_window.h"
#include "test_support.h"
#include "ui/dialogue/dialogue.h"
#include "ui/menus/menus.h"
#include "ui/theme.h"
#include "engine/graphics/placeholder_art.h"
#include "rlgl.h"

#include <fstream>
#include <iostream>

namespace rage
{
void TestLevelAuthoring();
void TestEventManager();
void TestGameData();
void TestAssetRendering();
void TestMappedLevelSpriteRendering();
void TestAudioData();
void TestAudioPlayback();
void TestObjects();
void TestCollisions();
void TestBirds();
void TestBirdRendering();
void TestPlayerMovement();
void TestPlayerTuning();
void TestCrouchJumps();
void TestJumpParticleRendering();
#if defined(_WIN32) && defined(RAGE_DESKTOP_GLFW)
void TestWindowResize(void* handle);
#endif
namespace
{
using test::Check;
using test::ClickAction;
using test::Near;
using test::TestDirectory;

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
    // Verify aspect fit and inverse input mapping, including fractional and wide/tall outputs.
    for (const auto size : {Vector2{640, 360}, Vector2{1366, 768}, Vector2{1920, 1200}, Vector2{2560, 1080}, Vector2{900, 1600}, Vector2{3840, 2160}}) {
        const auto rect = CanvasDestination(size.x, size.y);
        Check(Near(rect.width / canvasWidth, rect.height / canvasHeight, 0.0001f), "Viewport is not uniform");
        const auto matrix = ViewportMatrix(rect);
        const Vector2 point{123.4f, 567.8f};
        const auto physical = Vector2Transform(point, matrix);
        const auto restored = CanvasCoordinates(physical, rect);
        Check(Near(restored.x, point.x) && Near(restored.y, point.y), "Viewport/input transform mismatch");
        Camera2D camera{}; camera.zoom = 2; camera.target = {100, 200}; camera.offset = {800, 330};
        const auto composed = Vector2Transform(point, CameraModelview(camera, matrix));
        const auto expected = Vector2Transform(GetWorldToScreen2D(point, camera), matrix);
        Check(Near(composed.x, expected.x) && Near(composed.y, expected.y), "Camera changed with framebuffer scale");
    }
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
#if !defined(PLATFORM_WEB)
    Check(!config.fullscreen, "Fullscreen should default to off");
    game.TakeSettingsDirty();
    game.Update(0, ClickAction(game, ActionKind::ToggleFullscreen));
    Check(config.fullscreen && game.TakeSettingsDirty(), "Fullscreen checkbox failed");
    const auto selected = game.Navigation().Selected();
    InputFrame shortcut;
    shortcut.confirm = true; // A shortcut wins over menu confirmation.
    shortcut.completeDialogue = true;
    shortcut.toggleFullscreen = true;
    game.Update(0, shortcut);
    Check(!config.fullscreen && game.TakeSettingsDirty() && game.Screen() == ScreenId::Settings &&
          game.Navigation().Selected() == selected, "Fullscreen shortcut did not preserve the settings screen/focus");
    const auto &checkbox = std::get<Button>(game.Navigation().Items()[selected]);
    Check(checkbox.action.kind == ActionKind::ToggleFullscreen && !checkbox.checked, "Fullscreen checkbox became stale");
    Check(FullscreenShortcut(true, false, false) && FullscreenShortcut(false, true, true) &&
          FullscreenShortcut(true, true, true) && !FullscreenShortcut(false, true, false) &&
          !FullscreenShortcut(false, false, true), "Fullscreen shortcut detection failed");
#endif
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
    Check(ConfigStore::DefaultSettingsPath() == std::filesystem::path(GetApplicationDirectory()) / "settings.yaml",
          "Default settings should be beside the executable");
    TestDirectory directory;
    ConfigStore store;
    Check(store.values.windowWidth == 1600 && store.values.windowHeight == 900, "Default resolution is incorrect");
    Check(!store.Load(directory.path / "missing.yaml") && store.values.moveSpeed == GameConfig{}.moveSpeed, "Missing config defaults failed");
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
    Check(store.values.moveSpeed == 310 && store.values.gravity == GameConfig{}.gravity && store.values.jumpSpeed == -3, "Player config fallback failed");
    Check(Near(store.values.masterVolume, 0.7f, 0.001f) && !store.values.muted && store.values.charactersPerSecond == 0 && store.values.lastLevel == 3, "Valid audio/dialogue/session config failed");
    store.values.fullscreen = true;
    store.values.unlimitedFps = true;
    store.values.targetFps = maxTargetFps;
    const auto saved = directory.path / "preferences/settings.yaml";
    Check(store.Save(saved), "Settings save failed");
    ConfigStore reloaded;
    Check(reloaded.Load(saved) && reloaded.values.windowWidth == 1280 && reloaded.values.moveSpeed == 310 && reloaded.values.lastLevel == 3, "Settings round trip failed");
    Check(reloaded.values.fullscreen, "Fullscreen preference round trip failed");
    Check(reloaded.values.unlimitedFps && reloaded.values.EffectiveTargetFps() == 0 && reloaded.values.targetFps == maxTargetFps, "Unlimited/cap preferences round trip failed");
    {
        std::ofstream output(file);
        output << "window:\n  width: 900\n  width: 1000\n  height: 99999\n\tbad: true\n  width: 1100\n"
               << "audio:\n  master_volume: nan\nunknown:\n  move_speed: 600\n";
    }
    Check(!reloaded.Load(file) && reloaded.values.windowWidth == 900 && reloaded.values.windowHeight == 99999 &&
          reloaded.values.moveSpeed == 310 && Near(reloaded.values.masterVolume, 0.7f, 0.001f),
          "Invalid or duplicate settings replaced valid values");
    // Old mode preferences remain readable but are discarded on save.
    for (const auto* mode : {"hybrid", "native", "performance"}) {
        { std::ofstream output(file); output << "window:\n  rendering_mode: " << mode << "\n  target_fps: 120\n"; }
        Check(reloaded.LoadFile(file, ConfigFile::Settings) && reloaded.values.targetFps == 120,
              "Legacy rendering mode prevented settings migration");
    }
    Check(reloaded.SaveFile(saved, ConfigFile::Settings), "Migrated settings save failed");
    { std::ifstream input(saved); const std::string contents((std::istreambuf_iterator<char>(input)), {});
      Check(contents.find("rendering_mode") == std::string::npos, "Obsolete rendering mode persisted"); }
    std::cout << "PASS: defaults, valid/malformed YAML subset and preferences round trip\n";
}

    void TestPlayerHealth()
{
    GameConfig config;
    const Rectangle world = {0, 0, 2000, 700};
    const std::vector<Rectangle> platforms = {{0, 650, 2000, 50}};
    InputFrame right;
    right.move = 1;
    Player player;
    player.Reset({100, 606});
    Check(player.GetHealth() == player.GetMaxHealth() && !player.IsDead(), "Player did not start at full health");
    player.TakeDamage(1);
    Check(player.GetHealth() == 0 && player.IsDead(), "TakeDamage did not reduce health");
    player.TakeDamage(5);
    Check(player.GetHealth() == 0, "Health dropped below zero");
    const float deadX = player.Position().x;
    for (int i = 0; i < 30; ++i) player.Update(1.0f / 60, right, config, platforms, world);
    Check(Near(player.Position().x, deadX), "Dead player moved");
    player.Reset({100, 606});
    Check(!player.IsDead() && player.GetHealth() == player.GetMaxHealth(), "Reset did not restore health");
    player.Kill();
    Check(player.IsDead(), "Kill did not set the dead state");
    std::cout << "PASS: health, damage, death and reset\n";
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
    Check(player.Position().x > 100 + config.moveSpeed * 0.9f && player.Position().x < 100 + config.moveSpeed
          && Near(player.Velocity().x, config.moveSpeed), "Movement does not accelerate toward moveSpeed");
    Player slowFrames;
    slowFrames.Reset({100, 606});
    for (int i = 0; i < 30; ++i) slowFrames.Update(1.0f / 30, right, config, platforms, world);
    Check(Near(player.Position().x, slowFrames.Position().x, 0.15f), "Horizontal movement depends on frame rate");
    InputFrame jump;
    jump.jump = true;
    player.Update(1.0f / 60, jump, config, platforms, world);
    Check(player.Position().y < 606 && !player.Grounded(), "Jump did not leave the floor");
    for (int i = 0; i < 120; ++i) player.Update(1.0f / 60, {}, config, platforms, world);
    Check(player.Grounded() && Near(player.Position().y, 606) && player.Velocity().y == 0, "Landing did not settle the player");
    player.Reset({1955, 606});
    for (int i = 0; i < 60; ++i) player.Update(1.0f / 60, right, config, platforms, world);
    Check(Near(player.Position().x, 1970), "World boundary clamp failed");
    player.Reset({100, 606});
    const std::vector<Rectangle> wall = {{0, 650, 2000, 50}, {150, 550, 30, 100}};
    for (int i = 0; i < 60; ++i) player.Update(1.0f / 60, right, config, wall, world);
    Check(Near(player.Position().x, 120), "Side collision failed");
    Check(player.Velocity().x == 0, "Wall contact did not stop horizontal speed");
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

void TestClimbingAndStamina()
{
    GameConfig config;
    const Rectangle world = {0, 0, 2000, 700};
    const std::vector<Rectangle> floor = {{0, 650, 2000, 50}};
    Check(config.maxStamina == 100.0f, "Default max stamina changed");
    Player player;
    player.Reset({100, 606});
    Check(player.StaminaPercent() == 1.0f && player.StaminaPercent(config.maxStamina) == 1.0f,
          "Player did not start at full stamina");
    // Climb a wall: the player starts flush against the left side of
    // a pillar and holds W/UP. Stamina drains while climbing.
    const std::vector<Rectangle> wall = {{0, 650, 2000, 50}, {150, 550, 30, 100}};
    player.Reset({120, 606});
    InputFrame climb;
    climb.climb = true;
    for (int i = 0; i < 30; ++i) player.Update(1.0f / 60, climb, config, wall, world);
    Check(player.Climbing() || player.Position().y < 606, "Climbing did not move the player up the wall");
    Check(player.StaminaPercent() < 1.0f, "Climbing did not drain stamina");
    Check(player.Position().x == 120.0f, "Climbing moved the player sideways");
    // Let go, land, and stand still: stamina regenerates to full.
    for (int i = 0; i < 180; ++i) player.Update(1.0f / 60, {}, config, floor, world);
    Check(player.Grounded() && player.StaminaPercent() == 1.0f, "Stamina did not regenerate on the ground");
    // Reset restores stamina.
    player.Reset({120, 606});
    Check(player.StaminaPercent() == 1.0f && !player.Climbing(), "Reset did not restore stamina");
    std::cout << "PASS: wall climbing, stamina drain, ground regen and reset\n";
}

void TestNativePresentation(Resources& resources, const Game& game, Rectangle destination)
{
    // Compare the native framebuffer to a test-only logical reference render,
    // sampling solid regions independently of differences in glyph rasterization.
    Image screen = LoadImageFromScreen();
    const test::TestRenderTarget reference;
    BeginTextureMode(reference.target);
    ClearBackground(BLACK);
    game.DrawFrame(resources, {0, 0, canvasWidth, canvasHeight});
    EndTextureMode();
    Image canvas = LoadImageFromTexture(reference.target.texture);
    ImageFlipVertical(&canvas);
    const auto dpi = WindowPixelScale();
    Check(canvas.width == canvasWidth && canvas.height == canvasHeight,
          "Logical reference target dimensions changed");
    // Sample solid regions of the real menu, away from filtered text and shape edges.
    for (const auto point : {Vector2{20, 20}, Vector2{700, 700}, Vector2{730, 730}, Vector2{1500, 800}})
    {
        const Color expected = GetImageColor(canvas, static_cast<int>(point.x), static_cast<int>(point.y));
        const Color actual = GetImageColor(screen,
            static_cast<int>((destination.x + point.x * destination.width / canvasWidth) * dpi.x),
            static_cast<int>((destination.y + point.y * destination.height / canvasHeight) * dpi.y));
        Check(std::abs(expected.r - actual.r) <= 2 && std::abs(expected.g - actual.g) <= 2 &&
              std::abs(expected.b - actual.b) <= 2, "Native frame is misplaced or discolored");
    }
    if (destination.x * dpi.x >= 1)
        Check(ColorIsEqual(GetImageColor(screen, 0, screen.height / 2), BLACK), "Side letterbox is not black");
    if (destination.y * dpi.y >= 1)
        Check(ColorIsEqual(GetImageColor(screen, screen.width / 2, 0), BLACK), "Top letterbox is not black");
    UnloadImage(canvas);
    UnloadImage(screen);
}

void TestNativeFonts(Resources& resources)
{
    for (const auto role : {FontRole::Regular, FontRole::Heading, FontRole::Typewriter})
        if (!resources.HasCustomFont(role)) return; // Missing optional assets are tested separately.
    constexpr std::string_view sample = "PIXEL FONT 123\nDialogue should wrap consistently.";
    constexpr std::string_view paragraph = "A long dialogue sentence should keep exactly the same words on every line at every window size.";
    const auto baseline = resources.MeasureText(sample, 32, 1, FontRole::Regular);
    const auto referenceLines = WrapText(paragraph, 450, [&](std::string_view line) {
        return resources.MeasureText(line, 26, 1, FontRole::Typewriter).x;
    });
    for (const int width : {640, 960, 1280, 1600, 1920, 2560, 3840}) {
        const int height = width * 9 / 16;
        const float scale = static_cast<float>(width) / canvasWidth;
        RenderTexture2D target = LoadRenderTexture(width, height);
        Check(target.id != 0, "Could not create native font test target");
        resources.BeginFrame();
        resources.SetViewport({0, 0, static_cast<float>(width), static_cast<float>(height)});
        const auto measured = resources.MeasureText(sample, 32, 1, FontRole::Regular);
        Check(Near(measured.x, baseline.x, 0.001f) && Near(measured.y, baseline.y, 0.001f),
              "Logical text measurement depends on framebuffer size");
        const auto lines = WrapText(paragraph, 450, [&](std::string_view line) {
            return resources.MeasureText(line, 26, 1, FontRole::Typewriter).x;
        });
        Check(lines.size() == referenceLines.size(), "Dialogue line count changed on resize");
        for (std::size_t i = 0; i < lines.size(); ++i)
            Check(lines[i].begin == referenceLines[i].begin && lines[i].end == referenceLines[i].end,
                  "Dialogue wrapping changed on resize");
        BeginTextureMode(target);
        ClearBackground(BLACK);
        {
            const ScopedModelview viewport(ViewportMatrix({0, 0, static_cast<float>(width), static_cast<float>(height)}));
            for (const auto role : {FontRole::Regular, FontRole::Heading, FontRole::Typewriter}) {
                const auto y = 50.0f + 80.0f * static_cast<int>(role);
                const auto dimensions = resources.MeasureText("PIXEL FONT 123", 32, 1, role);
                Check(dimensions.x > 0 && dimensions.y > 0, "Native font measurement failed");
                resources.DrawText("PIXEL FONT 123", {50.3f, y + 0.6f}, 32, 1, WHITE, role);
            }
            const auto builds = resources.FontBuildCount();
            resources.DrawText("CACHE REUSE", {50, 330}, 32, 1, WHITE, FontRole::Regular);
            Check(resources.FontBuildCount() == builds, "Repeated draw rebuilt a cached font atlas");
            // Clipping must also use physical coordinates after the UI transform.
            resources.BeginClip({50, 430, 80, 28});
            DrawRectangle(0, 400, 300, 100, RED);
            EndScissorMode();
            // A world label under a camera zoom must use a separate native-size atlas.
            Camera2D camera{}; camera.zoom = 2; camera.offset = {0, 520};
            {
                const ScopedModelview world(CameraModelview(camera, rlGetMatrixModelview()));
                resources.DrawText("WORLD", {25, 10}, 32, 1, WHITE);
                const auto count = resources.FontBuildCount();
                resources.DrawText("WORLD", {25, 50}, 32, 1, WHITE);
                Check(resources.FontBuildCount() == count, "World label rebuilt the atlas");
            }
        }
        EndTextureMode();
        Image image = LoadImageFromTexture(target.texture);
        ImageFlipVertical(&image);
        for (int role = 0; role < 3; ++role) {
            int solid = 0;
            for (int y = static_cast<int>((40 + role * 80) * scale);
                 y < static_cast<int>((105 + role * 80) * scale); ++y)
                for (int x = 0; x < static_cast<int>(900 * scale); ++x) {
                    const Color color = GetImageColor(image, x, y);
                    Check(color.r == 0 || color.r == 255, "Native bitmap glyph has filtered edge pixels");
                    if (color.r == 255) ++solid;
                }
            Check(solid > 20, "Native font did not produce visible glyphs");
        }
        Check(ColorIsEqual(GetImageColor(image, static_cast<int>(75 * scale), static_cast<int>(445 * scale)), RED) &&
              ColorIsEqual(GetImageColor(image, static_cast<int>(40 * scale), static_cast<int>(445 * scale)), BLACK),
              "Native UI clip did not match the physical destination");
        int worldPixels = 0;
        for (int y = static_cast<int>(520 * scale); y < static_cast<int>(700 * scale); ++y)
            for (int x = 0; x < static_cast<int>(500 * scale); ++x) {
                const auto color = GetImageColor(image, x, y);
                Check(color.r == 0 || color.r == 255, "World glyph has filtered edges");
                if (color.r == 255) ++worldPixels;
            }
        Check(worldPixels > 20, "World font did not render");
        UnloadImage(image);
        UnloadRenderTexture(target);
        resources.BeginFrame();
    }
    std::cout << "PASS: all three bitmap font roles, native pixel edges and atlas reuse at multiple resolutions\n";
}

// Verify actual centered glyph pixels, not just the font advance box, and
// the menu background fallback / optional main artwork composition.
void TestMenuRendering(Resources& resources)
{
    if (resources.HasCustomFont(FontRole::Typewriter)) {
        constexpr std::array<Rectangle, 4> boxes{{{100,100,30,34}, {180,100,67,34}, {300,100,58,34}, {420,100,49,34}}};
        constexpr std::array<std::string_view, 4> labels{{"A", "ENTER", "DOWN", "F11"}};
        for (const int width : {640, 960, 1366, 1600, 1920, 2560, 3840}) {
            const int height = width * 9 / 16;
            const Rectangle dest = CanvasDestination(width, height);
            const float scale = dest.width / canvasWidth;
            const test::TestRenderTarget target(width, height);
            resources.BeginFrame();
            resources.SetViewport(dest);
            BeginTextureMode(target.target);
            ClearBackground(BLACK);
            {
                const ScopedModelview viewport(ViewportMatrix(dest));
                for (std::size_t k = 0; k < boxes.size(); ++k)
                    resources.DrawCenteredText(labels[k], boxes[k], 20, 1, WHITE, FontRole::Typewriter);
            }
            EndTextureMode();
            Image image = LoadImageFromTexture(target.target.texture);
            ImageFlipVertical(&image);
            for (const auto box : boxes) {
                const float cx = dest.x + (box.x + box.width / 2) * scale;
                const float cy = dest.y + (box.y + box.height / 2) * scale;
                int left = width, right = -1, top = height, bottom = -1;
                for (int y = static_cast<int>(dest.y + (box.y-5)*scale); y < dest.y + (box.y+box.height+5)*scale; ++y)
                    for (int x = static_cast<int>(dest.x + (box.x-5)*scale); x < dest.x + (box.x+box.width+5)*scale; ++x) {
                        const auto pixel = GetImageColor(image, x, y);
                        Check(pixel.r == 0 || pixel.r == 255, "Centered key glyph has filtered edges");
                        if (pixel.r != 255) continue;
                        left = std::min(left,x); right = std::max(right,x);
                        top = std::min(top,y); bottom = std::max(bottom,y);
                    }
                Check(right >= left && bottom >= top, "Centered key glyph was not drawn");
                Check(std::abs((left+right+1)*0.5f-cx) <= 2 && std::abs((top+bottom+1)*0.5f-cy) <= 2,
                      "Visible key glyph is not centered in its logical bounds");
            }
            UnloadImage(image);
        }
    }
    TestDirectory fixture;
    const auto render = [&](Resources& res, ArtId menu, bool mainUi = false) {
        const test::TestRenderTarget target;
        res.BeginFrame();
        res.SetViewport({0,0,canvasWidth,canvasHeight});
        BeginTextureMode(target.target);
        ClearBackground(RED);
        if (mainUi) {
            GameConfig config;
            config.showFps = false;
            Game game(config);
            game.DrawFrame(res, {0,0,canvasWidth,canvasHeight});
        } else art::MenuBackdrop(res, menu);
        EndTextureMode();
        Image image = LoadImageFromTexture(target.target.texture);
        ImageFlipVertical(&image);
        return image;
    };
    {
        Resources missing(fixture.path);
        for (const auto menu : {ArtId::MainMenu, ArtId::Settings, ArtId::StageSelect, ArtId::Pause}) {
            Image image = render(missing,menu);
            for (const auto point : {Vector2{5,5}, Vector2{800,450}, Vector2{1350,160}, Vector2{1590,890}})
                Check(ColorIsEqual(GetImageColor(image,point.x,point.y),theme::background), "Default menu backdrop is not solid");
            UnloadImage(image);
        }
        Image main = render(missing,ArtId::MainMenu,true);
        for (int y = 0; y < canvasHeight; y += 7) for (int x = 680; x < canvasWidth; x += 7)
            Check(ColorIsEqual(GetImageColor(main,x,y),theme::background), "Main menu right side is not clear for artwork");
        UnloadImage(main);
    }
    const auto path = fixture.path / artPaths[static_cast<std::size_t>(ArtId::MainMenu)];
    std::filesystem::create_directories(path.parent_path());
    const Color marker{25,112,173,255};
    Image art = GenImageColor(16,9,marker);
    const bool exported = ExportImage(art,path.string().c_str());
    UnloadImage(art);
    Check(exported,"Could not export main-menu background fixture");
    {
        Resources withArt(fixture.path);
        Image main = render(withArt,ArtId::MainMenu,true);
        Check(ColorIsEqual(GetImageColor(main,1200,450),marker) && ColorIsEqual(GetImageColor(main,20,20),marker),
              "Optional main-menu artwork does not cover the canvas");
        Check(!ColorIsEqual(GetImageColor(main,100,450),marker), "Main-menu buttons are behind the artwork");
        UnloadImage(main);
    }
    std::cout << "PASS: centered native key glyphs, solid menu defaults and optional main-menu art\n";
}

// Render the same game state at many framebuffer sizes, including non-16:9.
// Compare only interior reference pixels (not texture or geometry edges), where
// fractional sampling is expected to vary by at most a physical pixel.
void TestNativeScale(Resources& resources, const Game& game, GameConfig& config)
{
    const bool showFps = config.showFps;
    config.showFps = false;
    auto render = [&](int width, int height) {
        const test::TestRenderTarget target(width, height);
        BeginTextureMode(target.target);
        ClearBackground(BLACK);
        game.DrawFrame(resources, CanvasDestination(width, height));
        EndTextureMode();
        Image result = LoadImageFromTexture(target.target.texture);
        ImageFlipVertical(&result);
        return result;
    };
    const Image reference = render(canvasWidth, canvasHeight);
    const auto same = [](Color a, Color b) {
        return std::abs(a.r - b.r) <= 2 && std::abs(a.g - b.g) <= 2 && std::abs(a.b - b.b) <= 2;
    };
    for (const auto size : {Vector2{640,360}, Vector2{960,600}, Vector2{1280,720}, Vector2{1366,768},
                             Vector2{1920,1080}, Vector2{1920,1200}, Vector2{2560,1080}, Vector2{2560,1440},
                             Vector2{3840,2160}, Vector2{900,1600}}) {
        const int width = static_cast<int>(size.x), height = static_cast<int>(size.y);
        const Rectangle dest = CanvasDestination(width, height);
        const Image image = render(width, height);
        if (dest.y >= 1) Check(ColorIsEqual(GetImageColor(image, width/2, 0), BLACK) &&
            ColorIsEqual(GetImageColor(image, width/2, height-1), BLACK), "Native rendering broke vertical letterbox");
        if (dest.x >= 1) Check(ColorIsEqual(GetImageColor(image, 0, height/2), BLACK) &&
            ColorIsEqual(GetImageColor(image, width-1, height/2), BLACK), "Native rendering broke horizontal letterbox");
        int total = 0, matching = 0;
        for (int y = 8; y < canvasHeight-8; y += 13) for (int x = 8; x < canvasWidth-8; x += 13) {
            const Color expected = GetImageColor(reference, x, y);
            bool interior = true;
            for (int dy : {-3, 0, 3}) for (int dx : {-3, 0, 3})
                interior &= same(expected, GetImageColor(reference, x+dx, y+dy));
            if (!interior) continue;
            const int px = static_cast<int>(dest.x + (x+0.5f)*dest.width/canvasWidth);
            const int py = static_cast<int>(dest.y + (y+0.5f)*dest.height/canvasHeight);
            ++total;
            if (same(expected, GetImageColor(image, px, py))) ++matching;
        }
        if (!(total > 100 && matching >= total * 0.96f)) {
            std::cerr << "Scale " << width << "x" << height << ": " << matching << "/" << total << " interior samples match\n";
            Check(false, "Native frame geometry changed with framebuffer size");
        }
        UnloadImage(image);
    }
    UnloadImage(reference);
    config.showFps = showFps;
    std::cout << "PASS: native frame geometry and letterboxing across ten framebuffer sizes\n";
}

void TestSession()
{
    GameConfig config;
    Game game(config);
    game.Update(0, {.down = true});
    game.Update(0, {.down = true});
    game.Update(0, {.confirm = true});
    Check(game.Screen() == ScreenId::LevelSelect, "Keyboard menu selection failed");
    const auto& definition = Level1Definition();
    game.Update(0, ClickAction(game, ActionKind::StartLevel, static_cast<int>(definition.id)));
    const auto* level = game.ActiveLevel();
    Check(game.Screen() == ScreenId::Playing && level && level->Definition().id == definition.id, "Stage selection failed");
    game.Update(0.1f, {.move = 1});
    const auto position = level->GetPlayer().Position();
    const auto bytes = level->Dialogue().VisibleBytes();
    const auto elapsed = level->ElapsedSeconds();
    game.Update(0.1f, {.move = 1, .pause = true});
    game.Update(0.1f, {.move = 1});
    Check(game.Screen() == ScreenId::Paused && game.ActiveLevel() == level && level->GetPlayer().Position().x == position.x &&
          level->Dialogue().VisibleBytes() == bytes && level->ElapsedSeconds() == elapsed, "Pause advanced the level");

    game.Update(0, ClickAction(game, ActionKind::OpenSettings));
    game.TakeSettingsDirty(); // Stage selection also changes a setting.
    game.Update(0.1f, ClickAction(game, ActionKind::ToggleFps));
    Check(game.Screen() == ScreenId::Settings && config.showFps && game.TakeSettingsDirty(), "Settings mutation failed");
#if !defined(PLATFORM_WEB)
    game.Update(0.1f, ClickAction(game, ActionKind::ToggleFullscreen));
    Check(config.fullscreen && game.TakeSettingsDirty(), "Paused settings fullscreen toggle failed");
    game.Update(0.1f, {.toggleFullscreen = true});
    Check(!config.fullscreen && game.TakeSettingsDirty(), "Paused settings fullscreen shortcut failed");
#endif
    Check(game.ActiveLevel() == level && level->ElapsedSeconds() == elapsed, "Settings advanced or replaced the paused level");
    game.Update(0, ClickAction(game, ActionKind::Back));
    Check(game.Screen() == ScreenId::Paused, "Settings did not return to pause");
    game.Update(0, ClickAction(game, ActionKind::Restart));
    level = game.ActiveLevel();
    Check(game.Screen() == ScreenId::Playing && level && level->GetPlayer().Position().x == definition.spawn.x &&
          level->Dialogue().VisibleBytes() == 0 && level->ElapsedSeconds() == 0, "Restart failed to reset the stage");

    game.Update(0, {.pause = true});
    game.Update(0, ClickAction(game, ActionKind::OpenLevels));
    Check(!game.ActiveLevel(), "Leaving a level retained the scene");
    game.Update(0, {.back = true});
    Check(game.Screen() == ScreenId::MainMenu, "Selection back navigation failed");
    game.Update(0, ClickAction(game, ActionKind::Quit));
    Check(game.WantsQuit(), "Quit action failed");
    std::cout << "PASS: keyboard/mouse menus, pause, settings, restart and quit\n";
}
}

int RunSelfTests()
{
    int failures = 0;
    // Report independent groups even when one fails, so an unrelated regression
    // cannot hide the results of the remaining gameplay and asset checks.
    const auto run = [&](const char* name, void (*test)()) {
        try {
            test();
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "FAIL: " << name << ": " << error.what() << '\n';
        }
    };
    run("dialogue", TestDialogue);
    run("config", TestConfig);
    run("movement", TestMovement);
    run("climbing and stamina", TestClimbingAndStamina);
    run("player movement", TestPlayerMovement);
    run("player tuning", TestPlayerTuning);
    run("crouch jumps", TestCrouchJumps);
    run("player health", TestPlayerHealth);
    run("sliders", TestSliders);
    run("session", TestSession);
    run("level authoring", TestLevelAuthoring);
    run("events", TestEventManager);
    run("Game Data", TestGameData);
    run("audio data", TestAudioData);
    run("objects", TestObjects);
    run("collisions", TestCollisions);
    run("birds", TestBirds);
    if (failures == 0) {
        std::cout << "All foundation self-tests passed (no window/audio context required).\n";
    } else {
        std::cerr << failures << " self-test group(s) failed.\n";
    }
    return failures == 0 ? 0 : 1;
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
    for (std::size_t i = 0; i < fontPaths.size(); ++i)
        if (FileExists((FindResourceDirectory() / fontPaths[i]).string().c_str()))
            Check(resources.HasCustomFont(static_cast<FontRole>(i)), "A configured font exists but did not load");
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
        const test::TestRenderTarget fixtureTarget;
        BeginTextureMode(fixtureTarget.target);
        ClearBackground(BLACK);
        const bool drewSprite = fixtureResources.DrawArt(ArtId::Student, {0,0,80,80});
        const bool drewMissing = fixtureResources.DrawArt(ArtId::Entity, {100,0,80,80});
        EndTextureMode();
        Check(drewSprite && !drewMissing, "Optional artwork selection/fallback failed");
        Image rendered = LoadImageFromTexture(fixtureTarget.target.texture);
        ImageFlipVertical(&rendered);
        const Color center = GetImageColor(rendered, 40, 40);
        const Color margin = GetImageColor(rendered, 5, 40);
        UnloadImage(rendered);
        Check(center.r == marker.r && center.g == marker.g && center.b == marker.b && margin.r == 0,
              "Artwork did not render or preserve its aspect ratio");
    }
    std::cout << "PASS: custom font, missing-font fallback and optional texture replacement\n";
    TestNativeFonts(resources);
    TestMenuRendering(resources);
    TestAssetRendering();
    TestMappedLevelSpriteRendering();
    TestBirdRendering();
    TestJumpParticleRendering();
    TestAudioPlayback();
    auto click = [&](ActionKind kind, int value = 0) { Frame(0, ClickAction(game, kind, value)); };
    auto capture = [&](const std::string& name, bool /*legacyScreenFlag*/ = true)
    {
        if (captureDirectory.empty()) return;
        std::filesystem::create_directories(captureDirectory);
        // The entire frame is drawn directly to the framebuffer. Draw the same state
        // twice before readback so both swap buffers contain the final composition.
        Draw();
        Draw();
        Image image = LoadImageFromScreen();
        const auto path = captureDirectory / (name + ".png");
        const bool saved = ExportImage(image, path.string().c_str());
        UnloadImage(image);
        Check(saved, "Could not capture a rendered smoke-test frame");
    };
    Frame(0, {});
    const auto stableBuilds = resources.FontBuildCount();
    Frame(0, {});
    Check(resources.FontBuildCount() == stableBuilds, "Unchanged menu rebuilt font resources");
    TestNativeScale(resources, game, config.values);
    capture("main_menu");
    click(ActionKind::OpenLevels);
    capture("level_select");
    for (const auto& level : LevelCatalog())
    {
        click(ActionKind::StartLevel, static_cast<int>(level.id));
        Check(game.Screen() == ScreenId::Playing && game.ActiveLevel() && game.ActiveLevel()->Definition().id == level.id,
              "Smoke test could not open a level for rendering");
        Frame(0, {.completeDialogue = true});
        capture("stage_" + std::to_string(static_cast<int>(level.id)));
        TestNativeScale(resources, game, config.values);
        if (level.id == LevelId::Level1) {
            Frame(1.0f / 60, {.duck = true});
            Check(game.ActiveLevel()->GetPlayer().Crouching(), "Rendered crouch lost its shared player state");
            capture("stage_1_crouch");
            Frame(1.0f / 60, {.jump = true, .duck = true});
            Check(!game.ActiveLevel()->GetPlayer().Crouching() && game.ActiveLevel()->PlayerAnimator().State() == "jump",
                  "Crouch jump kept the airborne crouch animation");
            capture("stage_1_crouch_jump");
            Frame(0.06f, {.duck = true});
            capture("stage_1_air_particles");
            // Capture the timer's entrance, travel and settled HUD in real gameplay.
            for (int frame = 0; frame < 60; ++frame) Frame(1.0f / 60, {});
            capture("stage_1_timer_entrance");
            for (int frame = 0; frame < 26; ++frame) Frame(1.0f / 60, {});
            capture("stage_1_timer_travel");
            for (int frame = 0; frame < 30; ++frame) Frame(1.0f / 60, {});
            capture("stage_1_timer_docked");
        }
        Frame(0, {.pause = true});
        TestNativeScale(resources, game, config.values);
        capture("pause");
        click(ActionKind::OpenLevels);
    }
    click(ActionKind::MainMenu);
#if defined(RAGE_DESKTOP_GLFW)
    MaximizeWindow();
    PollInputEvents();
    Frame(0, {});
    Frame(0, {}); // Draw both swap buffers before reading screen pixels.
    Check(IsWindowMaximized(), "The window did not maximize");
    capture("maximized", true);
    TestNativePresentation(resources, game, Destination());
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
    TestNativeScale(resources, game, config.values);
    capture("settings");
    if (!captureDirectory.empty())
    {
        Frame(0, {}); // Draw both swap buffers before reading screen pixels.
        capture("resized_options", true);
    }
    click(ActionKind::Back);
    config.values.showFps = false;
    for (const auto size : {Vector2{960, 540}, Vector2{1280, 720}, Vector2{1366, 768}, Vector2{1600, 900}, Vector2{1920, 1080}})
    {
        SetWindowSize(static_cast<int>(size.x), static_cast<int>(size.y));
        Frame(0, {});
        Frame(0, {});
        TestNativePresentation(resources, game, Destination());
        if (captureDirectory.empty()) continue;
        const auto suffix = std::to_string(static_cast<int>(size.x)) + "x" + std::to_string(static_cast<int>(size.y));
        capture("main_" + suffix);
        click(ActionKind::OpenSettings);
        capture("settings_" + suffix);
        click(ActionKind::Back);
        click(ActionKind::StartLevel);
        Frame(0, {.completeDialogue = true});
        capture("level_" + suffix);
        Frame(0, {.pause = true});
        click(ActionKind::MainMenu);
    }
#if defined(RAGE_DESKTOP_GLFW)
    // Exclusive fullscreen invalidates hidden-window readback on some drivers,
    // so check the mode and viewport after completing the windowed pixel checks.
    SetWindowSize(960, 540);
    PollInputEvents();
    click(ActionKind::OpenSettings);
    click(ActionKind::ToggleFullscreen);
    PollInputEvents();
    Check(IsWindowFullscreen() && config.values.fullscreen && game.Screen() == ScreenId::Settings,
          "Settings checkbox did not enter fullscreen");
    const auto fullscreen = Destination();
    Check(Near(fullscreen.width / fullscreen.height, 16.0f / 9) &&
          fullscreen.x >= -0.01f && fullscreen.y >= -0.01f &&
          fullscreen.x + fullscreen.width <= GetRenderWidth() + 0.01f &&
          fullscreen.y + fullscreen.height <= GetRenderHeight() + 0.01f &&
          (Near(fullscreen.width, GetRenderWidth()) || Near(fullscreen.height, GetRenderHeight())),
          "Fullscreen did not fit the 16:9 canvas within the display");
    Frame(0, {.toggleFullscreen = true});
    PollInputEvents();
    Check(!IsWindowFullscreen() && !config.values.fullscreen && game.Screen() == ScreenId::Settings,
          "Fullscreen shortcut did not restore the window");
    Check(Near(GetScreenWidth(), 960, 2) && Near(GetScreenHeight(), 540, 2), "Windowed size was not restored");
    click(ActionKind::Back);
#endif
    std::cout << "PASS: raylib rendering for menus and all stages, single native world and UI, resizing and fullscreen\n";
    return 0;
}
}
