#include "levels/level_1/level_1.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace rage {
    namespace {
        // All coordinates use world pixels. The player's collider is 30x44.
        // Default movement (260 px/s) and jump (620 px/s, gravity 1500) allow
        // about 128 px of jump height. Ledges in the climb are 78-102 px apart.
        constexpr float floorY = 1480.0f;
        constexpr float collisionGraceSeconds = 0.85f;
        constexpr Vector2 slideStart{4700.0f, 741.0f};
        constexpr Vector2 slideEnd{4925.0f, 1436.0f};
        constexpr float slideDuration = 1.8f;
        constexpr std::array<Rectangle, 3> wires{{
            {2940.0f, 800.0f, 220.0f, 16.0f},
            {3230.0f, 785.0f, 200.0f, 16.0f},
            {3495.0f, 770.0f, 200.0f, 16.0f}
        }};

        bool Intersects(Rectangle a, Rectangle b) {
            return CheckCollisionRecs(a, b);
        }

        void MoveRoadCar(Rectangle& body, float speed, float dt) {
            body.x += speed * dt;
            if (body.x + body.width < 620.0f) body.x = 1780.0f;
        }

        void MoveSkyBird(Rectangle& body, float speed, float dt) {
            body.x += speed * dt;
            if (body.x + body.width < 3000.0f) body.x = 4510.0f;
        }

        // A visible warning precedes each 1.1s electrified window.
        float WirePhase(int i, float elapsed) {
            return std::fmod(elapsed + static_cast<float>(i) * 1.1f, 4.2f);
        }

        bool WireLive(int i, float elapsed) {
            const float phase = WirePhase(i, elapsed);
            return phase >= 1.15f && phase < 2.25f;
        }

        bool WireWarning(int i, float elapsed) {
            const float phase = WirePhase(i, elapsed);
            return phase >= 0.65f && phase < 1.15f;
        }

        void DrawCar(Rectangle r, Color paint) {
            DrawRectangleRec(r, paint);
            DrawRectangleRec({r.x + 22.0f, r.y + 6.0f, r.width - 44.0f, 16.0f}, {52, 79, 109, 255});
            DrawCircle(static_cast<int>(r.x + 22.0f), static_cast<int>(r.y + r.height), 10.0f, BLACK);
            DrawCircle(static_cast<int>(r.x + r.width - 22.0f), static_cast<int>(r.y + r.height), 10.0f, BLACK);
        }
    }

    const LevelDefinition& Level1Definition() {
        static const LevelDefinition definition{
            .id = LevelId::Level1,
            .title = "Level 1 - Walk to School",
            .description = "Sidewalk -> road -> telephone poles -> clouds -> rainbow -> campus",
            .plannedContent = "Jump sidewalk obstacles, dodge right-to-left traffic and its chicken pile-up, "
                              "climb to electrified telephone wires, duck a flying pig (tap S), "
                              "dodge falling stars, then ride the rainbow to campus.",
            .intro = {
                .text = "The school is right down the road. Just keep walking.\n"
                        "The traffic lights are extremely trustworthy. I promise.",
                .speaker = "The Entity",
                .speedMultiplier = 1.0f,
                .startDelay = 0.0f,
                .fontSize = 24.0f,
                .allowSkip = true,
                .allowReplay = true,
                .effects = {.holdSeconds = 3.0f, .fadeSeconds = 1.0f}
            },
            .accent = {119, 167, 209, 255},
            .backdrop = ArtId::Level1,
            .world = {0.0f, 0.0f, 5200.0f, 1600.0f},
            .spawn = {100.0f, 1436.0f},
            .platforms = {
                {0, floorY, 2140, 120},          // Sidewalk + road
                {1850, 1378, 150, 22},          // Car-pileup climb
                {1980, 1300, 145, 22},
                {2110, 1222, 140, 22},
                {2240, 1144, 140, 22},          // Telephone-pole climb
                {2370, 1066, 140, 22},
                {2500, 988, 140, 22},
                {2630, 910, 150, 22},
                {2770, 828, 155, 22},          // Safe sky checkpoint
                wires[0], wires[1], wires[2],  // Timed electric wires
                {3765, 700, 170, 22},          // Cloud parkour
                {3970, 715, 170, 22},
                {4170, 665, 180, 22},
                {4400, 755, 170, 22},
                {4605, 785, 160, 22},          // Rainbow entrance
                {4875, floorY, 325, 120}       // Campus sidewalk
            }
        };
        return definition;
    }

    Level1::Level1() : Level(Level1Definition()) {
        for (auto& event : CreateEvents()) Events().RegisterEvent(std::move(event));
    }

    void Level1::OnReset() {
        traffic = {{
            {{760.0f, 1428.0f, 108.0f, 52.0f}, -195.0f, {223, 96, 91, 255}},
            {{1240.0f, 1428.0f, 116.0f, 52.0f}, -165.0f, {240, 189, 88, 255}},
            {{1710.0f, 1428.0f, 120.0f, 52.0f}, -225.0f, {141, 122, 207, 255}}
        }};
        birds = {{
            {{3510.0f, 711.0f, 65.0f, 22.0f}, -112.0f},
            {{4260.0f, 598.0f, 70.0f, 20.0f}, -125.0f}
        }};
        stars = {{
            {3650.0f, 3515.0f, 510.0f, 0.0f, false, false, false},
            {4380.0f, 4255.0f, 510.0f, 0.0f, false, false, false}
        }};
        checkpoint = Definition().spawn;
        collisionCooldown = 0.0f;
        duckSeconds = 0.0f;
        pileupSeconds = 0.0f;
        slideProgress = 0.0f;
        chickenX = 1550.0f;
        collisionCount = 0;
        roadCheckpointReached = false;
        skyCheckpointReached = false;
        pileupStarted = false;
        pileupSettled = false;
        sliding = false;
        slideFinished = false;
        campusReached = false;
    }

    void Level1::Respawn(const char* notification) {
        MutablePlayer().Reset(checkpoint);
        collisionCooldown = collisionGraceSeconds;
        duckSeconds = 0.0f;
        ++collisionCount;
        Events().Notify({notification, static_cast<std::uint64_t>(collisionCount)});
    }

    void Level1::OnUpdate(float dt, const InputFrame& input) {
        // Slide motion stays inside the level rather than changing shared movement code.
        if (sliding) {
            slideProgress = std::min(1.0f, slideProgress + dt / slideDuration);
            const float ease = slideProgress * slideProgress * (3.0f - 2.0f * slideProgress);
            MutablePlayer().Reset({
                slideStart.x + (slideEnd.x - slideStart.x) * ease,
                slideStart.y + (slideEnd.y - slideStart.y) * ease
            });
            if (slideProgress >= 1.0f) {
                sliding = false;
                slideFinished = true;
            }
            return;
        }

        if (campusReached) return;
        // input.down is a key-press (not held state). Tap S to duck for 1 second.
        if (input.down && GetPlayer().Position().x >= 3740.0f) duckSeconds = 1.0f;
        duckSeconds = std::max(0.0f, duckSeconds - dt);
        collisionCooldown = std::max(0.0f, collisionCooldown - dt);

        for (std::size_t i = 0; i < traffic.size(); ++i) {
            if (pileupStarted && i > 0) continue;  // Two cars stop behind the chicken.
            MoveRoadCar(traffic[i].bounds, traffic[i].speed, dt);
        }
        for (auto& bird : birds) MoveSkyBird(bird.bounds, bird.speed, dt);
        if (!pileupStarted) chickenX = std::max(1460.0f, chickenX - 13.0f * dt);

        if (!pileupStarted && GetPlayer().Position().x >= 1290.0f) {
            pileupStarted = true;
            pileupSeconds = 0.0f;
            traffic[1].bounds.x = 1550.0f; // A car brakes behind the chicken.
            traffic[2].bounds.x = 1680.0f; // Its follower arrives too late.
        }
        if (pileupStarted && !pileupSettled) {
            pileupSeconds += dt;
            if (pileupSeconds >= 1.2f) pileupSettled = true;
        }

        const Rectangle player = GetPlayer().Bounds();
        const Vector2 position = GetPlayer().Position();
        for (auto& star : stars) {
            if (star.finished) continue;
            if (!star.warned && position.x >= star.triggerX && position.x < star.x + 170.0f
                && position.y < 1000.0f) {
                star.warned = true;
                star.warningSeconds = 0.55f;
                star.y = std::max(0.0f, position.y - 200.0f);
            }
            if (star.warned && !star.falling) {
                star.warningSeconds -= dt;
                if (star.warningSeconds <= 0.0f) star.falling = true;
            } else if (star.falling) {
                star.y += 1000.0f * dt;
                if (star.y >= 1600.0f) {
                    star.falling = false;
                    star.finished = true;
                }
            }
        }

        if (position.x >= 4650.0f && position.y < 900.0f && !slideFinished) {
            sliding = true;
            slideProgress = 0.0f;
            MutablePlayer().Reset(slideStart);
            Events().Notify({"level1.slide_started"});
            return;
        }

        if (collisionCooldown > 0.0f) return;

        // Sidewalk: two stationary objects to jump over.
        constexpr Rectangle trashCan{260.0f, 1426.0f, 46.0f, 54.0f};
        constexpr Rectangle hydrant{445.0f, 1437.0f, 38.0f, 43.0f};
        if (Intersects(player, trashCan) || Intersects(player, hydrant)) {
            Respawn("level1.obstacle_hit");
            return;
        }

        // Cars approach from the RIGHT and move LEFT across the roadway.
        for (std::size_t i = 0; i < traffic.size(); ++i) {
            if (pileupSettled && i > 0) continue;  // Wrecks become scenery/steps.
            if (Intersects(player, traffic[i].bounds)) {
                Respawn("level1.traffic_hit");
                return;
            }
        }

        for (const auto& bird : birds) {
            if (Intersects(player, bird.bounds)) {
                Respawn("level1.bird_hit");
                return;
            }
        }

        // Wire collision is a grounded contact, not simply passing beneath the wire.
        for (std::size_t i = 0; i < wires.size(); ++i) {
            if (!WireLive(static_cast<int>(i), ElapsedSeconds())) continue;
            const Rectangle wire = wires[i];
            const float feet = player.y + player.height;
            const bool standingOnWire = player.x < wire.x + wire.width && player.x + player.width > wire.x
                && std::abs(feet - wire.y) < 5.0f;
            if (standingOnWire) {
                Respawn("level1.wire_hit");
                return;
            }
        }

        // One flying pig: tap S to duck beneath its hitbox while it passes.
        const float pigX = 4160.0f - std::fmod(ElapsedSeconds() * 110.0f, 440.0f);
        const Rectangle pig{pigX, 609.0f, 94.0f, 45.0f};
        if (Intersects(player, pig) && duckSeconds <= 0.0f) {
            Respawn("level1.pig_hit");
            return;
        }

        for (const auto& star : stars) {
            if (star.falling && Intersects(player, {star.x - 14.0f, star.y - 18.0f, 28.0f, 38.0f})) {
                Respawn("level1.star_hit");
                return;
            }
        }

        // No collision floor exists beneath the suspended sky section.
        if (position.x > 2140.0f && position.x < 4875.0f && position.y >= 1520.0f) {
            Respawn("level1.fall_hit");
        }
    }

    void Level1::DrawWorld(const Resources&) const {
        // Sidewalk and roadway. The actual collision platforms are in LevelDefinition.
        DrawRectangleRec({0.0f, 1455.0f, 580.0f, 25.0f}, {151, 158, 175, 255});
        DrawRectangleRec({580.0f, 1430.0f, 1260.0f, 50.0f}, {50, 55, 71, 255});
        for (int x = 640; x < 1840; x += 145)
            DrawRectangleRec({static_cast<float>(x), 1455.0f, 80.0f, 5.0f}, {241, 216, 130, 255});
        DrawText("SIDEWALK", 70, 1390, 26, RAYWHITE);
        DrawText("ROAD - CARS APPROACH FROM THE RIGHT", 675, 1376, 23, RAYWHITE);
        DrawText("SKY CHECKPOINT", 2730, 755, 20, RAYWHITE);
        DrawText("S: DUCK (TAP)", 3940, 566, 22, RAYWHITE);

        // Trash can and fire hydrant: solid hazards independent of background art.
        DrawRectangleRec({260.0f, 1426.0f, 46.0f, 54.0f}, {74, 126, 119, 255});
        DrawRectangleRec({255.0f, 1421.0f, 56.0f, 8.0f}, {109, 160, 149, 255});
        DrawRectangleRec({445.0f, 1437.0f, 38.0f, 43.0f}, {214, 73, 83, 255});
        DrawCircle(464, 1433, 14, {236, 106, 106, 255});

        // Traffic lights do not perfectly agree with the approaching cars.
        for (int x : {780, 1170, 1500}) {
            DrawRectangleRec({static_cast<float>(x), 1322.0f, 7.0f, 108.0f}, {85, 87, 104, 255});
            DrawRectangleRec({static_cast<float>(x) - 13, 1290.0f, 33.0f, 55.0f}, {36, 38, 49, 255});
            const bool green = std::fmod(ElapsedSeconds() + static_cast<float>(x) * 0.003f, 4.2f) < 2.1f;
            DrawCircle(x + 3, 1304, 7, green ? Color{73, 78, 77, 255} : Color{235, 80, 79, 255});
            DrawCircle(x + 3, 1328, 7, green ? Color{112, 227, 144, 255} : Color{67, 73, 65, 255});
        }
        for (const auto& car : traffic) DrawCar(car.bounds, car.color);

        // The chicken makes one car brake; the next car becomes a cartoon pile-up.
        DrawEllipse(static_cast<int>(chickenX + 15.0f), 1465, 16, 12, {242, 239, 220, 255});
        DrawCircle(static_cast<int>(chickenX + 28.0f), 1456, 8, {247, 246, 232, 255});
        DrawTriangle({chickenX + 36, 1455}, {chickenX + 48, 1460}, {chickenX + 36, 1464}, {244, 168, 67, 255});
        if (pileupStarted) {
            DrawText("CLONK!", 1630, 1348, 30, {245, 208, 98, 255});
            if (pileupSettled) {
                DrawText("CAR PILE-UP: CLIMB UP!", 1720, 1315, 21, RAYWHITE);
            } else {
                DrawCircle(1695, 1404, 21, {253, 209, 123, 225});
                DrawCircle(1722, 1391, 15, {255, 244, 200, 230});
            }
        }

        // Stacked vehicle roofs and wooden poles communicate the climbing route.
        for (int i = 0; i < 3; ++i) {
            const float x = 1860.0f + 130.0f * static_cast<float>(i);
            const float y = 1385.0f - 78.0f * static_cast<float>(i);
            DrawCar({x, y, 120.0f, 38.0f}, Color{159, 119, 133, 255});
        }
        for (int i = 0; i < 5; ++i) {
            const float x = 2280.0f + 140.0f * static_cast<float>(i);
            const float top = 1120.0f - 78.0f * static_cast<float>(i);
            DrawRectangleRec({x, top, 14.0f, floorY - top}, {116, 93, 83, 255});
            DrawLineEx({x - 18.0f, top + 12.0f}, {x + 46.0f, top + 12.0f}, 6.0f, {153, 128, 108, 255});
        }

        // Telephone wires flash yellow before briefly becoming electrified.
        for (std::size_t i = 0; i < wires.size(); ++i) {
            const Rectangle w = wires[i];
            const bool live = WireLive(static_cast<int>(i), ElapsedSeconds());
            const bool warning = WireWarning(static_cast<int>(i), ElapsedSeconds());
            const Color glow = live ? Color{245, 101, 107, 255}
                : warning ? Color{254, 214, 87, 255} : Color{115, 194, 217, 255};
            DrawLineEx({w.x, w.y + 3}, {w.x + w.width, w.y + 3}, live ? 9.0f : 5.0f, glow);
            if (live) {
                for (float x = w.x + 22.0f; x < w.x + w.width; x += 45.0f)
                    DrawLineEx({x, w.y - 8}, {x + 11.0f, w.y + 2}, 3.0f, {255, 239, 156, 255});
            }
        }

        // Clouds (platform collisions are supplied separately by the base level).
        for (int i = 12; i < 17; ++i) {
            const auto& p = Definition().platforms[static_cast<std::size_t>(i)];
            DrawEllipse(static_cast<int>(p.x + p.width * 0.30f), static_cast<int>(p.y),
                        p.width * 0.38f, 26.0f, {205, 222, 239, 255});
            DrawEllipse(static_cast<int>(p.x + p.width * 0.65f), static_cast<int>(p.y - 8),
                        p.width * 0.37f, 30.0f, {232, 238, 249, 255});
        }

        for (const auto& bird : birds) {
            const auto& r = bird.bounds;
            DrawEllipse(static_cast<int>(r.x + r.width / 2), static_cast<int>(r.y + r.height / 2),
                        r.width / 4, 10.0f, {66, 54, 78, 255});
            DrawLineEx({r.x + 12, r.y + 12}, {r.x + 2, r.y}, 3, RAYWHITE);
            DrawLineEx({r.x + 52, r.y + 12}, {r.x + 65, r.y}, 3, RAYWHITE);
        }

        // Flying pig; tapping S grants a short collision-safe duck period.
        const float pigX = 4160.0f - std::fmod(ElapsedSeconds() * 110.0f, 440.0f);
        DrawEllipse(static_cast<int>(pigX + 46), 631, 43, 20, {237, 154, 174, 255});
        DrawCircle(static_cast<int>(pigX + 76), 625, 18, {244, 170, 188, 255});
        DrawEllipse(static_cast<int>(pigX + 25), 615, 22, 10, {254, 211, 221, 255});
        DrawEllipse(static_cast<int>(pigX + 58), 609, 20, 8, {254, 211, 221, 255});
        if (duckSeconds > 0.0f && GetPlayer().Position().x >= 3740.0f) {
            const auto p = GetPlayer().Position();
            DrawText("DUCK!", static_cast<int>(p.x - 20), static_cast<int>(p.y - 28), 20, {137, 239, 186, 255});
        }

        // Falling-star warnings appear above the drop point for 0.55 seconds.
        for (const auto& star : stars) {
            if (star.warned && !star.falling && !star.finished) {
                DrawCircle(static_cast<int>(star.x), static_cast<int>(star.y), 24, {255, 239, 141, 255});
                DrawText("!", static_cast<int>(star.x) - 6, static_cast<int>(star.y - 19), 32, {77, 57, 72, 255});
            }
            if (star.falling) {
                DrawLineEx({star.x - 10.0f, star.y - 65.0f}, {star.x, star.y}, 9.0f,
                           {243, 184, 117, 255});
                DrawCircle(static_cast<int>(star.x), static_cast<int>(star.y), 17.0f,
                           {255, 225, 149, 255});
            }
        }

        // Rainbow slide drops the player onto the campus sidewalk.
        const Color rainbow[] = {
            {247, 110, 117, 255}, {249, 164, 99, 255}, {252, 216, 111, 255},
            {136, 216, 146, 255}, {122, 190, 247, 255}, {176, 151, 222, 255}
        };
        for (int i = 0; i < 6; ++i) {
            const float shift = static_cast<float>(i) * 8.0f;
            DrawLineEx({4700.0f + shift, 780.0f}, {4950.0f + shift, 1480.0f}, 9.0f, rainbow[i]);
        }
        DrawText("RAINBOW SLIDE", 4535, 670, 23, RAYWHITE);
        DrawRectangleRec({4980, 1300, 170, 180}, {59, 89, 115, 255});
        DrawRectangleLinesEx({4980, 1300, 170, 180}, 4, {143, 205, 194, 255});
        DrawText("CAMPUS", 4990, 1320, 24, RAYWHITE);
        DrawRectangleRec({5040, 1390, 60, 90}, {26, 48, 65, 255});
        if (campusReached) DrawText("YOU MADE IT!", 4900, 1252, 24, {160, 237, 179, 255});
    }

    std::unique_ptr<Level> MakeLevel1() { return std::make_unique<Level1>(); }
}
