#include "objects/bird.h"
#include "test_support.h"

#include <array>
#include <fstream>
#include <iostream>
#include <limits>

namespace rage {
namespace {
using test::Check;
using test::Near;

constexpr Rectangle distantPlayer{-10000, -10000, 30, 44};

void TestPoleFlight() {
    // Recorded positions from Level 1's original flight at 1.75 seconds.
    const std::array<PoleFlight, 3> routes{{
        {{3240, 1120}, 2.2f, 0.4f},
        {{3415, 1010}, 1.8f, 2.1f},
        {{3610, 882}, 2.5f, 4.6f}
    }};
    const std::array<Vector2, 3> expected{{
        {3164.8209f, 1080.0756f}, {3342.8495f, 966.0152f}, {3646.5203f, 842.8475f}
    }};
    const std::array<Facing, 3> directions{Facing::Left, Facing::Right, Facing::Left};
    for (std::size_t i = 0; i < routes.size(); ++i) {
        Bird bird(routes[i]);
        const Rectangle initial = bird.Bounds();
        bird.Update(1.75f, 1.75f, distantPlayer);
        const Rectangle bounds = bird.Bounds();
        Check(Near(bounds.x, expected[i].x) && Near(bounds.y, expected[i].y),
              "Level 1 bird loop, rhythm or phase changed");
        Check(bounds.width == 38 && bounds.height == 27 && bird.Direction() == directions[i],
              "Level 1 bird hitbox or facing changed");
        bird.Reset();
        Check(Near(bird.Bounds().x, initial.x) && Near(bird.Bounds().y, initial.y) &&
                  bird.Animation().Seconds() == 0 && bird.Animation().State() == "fly",
              "Pole bird restart did not restore its initial phase and animation");
        // This mode follows the absolute level clock, even when the gameplay step is capped.
        bird.Update(0.1f, 1.75f, distantPlayer);
        Check(Near(bird.Bounds().x, bounds.x) && Near(bird.Bounds().y, bounds.y),
              "Pole bird movement depended on the capped gameplay step");
    }
}

void TestPatrolFlight() {
    Bird bird(PatrolFlight{{250, 1200}, 150, 200, 520, 1.5f});
    bird.Update(1, 1, distantPlayer);
    Check(bird.Bounds().x == 400 && bird.Bounds().y == 1200 && bird.Direction() == Facing::Right,
          "Patrol bird speed or home height changed");
    bird.Update(1, 2, distantPlayer);
    Check(bird.Bounds().x == 520 && bird.Direction() == Facing::Left,
          "Patrol bird did not clamp and turn at the right edge");
    bird.Update(3, 5, distantPlayer);
    Check(bird.Bounds().x == 200 && bird.Direction() == Facing::Right,
          "Patrol bird did not clamp and turn at the left edge");
    Check(bird.Bounds().width == 38 && bird.Bounds().height == 22, "Level 2 bird hitbox changed");
    bird.Reset();
    Check(bird.Bounds().x == 250 && bird.Bounds().y == 1200 && bird.Direction() == Facing::Right &&
              bird.Animation().Seconds() == 0,
          "Patrol restart did not restore its spawn, facing and animation");

    Bird left(PatrolFlight{{650, 1080}, 170, 500, 850, 2, false});
    left.Update(0.5f, 0.5f, distantPlayer);
    Check(left.Bounds().x == 565 && left.Direction() == Facing::Left, "Left-facing patrol started in the wrong direction");
    const Rectangle paused = left.Bounds();
    for (float dt : {0.0f, -1.0f, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()}) {
        left.Update(dt, 1, distantPlayer);
    }
    Check(left.Bounds().x == paused.x && left.Bounds().y == paused.y && left.Animation().Seconds() == 0.5,
          "Zero or invalid time advanced patrol movement or animation");
}

void TestSwoopsAndAttacks() {
    const Rectangle nearby{100, 100, 38, 22};
    Bird bird(PatrolFlight{{100, 100}, 0, 0, 1000, 0.5f});
    bird.Update(0.25f, 0.25f, nearby);
    Check(bird.Bounds().y == 100 && bird.Animation().State() == "fly", "Bird swooped before its initial delay");
    bird.Update(0.25f, 0.5f, {360, 100, 38, 22});
    bird.Update(0.1f, 0.6f, {100, 320, 38, 22});
    Check(bird.Bounds().y == 100, "Swoop proximity thresholds changed");
    bird.Update(0.2f, 0.8f, nearby);
    Check(Near(bird.Bounds().y, 170.7107f) && bird.Animation().State() == "swoop",
          "Nearby player did not trigger the original swoop curve");
    bird.Update(0.2f, 1, distantPlayer);
    Check(Near(bird.Bounds().y, 200), "Swoop did not reach its original 100-pixel depth");
    bird.Update(0.4f, 1.4f, distantPlayer);
    Check(bird.Bounds().y == 100 && bird.Animation().State() == "fly", "Swoop did not return home after 0.8 seconds");
    bird.Update(2.8f, 4.2f, nearby);
    Check(bird.Bounds().y == 100, "Bird swooped again before the 3.5-second cooldown");
    bird.Update(0.2f, 4.4f, nearby);
    Check(bird.Animation().State() == "swoop", "Bird did not rearm its swoop");
    Check(!bird.TryAttack(distantPlayer), "Bird hit a distant player");
    Check(bird.TryAttack(bird.Bounds()) && bird.Bounds().y == 100 && bird.Animation().State() == "fly",
          "Patrol attack did not cancel the swoop and return home");
    Check(!bird.TryAttack(nearby), "Patrol attack ignored its per-bird cooldown");
    bird.Update(0.9f, 5.3f, distantPlayer);
    Check(!bird.TryAttack(nearby), "Patrol bird attacked again before one second");
    bird.Update(0.11f, 5.41f, distantPlayer);
    Check(bird.TryAttack(nearby), "Patrol attack did not rearm after one second");
    bird.Reset();
    bird.Update(0.25f, 0.25f, nearby);
    Check(bird.Animation().State() == "fly" && bird.TryAttack(nearby),
          "Reset retained the previous swoop or attack cooldown");

    Bird other(PatrolFlight{{100, 100}, 0, 0, 1000, 0});
    Check(other.TryAttack(nearby), "Bird instances shared their attack cooldown");
}

bool Same(Color a, Color b) { return a.r == b.r && a.g == b.g && a.b == b.b; }
}

void TestBirds() {
    TestPoleFlight();
    TestPatrolFlight();
    TestSwoopsAndAttacks();
    std::cout << "PASS: shared birds, pole loops, patrol bounds, swoops, attack cooldowns and restart\n";
}

void TestBirdRendering() {
    test::TestDirectory fixture;
    std::filesystem::create_directories((fixture.path / gameDataPath).parent_path());
    std::filesystem::create_directories(fixture.path / animatedArtFolder);
    const auto writeCsv = [&](bool includeSwoop) {
        std::ofstream csv(fixture.path / gameDataPath);
        csv << "Animation ID,Asset ID,File Name,Frame Width (px),Frame Height (px),Frames,Frames per Row,FPS,Loops,Pivot,Faces,Layer,Notes\n"
               "bird_fly,bird,bird.png,8,8,2,,4,Yes,centre,right,Foreground,\n";
        if (includeSwoop) csv << "bird_swoop,bird,swoop.png,8,8,1,,,,centre,right,Foreground,\n";
    };
    writeCsv(true);
    Image sheet = GenImageColor(16, 8, BLUE);
    Image swoop = GenImageColor(8, 8, YELLOW);
    ImageDrawRectangle(&sheet, 0, 0, 4, 8, RED);
    ImageDrawRectangle(&sheet, 4, 0, 4, 8, WHITE);
    const bool exported = ExportImage(sheet, (fixture.path / animatedArtFolder / "bird.png").string().c_str()) &&
                          ExportImage(swoop, (fixture.path / animatedArtFolder / "swoop.png").string().c_str());
    UnloadImage(sheet);
    UnloadImage(swoop);
    Check(exported, "Could not create bird sprite fixture");
    Resources resources(fixture.path);
    const auto &assets = resources.Assets();
    Bird right(PatrolFlight{{81, 89}, 0, 0, 1000, 10}); // Centre (100, 100).
    Bird left(PatrolFlight{{181, 89}, 0, 0, 1000, 10, false});
    Bird animated(PatrolFlight{{281, 89}, 0, 0, 1000, 10});
    animated.Update(0.25f, 0.25f, distantPlayer);
    Bird pole(PoleFlight{{381, 113}, 1, 0}); // Centre (400, 126.5).
    Bird diving(PatrolFlight{{481, 89}, 0, 0, 1000, 0});
    diving.Update(0.2f, 0.2f, diving.Bounds());
    const Rectangle collision = pole.Bounds();
    RenderQueue queue;
    right.SubmitSprites(queue, assets, 2);
    left.SubmitSprites(queue, assets, 2);
    animated.SubmitSprites(queue, assets, 2);
    pole.SubmitSprites(queue, assets, 2);
    diving.SubmitSprites(queue, assets, 2);
    // Submitted later, but the sheet's Foreground layer must keep birds in front.
    queue.Submit(Layer::Middle, [] { DrawRectangle(90, 90, 20, 20, GREEN); });
    BeginTextureMode(resources.Canvas());
    ClearBackground(BLACK);
    queue.Flush();
    EndTextureMode();
    Image rendered = LoadImageFromTexture(resources.Canvas().texture);
    ImageFlipVertical(&rendered);
    Check(Same(GetImageColor(rendered, 95, 100), RED) && Same(GetImageColor(rendered, 105, 100), WHITE),
          "Bird sprite pivot or sheet layer was ignored");
    Check(Same(GetImageColor(rendered, 195, 100), WHITE) && Same(GetImageColor(rendered, 205, 100), RED),
          "Bird sprite did not mirror with patrol facing");
    Check(Same(GetImageColor(rendered, 300, 100), BLUE), "Bird animation did not advance to the next sprite frame");
    Check(Same(GetImageColor(rendered, 395, 126), RED), "Pole bird did not use the shared sprite asset");
    Check(Same(GetImageColor(rendered, 500, 171), YELLOW), "Diving bird did not select the swoop sprite");
    UnloadImage(rendered);
    Check(pole.Bounds().x == collision.x && pole.Bounds().y == collision.y && pole.Bounds().height == 27,
          "Sprite dimensions changed bird collision bounds");

    // A fly-only sheet also works while diving, without a separate swoop row.
    writeCsv(false);
    AssetManager flyOnly;
    flyOnly.Load(fixture.path);
    diving.SubmitSprites(queue, flyOnly, 2);
    BeginTextureMode(resources.Canvas());
    ClearBackground(BLACK);
    queue.Flush();
    EndTextureMode();
    rendered = LoadImageFromTexture(resources.Canvas().texture);
    ImageFlipVertical(&rendered);
    const bool flyFallback = Same(GetImageColor(rendered, 495, 171), RED);
    UnloadImage(rendered);
    Check(flyFallback, "Missing swoop row did not fall back to the fly sprite");

    // Both an absent row and a missing PNG must retain a visible bird stand-in.
    AssetManager empty;
    AssetManager missing;
    std::filesystem::remove(fixture.path / animatedArtFolder / "bird.png");
    missing.Load(fixture.path);
    for (const auto *source : {&empty, &missing}) {
        RenderQueue fallback;
        right.SubmitSprites(fallback, *source, 2);
        BeginTextureMode(resources.Canvas());
        ClearBackground(MAGENTA);
        fallback.Flush();
        EndTextureMode();
        Image image = LoadImageFromTexture(resources.Canvas().texture);
        ImageFlipVertical(&image);
        const bool visible = !Same(GetImageColor(image, 100, 100), MAGENTA);
        UnloadImage(image);
        Check(visible, "Missing bird art did not draw the shared placeholder");
    }
    std::cout << "PASS: bird sprite replacement, animation, facing, layers, collision independence and placeholders\n";
}
}
