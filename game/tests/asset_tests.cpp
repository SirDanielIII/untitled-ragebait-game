#include "engine/assets/animator.h"
#include "engine/assets/game_data.h"
#include "engine/assets/render_queue.h"
#include "engine/resources/resources.h"
#include "test_support.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace rage
{
namespace
{
using test::Check;
using test::TestDirectory;

const AnimationData& RequireAnimation(const GameData& data, const char* id)
{
    if (const auto* animation = data.Find(id)) return *animation;
    throw std::runtime_error(std::string("Missing animation: ") + id);
}

bool HasProblem(const GameData& data, const std::string& fragment)
{
    for (const auto& problem : data.Problems())
        if (problem.find(fragment) != std::string::npos) return true;
    return false;
}

const char* sampleCsv =
    "\xEF\xBB\xBF" "Animation ID,Asset ID,File Name,Frame Width (px),Frame Height (px),Frames,Frames per Row,bottom-centre,Loops,Pivot,Faces,Layer,Notes\r\n"
    "player_idle,player_character,player_idle.png,48,80,4,,8,Yes,bottom-centre,right,Middle,\"Breathing, slowly\"\r\n"
    "player_walk,player_character,player_walk.png,48,80,6,,12,Yes,Bottom-Centre,Right,middle,\r\n"
    "player_jump,player_character,player_jump.png,48,80,3,,10,No,bottom-centre,right,Middle,\"Two\nlines\"\r\n"
    "door_exit,door_exit,door_exit.png,16,32,1,,,,bottom-centre,,Background,\r\n"
    "tiles_street,tiles_street,tiles_street.png,16,16,8,4,,,top-left,,Background,\r\n"
    "coin_spin,coin,,12,12,6,,10,Yes,centre,,Foreground,no art yet\r\n"
    "trigger_exit,trigger,none,64,64,1,,,,top-left,,Middle,\r\n"
    ",,,,,,,,,,,,\r\n";

void TestImport()
{
    GameData data;
    data.Parse(sampleCsv, "sample");
    Check(data.Size() == 7, "Game Data did not import every valid row");
    Check(data.Problems().size() == 1 && HasProblem(data, "column H"), "Renamed FPS column was not read by position");

    const auto* walk = data.Find("player_walk");
    Check(walk && walk->assetId == "player_character" && walk->fileName == "player_walk.png", "Animation lookup failed");
    Check(walk->frameWidth == 48 && walk->frameHeight == 80 && walk->frames == 6 && walk->fps == 12, "Animation numbers were not parsed");
    Check(walk->loops && walk->pivot == Pivot::BottomCentre && walk->faces == Facing::Right && walk->layer == Layer::Middle,
          "Animation options were not parsed");
    Check(data.Find("player_character", "walk") == walk, "Asset ID + state lookup failed");
    Check(data.Find("player_character", "run") == nullptr && data.Find("nobody", "walk") == nullptr, "Unknown state should be absent");
    Check(RequireAnimation(data, "player_idle").notes == "Breathing, slowly", "Quoted CSV cell with a comma failed");
    const auto& jump = RequireAnimation(data, "player_jump");
    Check(jump.notes == "Two\nlines" && !jump.loops, "Quoted newline or Loops=No failed");
    const auto player = data.ForAsset("player_character");
    Check(player.size() == 3 && player[0]->animationId == "player_idle" && player[2]->animationId == "player_jump",
          "Animations were not grouped by Asset ID in sheet order");

    const auto* door = data.Find("door_exit", "");
    Check(door && door->frames == 1 && door->fps == 0 && !door->loops && !door->IsAnimated(), "Static asset defaults failed");
    Check(door->layer == Layer::Background && door->faces == Facing::None, "Static asset options failed");

    const auto& coin = RequireAnimation(data, "coin_spin");
    Check(coin.fileName.empty() && !coin.invisible && coin.pivot == Pivot::Centre && coin.layer == Layer::Foreground,
          "Placeholder row without a file failed");
    Check(RequireAnimation(data, "trigger_exit").invisible, "File Name 'none' should mark an invisible object");
}

void TestValidation()
{
    GameData data;
    const bool clean = data.Parse(
        "Animation ID,Asset ID,File Name,Frame Width (px),Frame Height (px),Frames,Frames per Row,FPS,Loops,Pivot,Faces,Layer,Notes\n"
        "good_one,good,good.png,10,10,1,,,,top-left,,UI,\n"
        "fast_thing,thing,a.png,10,10,4,,fast,Yes,centre,,Middle,\n"
        "zero_frames,thing,b.png,10,10,0,,5,maybe,diagonal,up,Sky,\n"
        "bad_size,thing,c.png,abc,10,1,,,,top-left,,Middle,\n"
        "good_one,good,dup.png,10,10,1,,,,top-left,,UI,\n"
        ",thing,d.png,10,10,1,,,,top-left,,Middle,\n"
        "Bad-Name,thing,e.png,10,10,1,,,,top-left,,Middle,\n",
        "invalid");
    Check(!clean, "Invalid rows were not reported");
    Check(data.Size() == 4, "Valid rows were not kept beside invalid ones");
    const auto& good = RequireAnimation(data, "good_one");
    Check(good.fileName == "good.png" && good.layer == Layer::Ui, "Duplicate replaced the first row");
    Check(!data.Find("bad_size"), "Row with an invalid frame size was imported");
    const auto& fast = RequireAnimation(data, "fast_thing");
    Check(fast.fps == 0 && !fast.IsAnimated() && FrameAt(fast, 3) == 0, "Invalid FPS should show the first frame");
    const auto& zero = RequireAnimation(data, "zero_frames");
    Check(zero.frames == 1 && zero.pivot == Pivot::BottomCentre && zero.faces == Facing::None && zero.layer == Layer::Middle,
          "Invalid values did not fall back to defaults");
    Check(data.Find("Bad-Name") && HasProblem(data, "lowercase"), "Naming convention warning missing");
    for (const char* fragment : {"FPS", "Frames", "Loops", "Pivot", "Faces", "Layer", "duplicate", "Animation ID is blank", "Frame Width"})
        Check(HasProblem(data, fragment), "An invalid value was not reported");

    GameData missingColumn;
    Check(!missingColumn.Parse("Animation ID,Asset ID,Frames\nx,y,1\n") && missingColumn.Size() == 0,
          "Missing required columns should import nothing");
    GameData missingFile;
    Check(!missingFile.LoadFile("definitely/not/here/game_data.csv") && missingFile.Size() == 0 &&
          !missingFile.Problems().empty(), "Missing CSV should be reported, not thrown");
    GameData empty;
    Check(!empty.Parse("") && empty.Size() == 0, "Empty CSV should be reported");
}

void TestFramesAndPlayback()
{
    GameData data;
    data.Parse(sampleCsv, "sample");
    const auto& walk = RequireAnimation(data, "player_walk");
    const auto source = FrameSource(walk, 4);
    Check(source.x == 192 && source.y == 0 && source.width == 48 && source.height == 80, "Strip frame rectangle failed");
    const auto& tiles = RequireAnimation(data, "tiles_street");
    Check(tiles.Columns() == 4 && tiles.Rows() == 2, "Grid size failed");
    const auto tile = FrameSource(tiles, 5);
    Check(tile.x == 16 && tile.y == 16, "Grid frame rectangle failed");
    Check(FrameSource(tiles, 99).x == 48 && FrameSource(tiles, 99).y == 16, "Out-of-range frame was not clamped");

    Check(FrameAt(walk, 0) == 0 && FrameAt(walk, 0.25) == 3 && FrameAt(walk, 0.5) == 0 && FrameAt(walk, 0.55) == 0,
          "Looping playback timing failed");
    const auto& jump = RequireAnimation(data, "player_jump");
    Check(FrameAt(jump, 0.15) == 1 && !IsFinished(jump, 0.15), "One-shot playback timing failed");
    Check(FrameAt(jump, 10) == 2 && IsFinished(jump, 10), "One-shot animation did not hold its last frame");
    Check(FrameAt(RequireAnimation(data, "door_exit"), 5) == 0 && FrameAt(walk, -1) == 0 && FrameAt(walk, NAN) == 0, "Static/invalid time failed");

    Animator animator;
    animator.SetState("walk");
    animator.Update(0.2f);
    animator.SetState("walk");
    Check(std::abs(animator.Seconds() - 0.2) < 1e-6, "Re-selecting the current state restarted it");
    animator.Update(-1);
    animator.Update(NAN);
    Check(std::abs(animator.Seconds() - 0.2) < 1e-6, "Invalid frame time advanced the animation");
    animator.SetState("jump");
    Check(animator.Seconds() == 0 && animator.State() == "jump", "Changing state did not restart playback");

    float x = 0;
    float y = 0;
    PivotOffset(Pivot::BottomCentre, 48, 80, x, y);
    Check(x == 24 && y == 80, "Bottom-centre pivot failed");
    PivotOffset(Pivot::TopLeft, 48, 80, x, y);
    Check(x == 0 && y == 0, "Top-left pivot failed");
    PivotOffset(Pivot::Centre, 48, 80, x, y);
    Check(x == 24 && y == 40, "Centre pivot failed");
    AnimationData sprite;
    sprite.frameWidth = 16;
    sprite.frameHeight = 32;
    sprite.pivot = Pivot::BottomCentre;
    const Rectangle drawn = AssetManager::Bounds(sprite, {100, 650});
    Check(drawn.width == 16 * pixelScale && drawn.height == 32 * pixelScale, "Pixel scale was not applied");
    Check(drawn.x + drawn.width / 2 == 100 && drawn.y + drawn.height == 650, "Scaled sprite moved off its pivot");
    Check(FitScale(16, 32, 30, 44, 2) == 1, "Best fit should round down to whole screen pixels");
    Check(FitScale(16, 32, 64, 128, 2) == 4, "Best fit should fill an exact multiple");
    Check(FitScale(16, 32, 4, 4, 2) == 0.5f, "Best fit must keep at least one screen pixel per art pixel");
    Check(FitScale(32, 16, 30, 44, 2) == 0.5f, "Best fit must use the tighter side (wide art in a tall box)");

    // Physical viewport mapping must preserve the logical coordinates and offsets.
    const Rectangle destination = CanvasDestination(2560, 1600);
    const Vector2 mapped = Vector2Transform({800, 450}, ViewportMatrix(destination));
    Check(test::Near(mapped.x, 1280) && test::Near(mapped.y, 800), "Native viewport mapping failed");
    const Camera2D camera{{800, 330}, {100, 200}, 0, 2};
    const Vector2 focus = Vector2Transform(camera.target, CameraModelview(camera, ViewportMatrix(destination)));
    const Vector2 expected = Vector2Transform(camera.offset, ViewportMatrix(destination));
    Check(test::Near(focus.x, expected.x) && test::Near(focus.y, expected.y), "Native camera composition failed");
    Check(ShouldFlip(Facing::Right, Facing::Left) && !ShouldFlip(Facing::Right, Facing::Right) &&
          !ShouldFlip(Facing::None, Facing::Left), "Facing flip rule failed");

    std::vector<int> order;
    RenderQueue queue;
    queue.Submit(Layer::Ui, [&] { order.push_back(4); });
    queue.Submit(Layer::Foreground, [&] { order.push_back(3); });
    queue.Submit(Layer::Middle, [&] { order.push_back(2); });
    queue.Submit(Layer::Background, [&] { order.push_back(1); });
    queue.Submit(Layer::Middle, [&] { order.push_back(22); });
    queue.Flush();
    Check(order == std::vector<int>{1, 2, 22, 3, 4} && queue.Size() == 0, "Layer order failed");
    order.clear();
    queue.Submit(Layer::Foreground, [&] { order.push_back(3); });
    queue.Submit(Layer::Background, [&] { order.push_back(1); });
    queue.Flush();
    Check(order == std::vector<int>{1, 3} && queue.Size() == 0, "Reused queue replayed an old job or changed layer order");
}

void TestPlayerAnimationState()
{
    GameConfig config;
    auto level = CreateLevel(LevelId::Level1, config);
    Check(level->PlayerAnimator().State() == "idle" && level->PlayerFacing() == Facing::Right, "Player animation did not reset");
    level->Update(0.1f, {.move = -1}, config);
    Check(level->PlayerAnimator().State() == "walk" && level->PlayerFacing() == Facing::Left, "Walking left did not select walk/left");
    // Releasing movement should preserve facing while slowing down.
    level->Update(0.1f, {}, config);
    Check(level->PlayerFacing() == Facing::Left,
        "Releasing movement changed player facing");

    // Allow friction to bring the player to a complete stop.
    for (int i = 0; i < 4; ++i) {
        level->Update(0.1f, {}, config);
    }
    Check(level->PlayerAnimator().State() == "idle" &&
        level->PlayerFacing() == Facing::Left,
        "Stopping should idle and keep facing");
    level->Update(0.05f, {.jump = true}, config);
    Check(level->PlayerAnimator().State() == "jump", "Jumping did not select the jump state");
    level->Reset(config);
    level->Update(0.05f, {.duck = true}, config);
    Check(level->PlayerAnimator().State() == "crouch", "Crouching did not select its future sprite state");
    level->Update(0.05f, {.move = 1, .duck = true}, config);
    Check(level->PlayerAnimator().State() == "crouch_walk", "Crouch movement did not select its sprite state");
}

bool Same(Color a, Color b) { return a.r == b.r && a.g == b.g && a.b == b.b; }
}

void TestGameData()
{
    TestImport();
    TestValidation();
    TestFramesAndPlayback();
    TestPlayerAnimationState();
    std::cout << "PASS: Game Data import/validation, lookups, frame math, playback, pivots, flipping and layers\n";
}

void TestAssetRendering()
{
    TestDirectory fixture;
    std::filesystem::create_directories((fixture.path / gameDataPath).parent_path());
    std::filesystem::create_directories(fixture.path / staticArtFolder);
    std::filesystem::create_directories(fixture.path / animatedArtFolder);
    {
        std::ofstream csv(fixture.path / gameDataPath, std::ios::binary);
        csv << "Animation ID,Asset ID,File Name,Frame Width (px),Frame Height (px),Frames,Frames per Row,FPS,Loops,Pivot,Faces,Layer,Notes\r\n"
               "hero_walk,hero,hero.png,8,8,4,2,10,Yes,bottom-centre,right,Middle,\r\n"
               "hero_idle,hero,hero.png,8,8,1,,,,bottom-centre,right,Middle,shares the walk sheet\r\n"
               "sign,sign,sign.png,8,8,1,,,,top-left,,Background,\r\n"
               "ghost_float,ghost,ghost.png,8,8,2,,4,Yes,centre,,Foreground,art not made yet\r\n"
               "trigger_zone,trigger,none,8,8,1,,,,top-left,,Middle,\r\n";
    }
    const Color red{220, 30, 30, 255};
    const Color white{250, 250, 250, 255};
    const Color yellow{240, 220, 20, 255};
    const Color cyan{20, 210, 220, 255};

    Image hero = GenImageColor(16, 16, BLACK);
    ImageDrawRectangle(&hero, 0, 0, 4, 8, red);
    ImageDrawRectangle(&hero, 4, 0, 4, 8, white);
    ImageDrawRectangle(&hero, 8, 0, 8, 8, GREEN);
    ImageDrawRectangle(&hero, 0, 8, 8, 8, BLUE);
    ImageDrawRectangle(&hero, 8, 8, 8, 8, yellow);
    Image sign = GenImageColor(8, 8, cyan);
    const bool exported = ExportImage(hero, (fixture.path / animatedArtFolder / "hero.png").string().c_str()) &&
                          ExportImage(sign, (fixture.path / staticArtFolder / "sign.png").string().c_str());
    UnloadImage(hero);
    UnloadImage(sign);
    Check(exported, "Could not create Game Data PNG fixtures");

    Resources resources(fixture.path);
    const auto& assets = resources.Assets();
    Check(assets.Data().Size() == 5 && assets.Data().Problems().empty(), "Fixture Game Data did not load cleanly");
    Check(assets.TextureCount() == 2, "A shared PNG was loaded more than once");
    const auto* walk = assets.Find("hero", "walk");
    const auto* idle = assets.Find("hero_idle");
    const auto* signData = assets.Find("sign");
    const auto* ghost = assets.Find("ghost_float");
    const auto* trigger = assets.Find("trigger_zone");
    Check(walk && idle && signData && ghost && trigger, "Fixture animations are missing");
    Check(assets.HasTexture(*walk) && assets.HasTexture(*idle) && assets.HasTexture(*signData) && !assets.HasTexture(*ghost),
          "Texture availability is wrong");

    const test::TestRenderTarget target;
    BeginTextureMode(target.target);
    ClearBackground(BLACK);
    const float scale = 4;
    const bool drew = assets.Draw(*walk, 0, {100, 100}, Facing::Right, {}, scale) == DrawResult::Texture &&
                      assets.Draw(*walk, 0, {200, 100}, Facing::Left, {}, scale) == DrawResult::Texture &&
                      assets.Draw(*walk, 3, {300, 100}, Facing::Right, {}, scale) == DrawResult::Texture &&
                      assets.Draw(*signData, 0, {400, 60}, Facing::None, {}, scale) == DrawResult::Texture &&
                      assets.Draw(*ghost, 0, {500, 80}, Facing::None, {}, scale) == DrawResult::Placeholder &&
                      assets.Draw(*trigger, 0, {600, 60}, Facing::None, {}, scale) == DrawResult::Invisible;
    bool customPlaceholder = false;
    assets.Draw(*ghost, 0, {700, 80}, Facing::None, [&](Rectangle bounds, bool) {
        customPlaceholder = bounds.x == 684 && bounds.y == 64 && bounds.width == 32 && bounds.height == 32;
    }, scale);

    RenderQueue queue;
    queue.Submit(Layer::Foreground, [] { DrawRectangle(800, 60, 20, 20, RED); });
    queue.Submit(Layer::Background, [] { DrawRectangle(800, 60, 20, 20, BLUE); });

    // Best fit: the 8x8 frame goes into a 60x100 box. min(60/8, 100/8) = 7.5 rounds down to 7,
    // so it is drawn 56x56 (square, not stretched), standing on the box's bottom-centre:
    // x 902-958, y 84-140. Frame 0 is red on its left half, white on its right half.
    const bool fitted = assets.DrawFitted(*walk, 0, {900, 40, 60, 100}, Facing::Right) == DrawResult::Texture &&
                        assets.DrawFitted(*walk, 0, {1000, 40, 60, 100}, Facing::Left) == DrawResult::Texture;
    bool fittedPlaceholder = false;
    assets.DrawFitted(*ghost, 0, {1100, 40, 40, 80}, Facing::None, 1, [&](Rectangle bounds, bool) {
        // Centre pivot: 8x8 fits 40x80 at 5x, so a 40x40 box centred in the target.
        fittedPlaceholder = bounds.x == 1100 && bounds.y == 60 && bounds.width == 40 && bounds.height == 40;
    });
    queue.SubmitFitted(assets, *walk, 3, {1200, 40, 32, 32}); // Frame 3 (yellow) fills 32x32 at 4x.
    queue.Flush();
    EndTextureMode();
    Check(drew && customPlaceholder, "Draw results or placeholder bounds are wrong");
    Check(fitted && fittedPlaceholder, "Best-fit draw results or placeholder bounds are wrong");

    Image rendered = LoadImageFromTexture(target.target.texture);
    ImageFlipVertical(&rendered);
    const auto pixel = [&](int x, int y) { return GetImageColor(rendered, x, y); };

    const bool pivotOk = Same(pixel(88, 80), red) && Same(pixel(110, 80), white) && Same(pixel(80, 80), BLACK) &&
                         Same(pixel(100, 102), BLACK);
    const bool flipOk = Same(pixel(188, 80), white) && Same(pixel(210, 80), red);
    const bool gridOk = Same(pixel(300, 80), yellow);
    const bool topLeftOk = Same(pixel(404, 64), cyan) && Same(pixel(396, 64), BLACK);
    const Color placeholder = pixel(500, 70);
    const bool placeholderOk = placeholder.r > 0 && placeholder.b > 0 && Same(pixel(604, 64), BLACK);
    const bool layerOk = Same(pixel(810, 70), RED);
    // A sharp edge at x 929/930 with no blended colour in between, no stretching above y 84,
    // the bottom on the box's bottom edge, mirroring, and the queue path.
    const bool fitOk = Same(pixel(905, 120), red) && Same(pixel(929, 120), red) && Same(pixel(930, 120), white) &&
                       Same(pixel(955, 120), white) && Same(pixel(930, 80), BLACK) && Same(pixel(930, 139), white) &&
                       Same(pixel(930, 141), BLACK);
    const bool fitFlipOk = Same(pixel(1005, 120), white) && Same(pixel(1055, 120), red);
    const bool fitQueueOk = Same(pixel(1216, 56), yellow);
    UnloadImage(rendered);
    Check(pivotOk, "Bottom-centre pivot or frame 0 rendering failed");
    Check(flipOk, "Horizontal flip failed");
    Check(gridOk, "Grid frame selection failed");
    Check(topLeftOk, "Top-left pivot or static asset rendering failed");
    Check(placeholderOk, "Missing art did not draw a placeholder, or an invisible object was drawn");
    Check(layerOk, "Layers were drawn in the wrong order");
    Check(fitOk, "Best fit did not fill its box crisply without stretching, or left its pivot");
    Check(fitFlipOk, "Best fit did not mirror correctly");
    Check(fitQueueOk, "RenderQueue::SubmitFitted did not draw the fitted frame");
    // Exercise the same viewport and camera composition as the live game. These
    // test targets stand in for a physical framebuffer, not the old world canvas.
    for (const int width : {960, 1366, 1920, 2560}) {
        const int height = width * 10 / 16; // Deliberate top/bottom letterbox.
        const Rectangle viewport = CanvasDestination(static_cast<float>(width), static_cast<float>(height));
        const Matrix screen = ViewportMatrix(viewport);
        const Camera2D camera{{400, 300}, {100, 100}, 0, 2};
        const Matrix world = CameraModelview(camera, screen);
        const test::TestRenderTarget native(width, height);
        resources.BeginFrame();
        resources.SetViewport(viewport);
        BeginTextureMode(native.target);
        ClearBackground(BLACK);
        {
            const ScopedModelview logical(screen);
            resources.BeginClip(worldViewport);
            {
                const ScopedModelview cameraScope(world);
                // Source pixels, facing, animation and local matrix transforms must
                // still work when the viewport is applied AFTER the world camera.
                assets.Draw(*walk, 0, {100, 100}, Facing::Right, {}, 4);
                assets.Draw(*walk, 0, {160, 100}, Facing::Left, {}, 4);
                assets.Draw(*walk, 3, {220, 100}, Facing::Right, {}, 4);
                rlPushMatrix();
                rlTranslatef(280, 100, 0);
                rlScalef(1, 0.5f, 1);
                assets.Draw(*walk, 0, {0, 0}, Facing::Right, {}, 4);
                rlPopMatrix();
                DrawRectangle(-1000, 275, 3000, 80, MAGENTA); // Crosses world clip.
            }
            EndScissorMode();
            DrawRectangle(50, 700, 50, 50, cyan); // Camera must not affect HUD.
        }
        EndTextureMode();
        Image image = LoadImageFromTexture(native.target.texture);
        ImageFlipVertical(&image);
        const auto sample = [&](Vector2 point, Matrix transform) {
            const Vector2 pixel = Vector2Transform(point, transform);
            return GetImageColor(image, static_cast<int>(pixel.x), static_cast<int>(pixel.y));
        };
        Check(Same(sample({90, 84}, world), red) && Same(sample({110, 84}, world), white),
              "Native camera moved or filtered the sprite/pivot");
        Check(Same(sample({150, 84}, world), white) && Same(sample({170, 84}, world), red),
              "Native camera broke sprite mirroring");
        Check(Same(sample({220, 84}, world), yellow), "Native camera broke sprite frame selection");
        Check(Same(sample({270, 92}, world), red) && Same(sample({290, 92}, world), white) &&
              Same(sample({270, 76}, world), BLACK), "Native camera broke nested sprite scaling");
        Check(Same(sample({800, 655}, screen), MAGENTA) && Same(sample({800, 665}, screen), BLACK),
              "Native world clipping leaked into HUD");
        Check(Same(sample({75, 725}, screen), cyan) && Same(GetImageColor(image, width/2, 0), BLACK),
              "Native HUD transform or letterbox changed");
        const auto a = Vector2Transform({84, 80}, world);
        const auto b = Vector2Transform({116, 80}, world);
        for (int x = static_cast<int>(std::ceil(a.x)); x < static_cast<int>(b.x); ++x) {
            const Color color = GetImageColor(image, x, static_cast<int>(a.y));
            Check(Same(color, red) || Same(color, white), "Native sprite sampling introduced blended pixels");
        }
        UnloadImage(image);
    }
    std::cout << "PASS: native camera, sprite frames, mirroring, nested transforms, clipping and letterbox at four resolutions\n";
    std::cout << "PASS: Game Data textures (static, strip/grid animation, shared PNG), pivots, flipping, placeholders, layers and crisp best fit\n";
}

// Exercise a real mapping from level_1_sprites.cpp, not just a generic sprite.
// Adding a CSV row must replace the original sign, without hiding its front text.
void TestMappedLevelSpriteRendering() {
    TestDirectory fixture;
    const auto csvPath = fixture.path / gameDataPath;
    std::filesystem::create_directories(csvPath.parent_path());
    std::filesystem::create_directories(fixture.path / staticArtFolder);
    const Color marker{164, 23, 201, 255};
    Image sign = GenImageColor(41, 69, marker);
    const bool exported = ExportImage(sign, (fixture.path / staticArtFolder / "school.png").string().c_str());
    UnloadImage(sign);
    Check(exported, "Could not create mapped sign fixture");
    // Keep identical font metrics in the before/after resources.
    const auto source = FindResourceDirectory();
    for (const auto path : fontPaths) {
        const auto destination = fixture.path / path;
        if (std::filesystem::exists(source / path)) {
            std::filesystem::create_directories(destination.parent_path());
            std::filesystem::copy_file(source / path, destination);
        }
    }
    Resources original(fixture.path);
    {
        std::ofstream csv(csvPath);
        csv << "Animation ID,Asset ID,File Name,Frame Width (px),Frame Height (px),Frames,Frames per Row,FPS,Loops,Pivot,Faces,Layer,Notes\n"
               "school,sign_school,school.png,41,69,1,,,,top-left,,Middle,\n";
    }
    Resources replaced(fixture.path);
    GameConfig config;
    auto level = CreateLevel(LevelId::Level1, config);
    const auto *object = level->Objects().Get("sign-school");
    Check(object && object->AssetId() == "sign_school" && object->HasStandIn(), "School sprite mapping changed");
    const auto *animation = replaced.Assets().Find("sign_school", "");
    Check(animation && replaced.Assets().HasTexture(*animation), "Mapped school sprite did not load");
    for (const int width : {960, 1366, 1920, 2560}) {
        const int height = width * 9 / 16;
        const Rectangle viewport = CanvasDestination(width, height);
        const Matrix screen = ViewportMatrix(viewport);
        const Matrix world = CameraModelview(level->Camera(), screen);
        auto render = [&](Resources &resources) {
            const test::TestRenderTarget target(width, height);
            resources.BeginFrame();
            resources.SetViewport(viewport);
            BeginTextureMode(target.target);
            ClearBackground(BLACK);
            { const ScopedModelview scope(screen); level->DrawScene(resources, config); }
            EndTextureMode();
            Image image = LoadImageFromTexture(target.target.texture);
            ImageFlipVertical(&image);
            return image;
        };
        Image before = render(original);
        Image after = render(replaced);
        const auto point = Vector2Transform({498, 1348}, world);
        const Color previous = GetImageColor(before, int(point.x), int(point.y));
        const Color current = GetImageColor(after, int(point.x), int(point.y));
        // Count exactly matching opaque text pixels in the original and replaced sign.
        const Vector2 a = Vector2Transform({500, 1350}, world);
        const Vector2 b = Vector2Transform({570, 1374}, world);
        int glyphs = 0;
        for (int y = int(a.y); y < int(b.y); ++y)
            for (int x = int(a.x); x < int(b.x); ++x)
                if (Same(GetImageColor(before, x, y), RAYWHITE) &&
                    Same(GetImageColor(after, x, y), RAYWHITE)) ++glyphs;
        UnloadImage(before);
        UnloadImage(after);
        Check(Same(previous, Color{57, 110, 142, 255}) && Same(current, marker),
              "Mapped sprite did not replace its original stand-in at the same pivot");
        Check(glyphs > 10, "DrawWorldFront text was lost behind the mapped sprite");
    }
    std::cout << "PASS: actual Level 1 mapped sprite replaces stand-in and preserves front text at four resolutions\n";
}

}
