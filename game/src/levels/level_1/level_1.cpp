#include "levels/level_1/level_1.h"

#include "engine/graphics/layout.h"
#include "rlgl.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

namespace rage {
    namespace {
        // All coordinates use world pixels. Default standing/crouching colliders are 30x44/30x22.
        // Default jumps reach about 96 px, or 138 px from crouch for taller ledges.
        // Movement tuning lives in config.yaml; wall climbing is exclusive to Level 2.
        constexpr float floorY = 1480.0f;
        constexpr float collisionGraceSeconds = 0.35f;
        constexpr float roadStart = 580.0f;
        // Traffic enters the visible road at the right portal and descends
        // beneath the sidewalk at the left. The player's floor stays flat.
        constexpr float tunnelEntranceX = 2640.0f;
        constexpr float tunnelSpawnX = tunnelEntranceX + 140.0f;
        constexpr float rampStartX = 820.0f;
        constexpr float rampEndX = 580.0f;
        constexpr float underpassDepth = 110.0f;
        constexpr float tunnelEndX = tunnelEntranceX + 400.0f;
        constexpr float railHeight = 32.0f;
        constexpr Rectangle roadEdgeWall{tunnelEntranceX, floorY - railHeight,
                                         tunnelEndX - tunnelEntranceX, railHeight + 120};
        constexpr char roadSpringId[] = "road-edge-spring";
        constexpr float roadSpringWidth = 124.8f;
        constexpr float roadSpringDuration = 0.48f;
        constexpr float roadSpringLift = 36.0f;
        constexpr float roadPlateDepth = 7.0f;
        constexpr Rectangle roadSpringArt{tunnelEndX - roadSpringWidth, floorY - roadSpringLift,
                                           roadSpringWidth, roadSpringLift + roadPlateDepth};
        constexpr float blastDuration = 1.05f;
        constexpr float wreckFlightSeconds = 1.25f;
        constexpr float wreckStagger = 0.16f;
        constexpr float foundationY = 1572.0f;
        constexpr float starWarning = 0.24f;
        constexpr float cloudStarWarning = 0.7f;
        constexpr float starHitWidth = 28.0f;
        constexpr char anvilId[] = "doorway-anvil";
        constexpr float anvilPoofDuration = 0.45f;
        constexpr char exitId[] = "exit-level1";
        // Signs are much smaller than menu text. Fit to their actual world-space
        // bounds without the UI helper's 10-logical-pixel minimum.
        void DrawSignText(const Resources &resources, std::string_view text, Rectangle bounds,
                          float size, Color color, FontRole role = FontRole::Regular) {
            constexpr float spacing = 0.5f;
            size = std::min(size, bounds.height);
            float width = resources.MeasureText(text, size, spacing, role).x;
            while (width > bounds.width && size > 1.0f) {
                size = std::max(1.0f, std::min(size - 0.25f, size * bounds.width / width));
                width = resources.MeasureText(text, size, spacing, role).x;
            }
            resources.DrawText(text, {bounds.x + (bounds.width - width) / 2,
                                     bounds.y + (bounds.height - size) / 2}, size, spacing, color, role);
        }
        void DrawAnvil(Rectangle b) {
            const Color steel{62, 73, 92, 255};
            DrawTriangle({b.x, b.y + 2}, {b.x + 37, b.y + 22}, {b.x + 37, b.y + 2}, steel);
            DrawRectangleRec({b.x + 32, b.y, b.width - 32, 16}, steel);
            DrawRectangleRec({b.x + 32, b.y, b.width - 32, 4}, LIGHTGRAY);
            DrawRectangleRec({b.x + 55, b.y + 14, 34, b.height - 22}, steel);
            DrawRectangleRounded({b.x + 36, b.y + b.height - 11, 74, 11}, 0.3f, 4, steel);
        }
        void DrawAnvilPoof(Rectangle b, float elapsed) {
            const float t = elapsed / anvilPoofDuration;
            if (t >= 1) return;
            if (t < 0.2f) DrawAnvil(b);
            const float center = b.x + b.width * 0.5f;
            for (int i = -3; i <= 3; ++i) {
                const float spread = i / 3.0f;
                const Vector2 puff{center + spread * (b.width * 0.4f + 26 * t),
                                   b.y + b.height - 12 - (1 - std::abs(spread)) * 19 - 24 * t};
                const float radius = 13 + (1 - std::abs(spread)) * 7 + 7 * t;
                DrawCircleV(puff, radius, Fade({197, 204, 216, 255}, 1 - t));
                DrawCircleV({puff.x - 2, puff.y - 4}, radius * 0.72f, Fade(RAYWHITE, 1 - t));
            }
        }
        constexpr float pigScale = 1.35f;
        // World-space flight heights for the upper, middle, and lower cloud routes.
        constexpr std::array<float, 3> pigRouteY{475, 620, 750};
        constexpr float gasLifetime = 1.8f;
        constexpr float gasFadeSeconds = 0.45f;

        float GasRadius(float age) {
            const float growth = std::min(age / 0.28f, 1.0f);
            const float fade = std::clamp((gasLifetime - age) / gasFadeSeconds, 0.0f, 1.0f);
            return std::lerp(16.8f, 61.6f, growth) * fade;
        }

        void DrawGasPuff(Vector2 center, float age) {
            const float radius = GasRadius(age);
            const float opacity = std::min((gasLifetime - age) / gasFadeSeconds, 1.0f);
            DrawCircleV(center, radius, Fade({66, 153, 44, 255}, 0.82f * opacity));
            DrawCircleV({center.x - radius * 0.6f, center.y + radius * 0.2f},
                        radius * 0.65f, Fade({109, 196, 53, 255}, 0.8f * opacity));
            DrawCircleV({center.x + radius * 0.45f, center.y - radius * 0.25f},
                        radius * 0.72f, Fade({163, 221, 64, 255}, 0.87f * opacity));
        }

        constexpr std::array<Rectangle, 3> refuges{{
            {3975, 705, 96, 16},
            {4260, 690, 96, 16},
            {4535, 675, 92, 16}
        }};

        // The solid surfaces are the flat roofs, not the entire car chassis.
        // Keep the original step centres and heights; the chassis overhangs the roof.
        constexpr std::array<Rectangle, 3> wreckSteps{{
            {2778, 1378, 88, 16},
            {2906, 1300, 88, 16},
            {3034, 1222, 88, 16}
        }};
        constexpr float pileupDuration = wreckFlightSeconds + (wreckSteps.size() - 1) * wreckStagger;
        constexpr std::array<const char*, 3> wreckIds{{"pileup-step-1", "pileup-step-2", "pileup-step-3"}};
        constexpr Rectangle trashCan{260, 1426, 46, 54};
        constexpr Rectangle hydrant{445, 1437, 38, 43};
        constexpr Rectangle firstCloud{4690, 700, 170, 22};
        constexpr Rectangle rainbowEntrance{6610, 785, 160, 22};
        // The stripe marks the near edge of the trigger. Its depth catches fast crossings.
        constexpr Rectangle rainbowGate{rainbowEntrance.x + 20, 400, rainbowEntrance.width - 20, 500};
        bool InCloudSection(float x) {
            return x >= firstCloud.x && x < rainbowEntrance.x;
        }

        void DrawCloudStarWarning(float x, float y, float bottom, float remaining) {
            constexpr Color amber{255, 190, 58, 255};
            DrawRectangleRec({x - starHitWidth / 2, y, starHitWidth, bottom - y}, Fade(amber, 0.22f));
            const float offset = std::fmod((cloudStarWarning - remaining) * 60, 38.0f);
            for (float arrowY = y + 18 + offset; arrowY < bottom; arrowY += 38) {
                DrawLineEx({x - 7, arrowY}, {x, arrowY + 6}, 3, amber);
                DrawLineEx({x, arrowY + 6}, {x + 7, arrowY}, 3, amber);
            }
            DrawCircleV({x, y}, 14, amber);
            DrawLineEx({x, y - 8}, {x, y + 2}, 4, DARKBROWN);
            DrawCircleV({x, y + 7}, 2, DARKBROWN);
        }
        constexpr Vector2 slideStart{rainbowGate.x - 15, rainbowEntrance.y - 44};
        constexpr Vector2 slideEnd{6930.0f, 1436.0f};
        constexpr float slideDuration = 1.8f;
        constexpr std::array<Rectangle, 3> wires{{
            {3850.0f, 796.0f, 240.0f, 12.0f},
            {4150.0f, 781.0f, 220.0f, 12.0f},
            {4430.0f, 766.0f, 220.0f, 12.0f}
        }};

        float RoadDepth(float x) {
            const float t = std::clamp((rampStartX - x) / (rampStartX - rampEndX), 0.0f, 1.0f);
            return underpassDepth * t * t;
        }

        bool MoveRoadCar(Rectangle& body, float speed, float dt, float spawnX) {
            body.x += speed * dt;
            // Wrap only once the entire vehicle has passed behind the sidewalk.
            const bool finishedPass = body.x + body.width <= roadStart;
            if (finishedPass) body.x = spawnX;
            body.y = floorY - body.height + RoadDepth(body.x + body.width * 0.5f);
            return finishedPass;
        }

        // Never collide with the hidden portion of a car inside either tunnel.
        Rectangle VisibleRoadCar(Rectangle body) {
            const float left = std::max(body.x, roadStart);
            const float right = std::min(body.x + body.width, tunnelEntranceX);
            return {left, body.y, std::max(0.0f, right - left), body.height};
        }
        Vector2 SlidePoint(float t) {
            // The rider and the painted rainbow follow the same curve.
            return {slideStart.x + (slideEnd.x - slideStart.x) * t,
                    slideStart.y + (slideEnd.y - slideStart.y) * t * t};
        }
        bool SignalGreen(float x, float elapsed, bool carNear) {
            // Normally honest, but lies for a short, repeatable interval.
            const bool lying = std::fmod(elapsed + x * 0.004f, 7.0f) > 5.85f;
            return lying ? carNear : !carNear;
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
        // Clouds are top-only platforms: jump through them from below.
        GameObjectDefinition CloudPlatform(const char* id, Rectangle bounds) {
            auto cloud = SolidPlatform(id, bounds, "");
            cloud.collision = CollisionType::OneWay;
            return cloud;
        }
        void DrawCar(Rectangle r, Color paint) {
            const float roof = r.height * 0.42f;
            DrawRectangleRounded({r.x, r.y + roof, r.width, r.height - roof - 5}, 0.22f, 6, paint);
            DrawRectangleRounded({r.x + r.width * 0.22f, r.y, r.width * 0.58f, roof + 9}, 0.25f, 6, paint);
            DrawRectangleRec({r.x + r.width * 0.27f, r.y + 4, r.width * 0.20f, roof - 3}, {117, 174, 198, 255});
            DrawRectangleRec({r.x + r.width * 0.52f, r.y + 4, r.width * 0.21f, roof - 3}, {117, 174, 198, 255});
            DrawRectangleRec({r.x + 2, r.y + roof + 5, 10, 7}, {255, 239, 161, 255});
            DrawRectangleRec({r.x + r.width - 10, r.y + roof + 5, 8, 7}, {244, 77, 87, 255});
            for (float offset : {0.21f, 0.79f}) {
                const int x = static_cast<int>(r.x + r.width * offset);
                const int y = static_cast<int>(r.y + r.height - 8);
                DrawCircle(x, y, 8, {32, 34, 48, 255});
                DrawCircle(x, y, 3, {153, 161, 178, 255});
            }
        }
        // A cutaway roadway descends under the pedestrian path. Draw it behind
        // the vehicles, so the car wheels follow the same slope as the art.
        void DrawUnderpassRoad() {
            const Color asphalt{49, 55, 70, 255};
            const Color edge{151, 158, 169, 255};
            // Dark mouth behind the ramp, with masonry around the opening.
            DrawRectangleRec({roadStart, floorY - 4, 54, 120}, {25, 30, 41, 255});
            DrawRectangleRec({roadStart, floorY - 12, 54, 8}, {173, 181, 190, 255});
            // Repaint the road face with a curved retaining wall and road edge.
            for (int i = 0; i < 32; ++i) {
                const float a = roadStart + (rampStartX - roadStart) * i / 32.0f;
                const float b = roadStart + (rampStartX - roadStart) * (i + 1) / 32.0f;
                const float ay = floorY + RoadDepth(a);
                const float by = floorY + RoadDepth(b);
                DrawRectangleRec({a, floorY + 5, b - a + 0.5f,
                                  std::max(0.0f, (ay + by) * 0.5f - floorY - 5)},
                                 {87, 96, 110, 255});
                DrawLineEx({a, ay}, {b, by}, 4, edge);
                DrawLineEx({a, ay + 8}, {b, by + 8}, 3, asphalt);
                if ((i / 4) % 2 == 0)
                    DrawLineEx({a, ay + 17}, {b, by + 17}, 2, {225, 190, 105, 255});
            }
            // A thin upper deck makes the unchanged player path readable.
            DrawRectangleRec({roadStart, floorY, rampStartX - roadStart, 5}, {165, 175, 185, 255});
            DrawRectangleRec({roadStart + 3, floorY + 5, 6, 104}, {62, 72, 87, 255});
        }

        // The tunnel is a side-view cutaway: the opening faces left and its
        // solid wall conceals vehicles until they emerge onto the road.
        void DrawTunnelBackdrop() {
            const float x = tunnelEntranceX;
            DrawRectangleRec({x - 26, floorY - 103, 70, 103}, {40, 47, 60, 255});
            DrawRectangleRec({x - 20, floorY - 95, 50, 95}, {22, 29, 40, 255});
            DrawRectangleRec({x - 19, floorY - 92, 40, 5}, {246, 208, 130, 255});
            DrawRectangleRec({x - 19, floorY - 85, 40, 2}, {116, 99, 75, 255});
        }

        void DrawTunnelForeground() {
            const float x = tunnelEntranceX;
            const float width = tunnelEndX - x;
            const float top = floorY - 135;
            const Color wall{100, 111, 125, 255};
            const Color seam{76, 86, 99, 255};
            const Color trim{173, 183, 192, 255};
            // Opaque mask begins exactly where the traffic collision is clipped.
            DrawRectangleRec({x, top, width, floorY - top}, wall);
            for (int row = 0; row < 5; ++row) {
                const float y = top + 15 + row * 24;
                DrawLineEx({x + 17, y}, {tunnelEndX, y}, 1, seam);
                for (float bx = x + 42 + (row % 2) * 25; bx < tunnelEndX; bx += 50)
                    DrawLineEx({bx, y}, {bx, std::min(y + 24, floorY)}, 1, seam);
            }
            DrawRectangleRec({x, top, width, 10}, trim);
            DrawRectangleRec({x, top + 10, width, 4}, seam);
            // Beveled portal frame, lighting and hazard stripes.
            DrawRectangleRec({x, top + 10, 16, floorY - top - 10}, {67, 77, 91, 255});
            DrawRectangleRec({x, top + 10, 4, floorY - top - 10}, trim);
            DrawRectangleRec({x - 24, floorY - 110, 40, 12}, {138, 149, 161, 255});
            DrawRectangleRec({x - 24, floorY - 110, 40, 3}, trim);
            for (int j = 0; j < 4; ++j)
                DrawRectangleRec({x + 4, floorY - 32 + j * 8, 10, 4}, {234, 194, 100, 255});
            DrawRectangleRounded({x + 46, top + 23, 138, 28}, 0.18f, 4, {48, 63, 81, 255});
            DrawRectangleRec({x + 16, floorY - 5, width - 16, 5}, seam);
        }

        void DrawRoadEdgeWall(Rectangle b) {
            const Color cap{199, 202, 201, 255};
            const Color concrete{161, 166, 168, 255};
            const Color shade{103, 111, 116, 255};
            const Color joint{77, 84, 90, 255};
            const float deck = b.y + railHeight;
            const float bottom = b.y + b.height;
            const float end = b.x + b.width;
            constexpr int bays = 5;
            const float bayWidth = b.width / bays;

            // Cast panels below the deck. Recessed joints and form-tie holes
            // give the vertical face a different finish from the asphalt.
            DrawRectangleGradientV(static_cast<int>(b.x), static_cast<int>(deck),
                                   static_cast<int>(b.width), static_cast<int>(bottom - deck),
                                   concrete, {122, 129, 133, 255});
            for (int i = 0; i < bays; ++i) {
                const float x = b.x + i * bayWidth;
                DrawRectangleRec({x, deck + 12, 2, bottom - deck - 12}, joint);
                DrawRectangleRec({x + 2, deck + 12, 1, bottom - deck - 12}, concrete);
                for (float dx : {13.0f, bayWidth - 13}) {
                    for (float y : {deck + 29, bottom - 19}) {
                        DrawCircleV({x + dx, y + 1}, 2, concrete);
                        DrawCircleV({x + dx, y}, 1.5f, shade);
                    }
                }
            }
            DrawRectangleRec({end - 7, deck, 7, bottom - deck}, shade);
            DrawRectangleRec({end - 9, deck, 2, bottom - deck}, cap);

            // Bevelled coping at deck height and an open parapet above it.
            DrawRectangleRec({b.x, deck, b.width, 12}, shade);
            DrawRectangleRec({b.x, deck, b.width, 7}, concrete);
            DrawRectangleRec({b.x, deck, b.width, 2}, cap);
            for (int i = 0; i <= bays; ++i) {
                constexpr float postWidth = 13;
                const float x = std::clamp(b.x + i * bayWidth - postWidth / 2, b.x, end - postWidth);
                DrawRectangleRec({x, b.y + 8, postWidth, railHeight - 8}, concrete);
                DrawRectangleRec({x + postWidth - 4, b.y + 8, 4, railHeight - 8}, shade);
                DrawRectangleRec({x, b.y + 8, postWidth, 4}, shade);
            }
            DrawRectangleRec({b.x, b.y, b.width, 9}, shade);
            DrawRectangleRec({b.x, b.y, b.width, 6}, concrete);
            DrawRectangleRec({b.x, b.y, b.width, 2}, cap);
            for (int i = 1; i < bays; ++i)
                DrawRectangleRec({b.x + i * bayWidth, b.y, 2, 9}, shade);
        }

        void DrawUnderpassForeground() {
            // A departing car is drawn first; the opaque sidewalk hides its
            // left portion as it descends. Never cover the playable surface.
            DrawRectangleRec({0, floorY, roadStart, 120}, {108, 119, 133, 255});
            DrawRectangleRec({0, floorY, roadStart, 10}, {191, 199, 206, 255});
            for (int x = 0; x < static_cast<int>(roadStart); x += 72)
                DrawLineEx({static_cast<float>(x), 1488}, {static_cast<float>(x), 1520}, 1.0f, {76, 91, 108, 255});
            DrawRectangleRec({roadStart - 12, floorY + 10, 12, 110}, {65, 76, 91, 255});
            // Mask any car pixels still above the deck just inside the mouth.
            DrawRectangleRec({roadStart - 17, floorY - 5, 17, 15}, {191, 199, 206, 255});
            DrawRectangleRounded({roadStart - 116, floorY + 23, 91, 20}, 0.2f, 4, {48, 63, 81, 255});
        }

        // A damaged station wagon, with a flat roof that exactly spans the
        // collision rectangle. No separate floating platform is drawn above it.
        void DrawWreckCar(Rectangle roof, Color paint, float damage) {
            const float left = roof.x - 22.0f;
            const float width = roof.width + 44.0f;
            const float y = roof.y;
            const Color outline{42, 47, 62, 255};
            const Color glass{88, 133, 159, 255};
            const Color glassLight{155, 194, 208, 255};

            // Body and pillars, with crumpled front and rear bumpers.
            DrawRectangleRounded({left, y + 21, width, 25}, 0.24f, 6, outline);
            DrawRectangleRounded({left + 2, y + 22, width - 4, 21}, 0.24f, 6, paint);
            DrawTriangle({roof.x, y}, {left + 8, y + 23}, {roof.x, y + 23}, outline);
            DrawTriangle({roof.x + roof.width, y}, {roof.x + roof.width, y + 23},
                         {left + width - 8, y + 23}, outline);
            DrawRectangleRec({roof.x, y, roof.width, 24}, outline);
            DrawRectangleRec({roof.x + 3, y + 4, roof.width - 6, 19}, paint);
            DrawRectangleRec({roof.x + 5, y + 5, 23, 15}, glass);
            DrawRectangleRec({roof.x + 32, y + 5, roof.width - 38, 15}, glass);
            DrawLineEx({roof.x + 8, y + 7}, {roof.x + 20, y + 17}, 1.5f, glassLight);
            DrawLineEx({roof.x + 39, y + 7}, {roof.x + 50, y + 16}, 1.5f, glassLight);
            DrawLineEx({roof.x + 53, y + 7}, {roof.x + 49, y + 13}, 1.0f, Fade(outline, damage));
            DrawLineEx({roof.x + 49, y + 13}, {roof.x + 57, y + 20}, 1.0f, Fade(outline, damage));

            // Painted steel roof: its top and horizontal edges are the walkable surface.
            DrawRectangleRec({roof.x, y, roof.width, 4}, paint);
            DrawRectangleRec({roof.x, y, roof.width, 2}, {231, 230, 219, 255});
            DrawRectangleRec({left + 4, y + 28, 12, 5}, {249, 222, 148, 255});
            DrawRectangleRec({left + width - 13, y + 28, 9, 5}, {168, 62, 69, 255});
            DrawLineEx({left + 3, y + 40}, {left + width - 3, y + 40}, 2, outline);
            for (float offset : {0.22f, 0.79f}) {
                const int x = static_cast<int>(left + width * offset);
                const int wheelY = static_cast<int>(y + 45);
                DrawCircle(x, wheelY, 9, outline);
                DrawCircle(x, wheelY, 4, {143, 151, 163, 255});
                DrawCircle(x, wheelY, 2, {81, 90, 105, 255});
            }
        }

        // Both approaching crash cars and the finished roofs use the same body.
        // DrawWreckCar extends 22 px on each side of its roof and 54 px below it.
        void DrawCrashCar(Rectangle body, Color paint, float roofWidth, float damage, float tilt = 0.0f) {
            rlPushMatrix();
            rlTranslatef(body.x + body.width / 2, body.y + body.height / 2, 0);
            rlRotatef(tilt, 0, 0, 1);
            rlScalef(body.width / (roofWidth + 44), body.height / 54, 1);
            DrawWreckCar({-roofWidth / 2, -27, roofWidth, 16}, paint, damage);
            rlPopMatrix();
        }

        void DrawStreetPost(float center, float top, float width) {
            DrawRectangleRec({center - width / 2, top, width, floorY - top}, {85, 87, 104, 255});
            DrawRectangleRec({center - width / 2 + 1, top, 1, floorY - top}, {148, 157, 168, 255});
            DrawRectangleRec({center - width / 2 - 3, floorY - 5, width + 6, 5}, {83, 91, 103, 255});
        }

        struct Blast {
            Vector2 center;
            float radius, opacity;
        };
        Blast BlastAt(Vector2 impact, float seconds) {
            const float t = seconds / blastDuration;
            const float expansion = 1 - (1 - t) * (1 - t);
            return {{impact.x, impact.y - 23 * expansion}, 18 + 100 * expansion, 1 - t * t};
        }
        // The blast's shapes; its "CRASH!" text is drawn with the level's other text.
        void DrawCrashBlast(const Blast& blast) {
            const Vector2 center = blast.center;
            const float radius = blast.radius;
            const float opacity = blast.opacity;
            DrawRing(center, radius + 2, radius + 5, 0, 360, 64, Fade(GOLD, opacity));
            for (int i = 0; i < 10; ++i) {
                const float angle = i * 2 * PI / 10;
                const Vector2 ray{std::cos(angle), std::sin(angle)};
                const Vector2 puff{center.x + ray.x * radius * 0.55f,
                                   center.y + ray.y * radius * 0.55f};
                DrawCircleV(puff, radius * 0.45f, Fade({244, 106, 52, 255}, opacity));
                const float spark = radius + 9 + (i % 3) * 5;
                DrawLineEx({center.x + ray.x * spark, center.y + ray.y * spark},
                           {center.x + ray.x * (spark + 13), center.y + ray.y * (spark + 13)},
                           3, Fade(GOLD, opacity));
            }
            DrawCircleV(center, radius * 0.66f, Fade({255, 199, 77, 255}, opacity));
            DrawCircleV(center, radius * 0.37f, Fade({255, 246, 201, 255}, opacity));
        }

        // A smooth horizontal throw and a vertical arc share exact endpoints with
        // the resting artwork. The flash hides the launch of the smaller scrap cars.
        Rectangle WreckFlight(Rectangle from, Rectangle to, float t, float lift) {
            const float ease = t * t * (3 - 2 * t);
            return {std::lerp(from.x, to.x, ease),
                    std::lerp(from.y, to.y, ease) - 4 * t * (1 - t) * lift,
                    std::lerp(from.width, to.width, ease), std::lerp(from.height, to.height, ease)};
        }

        void DrawUtilityPole(Rectangle ledge) {
            const float center = ledge.x + ledge.width / 2;
            const float poleX = center - 9;
            const float bottom = ledge.y + ledge.height;
            const Color dark{83, 57, 48, 255};
            const Color wood{145, 104, 75, 255};
            const Color light{199, 155, 111, 255};
            const Color grain{113, 77, 58, 255};

            // Shaft, highlights, and grain. The shaft is scenery, not a ledge.
            DrawRectangleRec({poleX, bottom, 18, foundationY - bottom}, dark);
            DrawRectangleRec({poleX + 2, bottom, 13, foundationY - bottom}, wood);
            DrawRectangleRec({poleX + 3, bottom, 3, foundationY - bottom}, light);
            for (float y = bottom + 28; y < foundationY - 20; y += 53) {
                DrawLineEx({poleX + 10, y}, {poleX + 8, y + 19}, 1, grain);
            }
            DrawRectangleRec({center - 19, foundationY, 38, 28}, {79, 89, 101, 255});
            DrawRectangleRec({center - 19, foundationY, 38, 4}, {153, 166, 174, 255});
            // Diagonal timber braces support the crossarm from below.
            for (float side : {-1.0f, 1.0f}) {
                const Vector2 outer{center + side * (ledge.width / 2 - 15), bottom};
                const Vector2 inner{center + side * 7, bottom + 31};
                DrawLineEx(outer, inner, 7, dark);
                DrawLineEx({outer.x, outer.y - 1}, {inner.x, inner.y - 1}, 3, wood);
            }

            // Crossarm follows the exact solid bounds, including its top edge.
            DrawRectangleRec(ledge, dark);
            DrawRectangleRec({ledge.x + 1, ledge.y + 1, ledge.width - 2, ledge.height - 2}, wood);
            DrawRectangleRec({ledge.x, ledge.y, ledge.width, 3}, light);
            DrawRectangleRec({ledge.x, bottom - 3, ledge.width, 3}, grain);
            for (float x = ledge.x + 16; x < ledge.x + ledge.width - 12; x += 29) {
                DrawLineEx({x, ledge.y + 7}, {x + 11, ledge.y + 7}, 1, grain);
            }
            for (float x : {ledge.x + 8, center, ledge.x + ledge.width - 8}) {
                DrawCircle(static_cast<int>(x), static_cast<int>(ledge.y + 9), 2.5f, dark);
                DrawCircle(static_cast<int>(x), static_cast<int>(ledge.y + 8), 1, {204, 198, 181, 255});
            }
        }

        // Each span terminates on ceramic insulators. A shared timber mast
        // supports the two adjacent ends at every gap, so the cables do not
        // float. The supports sit BELOW the playable wire surface: they are
        // scenery and do not introduce new ledges or change the jump gaps.
        // There are N+1 masts for N wire spans. End masts sit under the outermost
        // insulators; interior masts sit between adjacent spans.
        struct WireMast {
            bool hasLeft, hasRight;
            Rectangle left, right;
            float leftX, rightX, center, top;
        };
        WireMast WireMastAt(std::size_t i) {
            WireMast m{};
            m.hasLeft = i > 0;
            m.hasRight = i < wires.size();
            m.left = m.hasLeft ? wires[i - 1] : wires.front();
            m.right = m.hasRight ? wires[i] : wires.back();
            m.leftX = m.left.x + m.left.width - 6.0f;
            m.rightX = m.right.x + 6.0f;
            m.center = !m.hasLeft ? m.rightX : !m.hasRight ? m.leftX : (m.leftX + m.rightX) * 0.5f;
            // Match the existing insulator base at wire.y + 25..29.
            m.top = !m.hasLeft ? m.right.y + 28 : !m.hasRight ? m.left.y + 28 : std::min(m.left.y, m.right.y) + 28;
            return m;
        }
        // The area one mast covers, brackets included, down to its footing.
        Rectangle WireMastBounds(std::size_t i) {
            const WireMast m = WireMastAt(i);
            float left = m.center - 19, right = m.center + 19;
            if (m.hasLeft) left = std::min(left, m.leftX - 9);
            if (m.hasRight) right = std::max(right, m.rightX + 9);
            return {left, m.top, right - left, foundationY + 28 - m.top};
        }

        void DrawWireMast(std::size_t i) {
            const Color shadow{71, 50, 43, 255};
            const Color timber{137, 98, 71, 255};
            const Color highlight{189, 146, 106, 255};
            const Color grain{103, 71, 53, 255};
            const Color steel{65, 78, 94, 255};
            const Color bolt{183, 195, 202, 255};
            const WireMast m = WireMastAt(i);
            const float center = m.center;
            const float top = m.top;

            // Concrete footing reaches the world boundary. No solid
            // ground is added beneath the sky section.
            DrawRectangleRec({center - 19, foundationY, 38, 28}, {79, 89, 101, 255});
            DrawRectangleRec({center - 19, foundationY, 38, 4}, {153, 166, 174, 255});
            DrawRectangleRec({center - 11, top, 22, foundationY - top}, shadow);
            DrawRectangleRec({center - 9, top, 16, foundationY - top}, timber);
            DrawRectangleRec({center - 8, top, 3, foundationY - top}, highlight);
            for (float y = top + 42; y < foundationY - 24; y += 64) {
                DrawLineEx({center + 2, y}, {center - 1, y + 23}, 1.5f, grain);
            }
            DrawRectangleRec({center - 13, foundationY - 18, 26, 9}, steel);

            // Staggered steel brackets follow the height of each span.
            // Their upper faces stay below the wire and insulator base.
            const auto attach = [&](float x, float wireY) {
                const float seatY = wireY + 28.0f;
                const float minX = std::min(center, x) - 9.0f;
                const float width = std::abs(x - center) + 18.0f;
                if (std::abs(x - center) > 1.0f) {
                    DrawLineEx({x, seatY + 7}, {center, seatY + 38}, 6, shadow);
                    DrawLineEx({x, seatY + 7}, {center, seatY + 38}, 2, highlight);
                }
                DrawRectangleRec({minX, seatY, width, 9}, shadow);
                DrawRectangleRec({minX, seatY, width, 3}, highlight);
                DrawRectangleRec({x - 9, seatY, 18, 5}, steel);
                DrawCircle(static_cast<int>(center), static_cast<int>(seatY + 5), 2, bolt);
            };
            if (m.hasLeft) attach(m.leftX, m.left.y);
            if (m.hasRight) attach(m.rightX, m.right.y);
        }

        void DrawElectricWire(Rectangle w, bool warning, bool live, float elapsed) {
            const float right = w.x + w.width;
            const Color dark{44, 55, 73, 255};
            const Color steel{118, 163, 186, 255};
            const Color amber{255, 211, 84, 255};
            const Color danger{246, 95, 107, 255};
            const Color spark{255, 246, 185, 255};
            const Color ceramic{191, 207, 218, 255};

            // The cable is flat at the collision surface; fittings hang below it.
            // The glow and sparks are decoration, not additional hitboxes.
            if (live || warning) {
                DrawLineEx({w.x, w.y + 3}, {right, w.y + 3}, live ? 13 : 8,
                           live ? Color{245, 90, 105, 70} : Color{255, 213, 89, 58});
            }
            DrawRectangleRec({w.x, w.y, w.width, 6}, dark);
            DrawRectangleRec({w.x, w.y, w.width, 2}, live ? danger : warning ? amber : steel);
            DrawRectangleRec({w.x, w.y + 3, w.width, 1}, {86, 103, 123, 255});
            DrawLineEx({w.x, w.y + 8}, {right, w.y + 8}, 2, dark);

            for (float x : {w.x + 6, right - 6}) {
                DrawRectangleRec({x - 2, w.y + 5, 4, 22}, dark);
                for (int ring = 0; ring < 3; ++ring) {
                    const float y = w.y + 10 + ring * 5;
                    DrawRectangleRounded({x - 6, y, 12, 4}, 0.4f, 4, ceramic);
                    DrawRectangleRec({x - 4, y + 3, 8, 1}, steel);
                }
                DrawRectangleRec({x - 8, w.y + 25, 16, 4}, dark);
            }

            if (warning && !live) {
                const float pulse = 0.65f + 0.35f * std::sin(elapsed * 16.0f);
                const Color marker{255, 236, 155, static_cast<unsigned char>(155 + 100 * pulse)};
                for (float x = w.x + 18; x < right - 12; x += 30)
                    DrawCircle(static_cast<int>(x), static_cast<int>(w.y + 1), 2, marker);
            }
            if (live) {
                const float phase = std::fmod(elapsed * 80.0f, 36.0f);
                for (float x = w.x + 8 + phase; x + 18 < right - 3; x += 36) {
                    const float lift = 6.0f + 3.0f * std::sin(elapsed * 19.0f + x * 0.08f);
                    const Vector2 a{x, w.y + 1};
                    const Vector2 b{x + 6, w.y - lift};
                    const Vector2 c{x + 10, w.y - 1};
                    const Vector2 d{x + 18, w.y - lift * 0.6f};
                    DrawLineEx(a, b, 2, spark);
                    DrawLineEx(b, c, 2, spark);
                    DrawLineEx(c, d, 2, spark);
                }
            }
        }

        void DrawCloud(Rectangle p) {
            const Color shade{172, 200, 229, 255};
            const Color light{244, 249, 255, 255};
            DrawRectangleRounded({p.x, p.y, p.width, p.height + 10}, 0.8f, 8, shade);
            for (int i = 0; i < 5; ++i) {
                const float x = p.x + p.width * (0.12f + 0.19f * i);
                DrawEllipse(static_cast<int>(x), static_cast<int>(p.y + 10),
                            p.width * 0.15f, 10 + (i % 2) * 3, light);
            }
            DrawRectangleRec({p.x + 8, p.y, p.width - 16, 3}, light);
        }

        // ---- Every visible thing in Level 1 is an object, so a sprite can replace its
        // drawing. Their Asset IDs are assigned in level_1_sprites.cpp; until a thing has a
        // Game Data row, its original drawing (its stand-in, see SetUpStandIns) is used.
        constexpr std::array<int, 5> lightXs{780, 1170, 1560, 1950, 2340};
        constexpr std::array<const char*, 5> lightIds{
            "traffic-light-1", "traffic-light-2", "traffic-light-3", "traffic-light-4", "traffic-light-5"};
        constexpr std::array<Rectangle, 5> carStart{{
            {760.0f, 1428.0f, 108.0f, 52.0f},
            {1240.0f, 1428.0f, 116.0f, 52.0f},
            {1710.0f, 1428.0f, 120.0f, 52.0f},
            {2330.0f, floorY - 48, 104, 48},
            {tunnelSpawnX, floorY - 38, 86, 38}
        }};
        constexpr std::array<const char*, 5> carIds{"car-1", "car-2", "car-3", "car-4", "car-5"};
        constexpr std::array<const char*, 3> wreckCarIds{"wreck-car-1", "wreck-car-2", "wreck-car-3"};
        constexpr std::array<const char*, 3> cableIds{"cable-1", "cable-2", "cable-3"}; // Drawn over wire-1 to wire-3.
        constexpr std::size_t gasSlots = 16; // Puffs on screen at once; extra puffs are not drawn.
        constexpr std::size_t pigSlots = 5;  // UpdatePigs never keeps more than 5.
        constexpr std::size_t starCount = 9; // Level1::stars.
        constexpr Color rainbowColors[] = {
            {247, 110, 117, 255}, {249, 164, 99, 255}, {252, 216, 111, 255},
            {136, 216, 146, 255}, {122, 190, 247, 255}, {176, 151, 222, 255}
        };
        constexpr Color gateTrim{43, 53, 78, 255};
        constexpr Color scrapColors[] = {{112, 115, 132, 255}, {132, 107, 111, 255}, {103, 125, 139, 255}};

        std::string NumberedId(const char* prefix, std::size_t index) {
            return std::string(prefix) + std::to_string(index + 1);
        }

        // The wreck pile: smaller scrap cars stacked under each roof, in drawing order.
        struct ScrapPiece {
            std::size_t roof;
            int layer;
            float y;
        };
        const std::vector<ScrapPiece>& ScrapPieces() {
            static const std::vector<ScrapPiece> pieces = [] {
                std::vector<ScrapPiece> list;
                for (std::size_t i = 0; i < wreckSteps.size(); ++i) {
                    int layer = 0;
                    for (float y = floorY - 50; y > wreckSteps[i].y + 34; y -= 42, ++layer) list.push_back({i, layer, y});
                }
                return list;
            }();
            return pieces;
        }
        Rectangle ScrapSlot(const ScrapPiece& piece) {
            const Rectangle roof = wreckSteps[piece.roof];
            const float shift = piece.layer % 2 == 0 ? -3.0f : 3.0f;
            return {roof.x - 22 + shift, piece.y, roof.width + 44, 50};
        }
        Rectangle WreckSlot(std::size_t roof) {
            return {wreckSteps[roof].x - 22, wreckSteps[roof].y, wreckSteps[roof].width + 44, 54};
        }
        // The slide's area: the curve from SlidePoint, plus the stripes' width.
        Rectangle SlideArea() {
            return {slideStart.x - 3, slideStart.y + 40, slideEnd.x - slideStart.x + 43, slideEnd.y - slideStart.y + 8};
        }
        Rectangle ChickenBounds(float chickenX) { return {chickenX - 13, 1439, 50, 41}; }
        Rectangle ChickenSignBounds(float chickenX) { return {chickenX - 61, 1405, 146, 33}; }

        void DrawFlyingPig(Rectangle pig, float time) {
            rlPushMatrix();
            rlTranslatef(pig.x, pig.y, 0);
            rlScalef(pigScale, pigScale, 1);
            const float flap = std::sin(time * 17.0f) * 5.0f;
            DrawEllipse(49, 21, 34, 16, {237, 154, 174, 255});
            DrawCircle(20, 18, 15, {244, 170, 188, 255});
            DrawEllipse(7, 23, 8, 5, {212, 115, 144, 255});
            DrawCircle(17, 15, 2, BLACK);
            DrawTriangle({13, 8}, {25, 8}, {12, 0}, PINK);
            DrawEllipse(47, static_cast<int>(9 + flap), 19, 9, RAYWHITE);
            DrawLineEx({81, 19}, {92, 11}, 1.0f, PINK);
            rlPopMatrix();
        }

        // The things that are only drawn, added after the collision objects (which draw
        // nothing) in the order they are drawn: later ones in front. Effects start hidden.
        std::vector<GameObjectDefinition> WithArtObjects(std::vector<GameObjectDefinition> objects) {
            const std::vector<GameObjectDefinition> solid = objects; // The collision objects.
            const auto add = [&objects](const std::string& id, Rectangle box, bool visible = true) {
                auto object = ArtObject(id.c_str(), box, "");
                object.visible = visible;
                objects.push_back(object);
            };
            // Street level, back to front.
            add("road", {roadStart, floorY, tunnelEndX - roadStart, 120});
            add("underpass-road", {roadStart, floorY - 12, rampStartX - roadStart, 132});
            add("tunnel-back", {tunnelEntranceX - 26, floorY - 103, 70, 103});
            add("sign-school", {494, 1342, 82, floorY - 1342});
            add("sign-wrong-way", {640, 1350, 100, floorY - 1350});
            add("sign-duck", {4745, 646, 112, 22});
            add("trash-can", trashCan);
            add("hydrant", hydrant);
            for (std::size_t i = 0; i < lightXs.size(); ++i)
                add(lightIds[i], {lightXs[i] - 13.0f, 1290, 33, floorY - 1290});
            for (std::size_t i = 0; i < carIds.size(); ++i) add(carIds[i], carStart[i]);
            // In front of the cars, so they disappear under the sidewalk and into the tunnel.
            add("sidewalk", {0, floorY - 5, roadStart, 125});
            add("tunnel-front", {tunnelEntranceX - 24, floorY - 135, tunnelEndX - tunnelEntranceX + 24, 135});
            add("chicken", ChickenBounds(tunnelEntranceX - 280));
            add("sign-chicken", ChickenSignBounds(tunnelEntranceX - 280), false);
            // The crash: each roof's scrap cars, then the roof car, then the blast.
            for (std::size_t i = 0, k = 0; i < wreckSteps.size(); ++i) {
                for (; k < ScrapPieces().size() && ScrapPieces()[k].roof == i; ++k)
                    add(NumberedId("scrap-car-", k), ScrapSlot(ScrapPieces()[k]), false);
                add(wreckCarIds[i], WreckSlot(i), false);
            }
            add("crash-blast", {0, 0, 280, 280}, false);
            add("road-edge-wall", roadEdgeWall);
            add("spring-floor", roadSpringArt, false);
            // The climb and the sky, on the collision rectangles they belong to.
            const auto box = [](const GameObjectDefinition& o) { return Rectangle{o.position.x, o.position.y, o.size.x, o.size.y}; };
            std::size_t poles = 0;
            for (const auto& o : solid)
                if (o.id.rfind("telephone-step-", 0) == 0 || o.id == "sky-checkpoint-ledge")
                    add(NumberedId("utility-pole-", poles++), box(o));
            for (std::size_t i = 0; i <= wires.size(); ++i) add(NumberedId("wire-mast-", i), WireMastBounds(i));
            for (std::size_t i = 0; i < wires.size(); ++i) add(cableIds[i], wires[i]);
            // Cloud refuges above the wires, then the clouds; the flag stands on the first cloud.
            std::size_t clouds = 0;
            for (const auto& o : solid)
                if (o.id.rfind("wire-refuge-", 0) == 0) add(NumberedId("sky-cloud-", clouds++), box(o));
            for (const auto& o : solid) {
                if (o.id.rfind("cloud-", 0) != 0 && o.id != "rainbow-entrance") continue;
                add(NumberedId("sky-cloud-", clouds++), box(o));
                if (o.id == "cloud-1") add("checkpoint-flag", {o.position.x + 4, o.position.y - 27, 24, 27});
            }
            for (std::size_t i = 0; i < gasSlots; ++i) add(NumberedId("gas-", i), {0, 0, 186, 161}, false);
            for (std::size_t i = 0; i < pigSlots; ++i) add(NumberedId("pig-", i), {0, 0, 94 * pigScale, 38 * pigScale}, false);
            for (std::size_t i = 0; i < starCount; ++i) {
                add(NumberedId("star-warning-", i), {0, 0, 28, worldViewport.height / 2 - 2}, false);
                add(NumberedId("falling-star-", i), {0, 0, 34, 82}, false);
            }
            // The rainbow and the campus.
            add("rainbow-slide", SlideArea());
            add("rainbow-gate", {rainbowGate.x - 2, rainbowGate.y - 2, 12, rainbowGate.height + 4});
            add("sign-rainbow", {rainbowGate.x + 6, rainbowEntrance.y - 38, 132, 24});
            add("campus-path", {6880, floorY, 325, 120});
            add("lecture-hall", {6975, 1245, 250, floorY - 1245});
            add("anvil", {7049, 1200, 124, 44}, false);
            add("anvil-poof", {7019, 1160, 184, 84}, false);
            return objects;
        }
    }

    const LevelDefinition& Level1Definition() {
        static const LevelDefinition definition{
            .id = LevelId::Level1,
            .title = "Level 1",
            .description = "Sidewalk -> road -> telephone poles -> clouds -> rainbow -> campus",
            .plannedContent = "Jump sidewalk obstacles, dodge right-to-left traffic and its chicken pile-up, "
                              "dodge birds on the poles, cross live wires, duck flying pigs (hold S or Shift), "
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
            .world = {0.0f, 0.0f, 7205.0f, 1600.0f},
            .spawn = {100.0f, 1436.0f},
            // The merged GameObject system replaces LevelDefinition::platforms.
            // Authored collision geometry is independent of the procedural artwork.
            // SolidPlatform and ArtObject are shared shortcuts (objects/game_object.h).
            // Level 1 draws its own platforms, so they pass "" (no sprite).
            .objects = WithArtObjects({
                SolidPlatform("sidewalk-road", {0, floorY, tunnelEndX, 120}, ""),
                {.id = roadSpringId, .type = ObjectType::Trigger, .collision = CollisionType::Trigger,
                 .position = {tunnelEndX - roadSpringWidth, floorY - 1}, .size = {roadSpringWidth, 1}},
                SolidPlatform(wreckIds[0], wreckSteps[0], "", false),
                SolidPlatform(wreckIds[1], wreckSteps[1], "", false),
                SolidPlatform(wreckIds[2], wreckSteps[2], "", false),
                SolidPlatform("telephone-step-1", {3172, 1148, 52, 18}, ""),
                SolidPlatform("telephone-step-2", {3309, 1070, 52, 18}, ""),
                SolidPlatform("telephone-step-3", {3448, 992, 52, 18}, ""),
                SolidPlatform("telephone-step-4", {3588, 914, 52, 18}, ""),
                SolidPlatform("sky-checkpoint-ledge", {3718, 832, 100, 18}, ""),
                SolidPlatform("wire-1", wires[0], ""),
                SolidPlatform("wire-2", wires[1], ""),
                SolidPlatform("wire-3", wires[2], ""),
                CloudPlatform("wire-refuge-1", refuges[0]),
                CloudPlatform("wire-refuge-2", refuges[1]),
                CloudPlatform("wire-refuge-3", refuges[2]),
                CloudPlatform("cloud-1", firstCloud),
                CloudPlatform("cloud-2", {4950, 715, 155, 22}),
                CloudPlatform("cloud-3", {5215, 665, 150, 22}),
                CloudPlatform("cloud-4", {5500, 755, 160, 22}),
                CloudPlatform("cloud-5", {5770, 700, 155, 22}),
                CloudPlatform("cloud-6", {6050, 645, 160, 22}),
                CloudPlatform("cloud-7", {6335, 710, 155, 22}),
                // Stagger the upper and lower landings across the middle route's gaps.
                CloudPlatform("cloud-upper-1", {4810, 600, 175, 22}),
                CloudPlatform("cloud-upper-2", {5065, 535, 175, 22}),
                CloudPlatform("cloud-upper-3", {5350, 550, 175, 22}),
                CloudPlatform("cloud-upper-4", {5635, 590, 180, 22}),
                CloudPlatform("cloud-upper-5", {5920, 565, 180, 22}),
                CloudPlatform("cloud-upper-6", {6200, 510, 175, 22}),
                CloudPlatform("cloud-upper-7", {6480, 600, 155, 22}),
                CloudPlatform("cloud-lower-1", {4840, 820, 200, 22}),
                CloudPlatform("cloud-lower-2", {5110, 850, 190, 22}),
                CloudPlatform("cloud-lower-3", {5390, 800, 195, 22}),
                CloudPlatform("cloud-lower-4", {5680, 840, 195, 22}),
                CloudPlatform("cloud-lower-5", {5950, 815, 200, 22}),
                CloudPlatform("cloud-lower-6", {6240, 810, 195, 22}),
                CloudPlatform("cloud-lower-7", {6510, 855, 145, 22}),
                CloudPlatform("rainbow-entrance", rainbowEntrance),
                SolidPlatform("campus-sidewalk", {6880, floorY, 325, 120}, ""),
                {.id = anvilId, .type = ObjectType::Moving, .collision = CollisionType::None,
                 .position = {7049, 1200}, .size = {124, 44}, .velocity = {0, 660},
                 .range = {0, floorY - 44 - 1200}, .falls = true, .active = false},
                {.id = exitId, .type = ObjectType::Trigger, .collision = CollisionType::Trigger,
                 .position = {7080, 1375}, .size = {75, 105}, .active = false, .triggerOnce = true}
            }),
            // Asset IDs for the objects above: level_1_sprites.cpp.
            .sprites = Level1Sprites()
        };
        return definition;
    }

    Level1::Level1() : Level(Level1Definition()), birds{
        Bird(PoleFlight{{3240, 1120}, 2.2f, 0.4f}),
        Bird(PoleFlight{{3415, 1010}, 1.8f, 2.1f}),
        Bird(PoleFlight{{3610, 882}, 2.5f, 4.6f})
    } {
        for (auto& event : CreateEvents()) Events().RegisterEvent(std::move(event));
        SetUpStandIns();
    }

    void Level1::OnTrigger(GameObject& object) {
        if (object.Id() == roadSpringId) {
            roadSpringEndsAt = ElapsedSeconds() + roadSpringDuration;
            MutablePlayer().Bounce({760.0f, -400.0f});
            return;
        }
        Level::OnTrigger(object);
    }

    void Level1::ResetTraffic() {
        traffic = {{
            {carStart[0], -195.0f, {223, 96, 91, 255}},
            {carStart[1], -165.0f, {240, 189, 88, 255}},
            {carStart[2], -225.0f, {141, 122, 207, 255}},
            {carStart[3], -210.0f, {93, 180, 198, 255}},
            {carStart[4], 0.0f, {150, 236, 78, 255}}
        }};
        speederDelay = GetRandomValue(15, 110) / 100.0f;
        chickenX = tunnelEntranceX - 280;
        pileupSeconds = 0.0f;
        pileupStarted = false;
        pileupSettled = false;
        crashImpact = false;
    }

    void Level1::OnReset() {
        timer.Reset();
        nextHazardId = 1;
        ResetTraffic();
        for (auto& bird : birds) bird.Reset();
        birdCooldown = 0;
        ResetStars();
        pigs.clear();
        gasPuffs.clear();
        pigSpawnSeconds = 1.0f;
        checkpoint = Definition().spawn;
        collisionCooldown = 0.0f;
        roadSpringEndsAt = 0.0f;
        slideProgress = 0.0f;
        collisionCount = 0;
        roadCheckpointReached = false;
        skyCheckpointReached = false;
        sliding = false;
        slideFinished = false;
        campusReached = false;
        SyncObjects();
    }

    void Level1::Respawn(const char* notification) {
        if (pileupStarted && !crashImpact) ResetTraffic();
        MutablePlayer().Reset(checkpoint);
        collisionCooldown = collisionGraceSeconds;
        ResetStars();
        pigs.clear();
        gasPuffs.clear();
        pigSpawnSeconds = 1.0f;
        RequestSound(playerSounds::death);
        ++collisionCount;
        Events().Notify({notification, static_cast<std::uint64_t>(collisionCount)});
        timer.Reset(60.0f, false); // Reset the countdown without repeating the entrance on each death.
    }

    void Level1::ResetStars() {
        stars = {{{3970, 3850}, {4205, 4090}, {4400, 4300}, {4550, 4415},
                  {4810, 4695}, {5320, 5200}, {5590, 5470}, {6160, 6040}, {6440, 6320}}};
    }

    void Level1::UpdatePigs(float dt) {
        const Rectangle player = GetPlayer().Bounds();
        const float halfView = worldViewport.width / (2 * Camera().zoom);
        const float playerCenter = player.x + player.width / 2;
        for (auto& puff : gasPuffs) {
            puff.age += dt;
            puff.center.x += 10 * dt;
            puff.center.y -= 4 * dt;
        }
        std::erase_if(gasPuffs, [](const GasPuff& puff) { return puff.age >= gasLifetime; });
        for (auto& pig : pigs) {
            pig.steerSeconds -= dt;
            if (pig.steerSeconds <= 0) {
                pig.steerSeconds = GetRandomValue(45, 95) / 100.0f;
                const bool dart = GetRandomValue(0, 3) == 0;
                pig.speed = static_cast<float>(dart ? GetRandomValue(245, 300) : GetRandomValue(135, 195));
            }
            pig.bounds.x -= pig.speed * dt;
            pig.wavePhase += pig.waveRate * dt;
            pig.bounds.y = pigRouteY[pig.route] + pig.waveAmplitude * std::sin(pig.wavePhase);
            pig.fartSeconds -= dt;
            const Vector2 tail{pig.bounds.x + 89 * pigScale, pig.bounds.y + 20 * pigScale};
            if (pig.fartSeconds <= 0 && InCloudSection(tail.x)) {
                gasPuffs.push_back({tail, 0, nextHazardId++});
                pig.fartSeconds = GetRandomValue(100, 165) / 100.0f;
            }
        }
        const float leftEdge = std::min(Camera().target.x, playerCenter) - halfView;
        std::erase_if(pigs, [leftEdge](const FlyingPig& pig) {
            return pig.bounds.x + pig.bounds.width < leftEdge - 32;
        });
        if (sliding || slideFinished || !InCloudSection(player.x)) return;
        // Let existing pigs fly past and their gas expire while the player waits or retreats.
        if (GetPlayer().Velocity().x <= 0) {
            pigSpawnSeconds = std::max(pigSpawnSeconds, 1.0f);
            return;
        }
        pigSpawnSeconds -= dt;
        if (pigSpawnSeconds > 0 || pigs.size() >= 5) return;

        // The camera follows the player after this update. Clear both right edges.
        const float spawnX = std::max(Camera().target.x, playerCenter) + halfView + 32;
        const int routeCount = static_cast<int>(pigRouteY.size());
        const int route = pigs.empty() ? GetRandomValue(0, routeCount - 1)
            : (pigs.back().route + GetRandomValue(1, routeCount - 1)) % routeCount;
        const float amplitude = static_cast<float>(GetRandomValue(12, 42));
        const float phase = GetRandomValue(0, 359) * DEG2RAD;
        const float entryY = pigRouteY[route] + amplitude * std::sin(phase);
        pigs.push_back({.bounds = {spawnX, entryY, 94 * pigScale, 38 * pigScale}, .route = route,
                        .waveAmplitude = amplitude, .waveRate = GetRandomValue(90, 210) / 100.0f,
                        .wavePhase = phase, .fartSeconds = GetRandomValue(70, 130) / 100.0f, .id = nextHazardId++});
        pigSpawnSeconds = GetRandomValue(100, 150) / 100.0f;
    }

    void Level1::UpdateCampus() {
        const Rectangle body = GetPlayer().Bounds();
        auto* anvil = Objects().Get(anvilId);
        if (!anvil->IsActive() && !anvil->IsTriggered() && body.x + body.width >= anvil->Position().x - 45) {
            ActivateObject(anvilId);
            StartObjectFall(anvilId);
        }
        if (anvil->IsActive() && !Objects().IsActive(exitId) && !IsComplete()) {
            if (ScriptContact(anvilId, anvil->CollisionBounds())) {
                Events().Notify({"level1.anvil_hit"});
                return;
            }
            if (anvil->IsTriggered()) {
                anvilLandedAt = ElapsedSeconds();
                DeactivateObject(anvilId);
                ActivateObject(exitId);
            }
        }
        if (ScriptContact("campus-fall", {0, 1564, 7200, 1000})) Respawn("level1.fall_hit");
    }

    void Level1::OnUpdate(float dt, const InputFrame& input) {
        UpdateScript(dt, input);
        SyncObjects(); // After the script, whichever way it returned, so sprites never lag.
    }

    void Level1::SyncObjects() {
        // Keeps each drawn-only object on the thing this script moves, and shows it only
        // while that thing is on screen, the same as the original drawing.
        const auto object = [this](const std::string& id) { return Objects().Get(id); };
        const auto show = [&](const std::string& id, bool visible, Rectangle box) {
            if (auto* o = object(id)) {
                o->SetVisible(visible);
                o->SetPosition({box.x, box.y});
            }
        };
        // For things whose drawn size changes (wrecks in flight, puffs), the object's box
        // keeps its size and stays centred on them.
        const auto showCentred = [&](const std::string& id, bool visible, Rectangle area) {
            if (auto* o = object(id)) {
                const Vector2 size = o->Size();
                show(id, visible, {area.x + area.width / 2 - size.x / 2, area.y + area.height / 2 - size.y / 2,
                                   size.x, size.y});
            }
        };
        const float time = ElapsedSeconds();

        if (auto* spring = object("spring-floor")) {
            spring->SetVisible(time < roadSpringEndsAt);
            if (spring->IsVisible()) spring->SetAnimationTime(roadSpringDuration - (roadSpringEndsAt - time));
        }
        for (std::size_t i = 0; i < lightXs.size(); ++i)
            if (auto* light = object(lightIds[i])) light->SetState(LightGreen(lightXs[i]) ? "green" : "red");
        // While the two crash cars approach, only they are drawn ("car_crash" when it exists).
        const bool crashApproach = pileupStarted && pileupSeconds >= 0;
        for (std::size_t i = 0; i < traffic.size(); ++i) {
            const bool crashCar = crashApproach && (i == 1 || i == 2);
            show(carIds[i], !crashImpact && (!crashApproach || crashCar), traffic[i].bounds);
            if (auto* car = object(carIds[i])) car->SetState(crashCar ? "crash" : "idle");
        }
        show("chicken", !pileupSettled, ChickenBounds(chickenX));
        show("sign-chicken", !pileupSettled && pileupStarted && !crashImpact, ChickenSignBounds(chickenX));
        for (std::size_t k = 0; k < ScrapPieces().size(); ++k) {
            float progress = 0;
            const Rectangle body = ScrapBody(k, &progress);
            showCentred(NumberedId("scrap-car-", k), crashImpact && progress > 0, body);
        }
        for (std::size_t i = 0; i < wreckCarIds.size(); ++i) {
            const Rectangle body = WreckBody(i);
            showCentred(wreckCarIds[i], crashImpact && body.width > 0, body);
        }
        const Blast blast = BlastAt(CrashImpact(), pileupSeconds);
        showCentred("crash-blast", crashImpact && pileupSeconds < blastDuration, {blast.center.x, blast.center.y, 0, 0});
        if (auto* o = object("crash-blast")) o->SetAnimationTime(std::max(0.0f, pileupSeconds));

        for (std::size_t i = 0; i < wires.size(); ++i) {
            const int w = static_cast<int>(i);
            if (auto* wire = object(cableIds[i]))
                wire->SetState(WireLive(w, time) ? "live" : WireWarning(w, time) ? "warning" : "idle");
        }
        if (auto* flag = object("checkpoint-flag")) flag->SetState(checkpoint.x >= firstCloud.x ? "reached" : "idle");
        for (std::size_t k = 0; k < gasSlots; ++k) {
            const bool used = k < gasPuffs.size();
            const Vector2 center = used ? gasPuffs[k].center : Vector2{};
            showCentred(NumberedId("gas-", k), used, {center.x, center.y, 0, 0});
            if (auto* o = object(NumberedId("gas-", k)); o && used) o->SetAnimationTime(gasPuffs[k].age);
        }
        for (std::size_t k = 0; k < pigSlots; ++k)
            show(NumberedId("pig-", k), k < pigs.size(), k < pigs.size() ? pigs[k].bounds : Rectangle{});
        const Rectangle player = GetPlayer().Bounds();
        const float halfView = worldViewport.height / (2 * Camera().zoom);
        const float viewTop = Camera().target.y - halfView + 16;
        for (std::size_t k = 0; k < stars.size(); ++k) {
            const auto& star = stars[k];
            const bool warning = star.warned && !star.falling && !star.finished
                && InCloudSection(star.x) && InCloudSection(player.x);
            show(NumberedId("star-warning-", k), warning, {star.x - 14, viewTop - 14, 28, 0});
            show(NumberedId("falling-star-", k), star.falling, {star.x - 17, star.y - 65, 34, 82});
        }
        if (const auto* anvil = Objects().Get(anvilId)) {
            const Rectangle b = anvil->Bounds();
            const float since = ElapsedSeconds() - anvilLandedAt;
            show("anvil", anvil->IsActive(), b);
            show("anvil-poof", !anvil->IsActive() && anvil->IsTriggered() && since < anvilPoofDuration,
                 {b.x - 30, b.y - 40, b.width + 60, b.height + 40});
            if (auto* poof = object("anvil-poof")) poof->SetAnimationTime(std::max(0.0f, since));
        }
    }

    void Level1::UpdateScript(float dt, const InputFrame&) {
        timer.Update(dt);
        for (auto& bird : birds) bird.Update(dt, ElapsedSeconds(), GetPlayer().Bounds());
        dt = std::min(dt, 0.1f); // Match the physics engine maximum step.
        UpdatePigs(dt);
        // Slide motion stays inside the level rather than changing shared movement code.
        if (sliding) {
            slideProgress = std::min(1.0f, slideProgress + dt / slideDuration);
            MutablePlayer().Reset(SlidePoint(slideProgress));
            if (slideProgress >= 1.0f) {
                sliding = false;
                slideFinished = true;
                checkpoint = slideEnd;
            }
            return;
        }
        if (slideFinished) {
            UpdateCampus();
            return;
        }
        collisionCooldown = std::max(0.0f, collisionCooldown - dt);
        const Vector2 position = GetPlayer().Position();
        // Once the chicken event is requested, let ordinary traffic finish
        // its current trip. Park each vehicle only after it is fully concealed.
        // A negative pileup timer is the staging phase; zero begins the crash.
        if (!pileupStarted && position.x >= tunnelEntranceX - 450) {
            pileupStarted = true;
            pileupSeconds = -1.0f;
        }
        const auto parkAfterExit = [&](Vehicle& car, float parkingX) {
            if (car.speed == 0.0f) return;
            if (car.bounds.x < tunnelEntranceX) {
                MoveRoadCar(car.bounds, car.speed, dt, parkingX);
            }
            if (car.bounds.x >= tunnelEntranceX) {
                car.bounds.x = parkingX;
                car.bounds.y = floorY - car.bounds.height;
                car.speed = 0.0f;
            }
        };
        bool carsMet = false;
        if (!pileupStarted) {
            auto& speeder = traffic.back();
            if (speeder.speed == 0 && position.x >= roadStart) {
                speederDelay -= dt;
                if (speederDelay <= 0)
                    speeder.speed = -static_cast<float>(GetRandomValue(520, 760));
            }
            for (auto& car : traffic) {
                if (MoveRoadCar(car.bounds, car.speed, dt, tunnelSpawnX) && &car == &speeder) {
                    speeder.speed = 0;
                    speederDelay = GetRandomValue(60, 240) / 100.0f;
                }
            }
        } else if (pileupSeconds < 0.0f) {
            for (auto& car : traffic) parkAfterExit(car, tunnelSpawnX);
            chickenX = std::max(tunnelEntranceX - 320, chickenX - 13.0f * dt);
            if (std::all_of(traffic.begin(), traffic.end(), [](const Vehicle& car) { return car.speed == 0; }))
                pileupSeconds = 0.0f;
        } else if (!pileupSettled) {
            chickenX = std::max(tunnelEntranceX - 320, chickenX - 13.0f * dt);
            const float brakeX = chickenX + 58.0f;
            auto& lead = traffic[1].bounds;
            auto& follower = traffic[2].bounds;
            if (!crashImpact) {
                const float brakingSpeed = std::clamp((lead.x - brakeX) * 2.8f, 42.0f, 230.0f);
                lead.x = std::max(brakeX, lead.x - brakingSpeed * dt);
                // Release the follower from the tunnel as the lead car finishes braking.
                if (lead.x - brakeX < 32)
                    follower.x = std::max(lead.x + lead.width, follower.x - 345.0f * dt);
                carsMet = lead.x <= brakeX + 1.0f && follower.x <= lead.x + lead.width + 1.0f;
            } else {
                pileupSeconds += dt;
                if (pileupSeconds >= pileupDuration) {
                    pileupSettled = true;
                    for (const auto* id : wreckIds) ActivateObject(id);
                }
            }
        }
        for (auto& star : stars) {
            if (star.finished) continue;
            if (!star.warned && position.x >= star.triggerX && position.x < star.x + 170.0f
                && position.y < 1000.0f) {
                star.warned = true;
                star.warningSeconds = InCloudSection(star.x) ? cloudStarWarning : starWarning;
                continue;
            }
            if (star.warned && !star.falling) {
                star.warningSeconds -= dt;
                if (star.warningSeconds <= 0.0f) {
                    const float halfView = worldViewport.height / (2 * Camera().zoom);
                    const float top = std::max(Definition().world.y, std::min(Camera().target.y, position.y) - halfView);
                    star.y = top - 32;
                    star.falling = true;
                }
            } else if (star.falling) {
                star.y += 720.0f * dt;
                if (star.y >= 1600.0f) {
                    star.falling = false;
                    star.finished = true;
                }
            }
        }
        if (ScriptContact("rainbow-slide", rainbowGate, CollisionType::Trigger)) {
            sliding = true;
            slideProgress = 0.0f;
            MutablePlayer().Reset(slideStart);
            Events().Notify({"level1.slide_started"});
            return;
        }
        if (collisionCooldown == 0.0f) CheckHazards(dt);
        // A fatal hit on the impact frame cancels the chicken trigger in Respawn.
        if (carsMet && pileupStarted) {
            crashImpact = true;
            pileupSeconds = 0;
        }
    }

    void Level1::CheckHazards(float dt) {
        Rectangle player = GetPlayer().Bounds();
        birdCooldown = std::max(0.0f, birdCooldown - dt);
        for (const auto& bird : birds) {
            const Rectangle b = bird.Bounds();
            if (ScriptContact("pole-bird-" + std::to_string(&bird - birds.data()), b, CollisionType::Trigger, birdCooldown == 0, true)) {
                const bool left = player.x + player.width / 2 < b.x + b.width / 2;
                MutablePlayer().Bounce({left ? -390.0f : 390.0f, -315.0f});
                birdCooldown = 0.6f;
                break;
            }
        }
        // Sidewalk: two stationary objects to jump over.
        const bool trashHit = ScriptContact("trash-can", trashCan);
        const bool hydrantHit = ScriptContact("hydrant", hydrant);
        if (trashHit || hydrantHit) {
            Respawn("level1.obstacle_hit");
            return;
        }
        // Cars approach from the RIGHT and move LEFT across the roadway.
        if (!crashImpact) {
            for (const auto& car : traffic) {
                const Rectangle visibleCar = VisibleRoadCar(car.bounds);
                if (ScriptContact("traffic-" + std::to_string(&car - traffic.data()), visibleCar, CollisionType::Hazard, visibleCar.width > 0)) {
                    Respawn("level1.traffic_hit");
                    return;
                }
            }
        }
        // Wire collision is a grounded contact, not simply passing beneath the wire.
        for (std::size_t i = 0; i < wires.size(); ++i) {
            if (!WireLive(static_cast<int>(i), ElapsedSeconds())) continue;
            const Rectangle wire = wires[i];
            const bool standingOnWire = ScriptContact("live-wire-" + std::to_string(i),
                {wire.x, wire.y - 4, wire.width, 4}, CollisionType::Hazard, GetPlayer().Grounded());
            if (standingOnWire) {
                Respawn("level1.wire_hit");
                return;
            }
        }
        for (const auto& pig : pigs) {
            const Rectangle body{pig.bounds.x + 5 * pigScale, pig.bounds.y + 6 * pigScale,
                                 78 * pigScale, 29 * pigScale};
            if (ScriptContact("pig-" + std::to_string(pig.id), body)) {
                Respawn("level1.pig_hit");
                return;
            }
        }
        for (const auto& puff : gasPuffs) {
            const float radius = GasRadius(puff.age);
            if (ScriptContact("pig-gas-" + std::to_string(puff.id),
                {puff.center.x - radius, puff.center.y - radius, 2 * radius, 2 * radius})) {
                Respawn("level1.pig_gas_hit");
                return;
            }
        }
        for (const auto& star : stars) {
            if (ScriptContact("star-" + std::to_string(&star - stars.data()),
                {star.x - starHitWidth / 2, star.y - 18.0f, starHitWidth, 38.0f}, CollisionType::Hazard, star.falling)) {
                Respawn("level1.star_hit");
                return;
            }
        }
        // No collision floor exists beneath the suspended sky section.
        if (ScriptContact("sky-fall", {0, 1564, 7200, 1000})) {
            Respawn("level1.fall_hit");
        }
    }

    bool Level1::LightGreen(int x) const {
        // Traffic lights do not perfectly agree with the approaching cars.
        const bool carNear = !crashImpact && std::any_of(traffic.begin(), traffic.end(), [x](const Vehicle& car) {
            const Rectangle visible = VisibleRoadCar(car.bounds);
            const float distance = visible.x - x;
            return visible.width > 0 && distance > -110 && distance < 230;
        });
        return SignalGreen(static_cast<float>(x), ElapsedSeconds(), carNear);
    }

    void Level1::SubmitSprites(RenderQueue& queue, const Resources& resources) const {
        Level::SubmitSprites(queue, resources);
        for (const auto& bird : birds) bird.SubmitSprites(queue, resources.Assets(), Camera().zoom);
    }

    Vector2 Level1::CrashImpact() const {
        const auto& follower = traffic[2].bounds;
        return {follower.x, follower.y + follower.height * 0.45f};
    }

    // A smooth horizontal throw and a vertical arc share exact endpoints with the
    // resting artwork. The flash hides the launch of the smaller scrap cars.
    Rectangle Level1::ScrapBody(std::size_t piece, float* progress) const {
        const ScrapPiece& p = ScrapPieces()[piece];
        const float elapsed = pileupSeconds - p.roof * wreckStagger;
        // The low layers land first, underneath the car that forms the roof.
        const float supportT = std::clamp(elapsed / (0.75f + p.layer * 0.09f), 0.0f, 1.0f);
        if (progress) *progress = supportT;
        const Vector2 impact = CrashImpact();
        return WreckFlight({impact.x, impact.y, 0, 0}, ScrapSlot(p), supportT, 35 + 9 * p.layer);
    }

    Rectangle Level1::WreckBody(std::size_t roof, float* progress) const {
        const float elapsed = pileupSeconds - roof * wreckStagger;
        const float t = std::clamp(elapsed / wreckFlightSeconds, 0.0f, 1.0f);
        if (progress) *progress = t;
        const Vector2 impact = CrashImpact();
        const auto& car = traffic[(roof + 1) % wreckSteps.size()];
        const Rectangle source = roof < 2 ? car.bounds : Rectangle{impact.x, impact.y, 0, 0};
        return WreckFlight(source, WreckSlot(roof), t, 80 + 18 * roof);
    }

    void Level1::SetUpStandIns() {
        // Each thing's own drawing, used until its Asset ID has a Game Data row. Objects
        // draw in the order they are created (WithArtObjects), so the layering is unchanged.
        static_assert(std::tuple_size_v<decltype(stars)> == starCount);
        const auto stand = [this](const std::string& id, std::function<void(Rectangle)> draw) {
            SetStandIn(id, std::move(draw));
        };

        // An ordinary sidewalk abruptly meets an oncoming carriageway.
        stand("road", [](Rectangle) {
            DrawRectangleRec({roadStart, floorY, tunnelEndX - roadStart, 120}, {49, 55, 70, 255});
            DrawRectangleRec({roadStart, floorY, tunnelEndX - roadStart, 5}, {105, 113, 126, 255});
            for (int x = 660; x < tunnelEndX; x += 150)
                DrawRectangleRec({static_cast<float>(x), 1528, 85, 5}, {241, 216, 130, 255});
        });
        stand("underpass-road", [](Rectangle) { DrawUnderpassRoad(); });
        stand("tunnel-back", [](Rectangle) { DrawTunnelBackdrop(); });
        stand("sign-school", [](Rectangle) {
            DrawStreetPost(534.5f, 1378, 5);
            DrawRectangleRounded({494, 1342, 82, 36}, 0.15f, 4, {57, 110, 142, 255});
        });
        stand("sign-wrong-way", [](Rectangle) {
            DrawStreetPost(690, 1380, 5);
            DrawRectangleRounded({640, 1350, 100, 30}, 0.15f, 4, {176, 66, 70, 255});
        });
        stand("sign-duck", [](Rectangle) {
            DrawRectangleRounded({4745, 646, 112, 22}, 0.2f, 4, {43, 59, 79, 235});
        });
        // Sidewalk hazards: clear silhouettes and consistent collision bounds.
        stand("trash-can", [](Rectangle) {
            DrawRectangleRounded(trashCan, 0.16f, 6, {69, 116, 109, 255});
            for (int x = 270; x <= 296; x += 8)
                DrawRectangleRec({static_cast<float>(x), 1435, 3, 39}, {108, 151, 142, 255});
            DrawRectangleRounded({255, 1423, 56, 9}, 0.4f, 6, {126, 165, 154, 255});
            DrawRectangleRec({277, 1419, 12, 5}, {69, 116, 109, 255});
        });
        stand("hydrant", [](Rectangle) {
            DrawRectangleRounded(hydrant, 0.35f, 6, {201, 68, 80, 255});
            DrawCircle(464, 1439, 14, {236, 106, 106, 255});
            DrawCircle(450, 1454, 7, {234, 105, 109, 255});
            DrawCircle(478, 1454, 7, {234, 105, 109, 255});
            DrawRectangleRec({445, 1474, 38, 6}, {152, 46, 58, 255});
        });
        for (std::size_t i = 0; i < lightXs.size(); ++i) {
            stand(lightIds[i], [this, x = lightXs[i]](Rectangle) {
                DrawStreetPost(x + 3, 1340, 7);
                DrawRectangleRec({static_cast<float>(x) - 13, 1290.0f, 33.0f, 55.0f}, {36, 38, 49, 255});
                const bool green = LightGreen(x);
                DrawCircle(x + 3, 1304, 7, green ? Color{73, 78, 77, 255} : Color{235, 80, 79, 255});
                DrawCircle(x + 3, 1328, 7, green ? Color{112, 227, 144, 255} : Color{67, 73, 65, 255});
            });
        }
        for (std::size_t i = 0; i < carIds.size(); ++i) {
            stand(carIds[i], [this, i](Rectangle) {
                const auto& car = traffic[i];
                if (pileupStarted && pileupSeconds >= 0) DrawCrashCar(car.bounds, car.color, wreckSteps[i - 1].width, 0);
                else DrawCar(car.bounds, car.color);
            });
            Objects().Get(carIds[i])->SetFacing(Facing::Left);
        }
        stand("sidewalk", [](Rectangle) { DrawUnderpassForeground(); });
        stand("tunnel-front", [](Rectangle) { DrawTunnelForeground(); });
        stand("road-edge-wall", DrawRoadEdgeWall);
        stand("spring-floor", [this](Rectangle b) {
            const float remaining = std::clamp((roadSpringEndsAt - ElapsedSeconds()) / roadSpringDuration, 0.0f, 1.0f);
            // A quick extension followed by two progressively smaller rebounds.
            const float lift = roadSpringLift * remaining * remaining * std::abs(std::sin(3 * PI * (1 - remaining)));
            const float center = b.x + b.width / 2;
            const float base = b.y + b.height;
            DrawRectangleRec({b.x, base - roadPlateDepth, b.width, roadPlateDepth}, {34, 38, 47, 255});
            Vector2 coil{center, base};
            for (int i = 1; i <= 6; ++i) {
                const float offset = i == 6 ? 0.0f : (i % 2 ? -9.0f : 9.0f);
                const Vector2 next{center + offset, base - lift * i / 6};
                DrawLineEx(coil, next, 3, {218, 192, 115, 255});
                coil = next;
            }
            const float top = base - lift - roadPlateDepth;
            DrawRectangleRec({b.x, top, b.width, roadPlateDepth}, {49, 55, 70, 255});
            DrawRectangleRec({b.x, top, b.width, 4}, {105, 113, 126, 255});
        });
        // The chicken is scenery, but its presence causes the traffic event.
        stand("chicken", [this](Rectangle) {
            const int cx = static_cast<int>(chickenX);
            DrawLineEx({static_cast<float>(cx + 12), 1473}, {static_cast<float>(cx + 9), 1479}, 1.0f, ORANGE);
            DrawLineEx({static_cast<float>(cx + 20), 1473}, {static_cast<float>(cx + 23), 1479}, 1.0f, ORANGE);
            DrawEllipse(cx + 18, 1465, 17, 11, {250, 245, 225, 255});
            DrawCircle(cx + 4, 1454, 8, {250, 245, 225, 255});
            DrawCircle(cx + 3, 1447, 4, RED);
            DrawCircle(cx + 1, 1452, 1, BLACK);
            DrawTriangle({chickenX - 3, 1452}, {chickenX - 13, 1455}, {chickenX - 3, 1458}, ORANGE);
        });
        Objects().Get("chicken")->SetFacing(Facing::Left);
        stand("sign-chicken", [this](Rectangle) {
            const Color amber{255, 199, 69, 255};
            DrawRectangleRounded({chickenX - 61, 1405, 146, 23}, 0.25f, 4, {51, 45, 49, 255});
            DrawTriangle({chickenX + 4, 1428}, {chickenX + 12, 1438}, {chickenX + 20, 1428}, amber);
        });
        for (std::size_t k = 0; k < ScrapPieces().size(); ++k) {
            stand(NumberedId("scrap-car-", k), [this, k](Rectangle) {
                const ScrapPiece& piece = ScrapPieces()[k];
                const Rectangle slot = ScrapSlot(piece);
                const Rectangle body = ScrapBody(k);
                rlPushMatrix();
                rlTranslatef(body.x, body.y, 0);
                rlScalef(body.width / slot.width, body.height / slot.height, 1);
                DrawCar({0, 0, slot.width, slot.height}, scrapColors[(piece.roof + piece.layer) % 3]);
                rlPopMatrix();
            });
        }
        for (std::size_t i = 0; i < wreckCarIds.size(); ++i) {
            stand(wreckCarIds[i], [this, i](Rectangle) {
                float t = 0;
                const Rectangle body = WreckBody(i, &t);
                const auto& car = traffic[(i + 1) % wreckSteps.size()];
                if (body.width > 0)
                    DrawCrashCar(body, car.color, wreckSteps[i].width, std::min(1.0f, t * 5), -18 * std::sin(PI * t));
            });
        }
        stand("crash-blast", [this](Rectangle) { DrawCrashBlast(BlastAt(CrashImpact(), pileupSeconds)); });

        // Every wooden crossarm and cloud uses its actual collision rectangle.
        for (const auto& object : Objects().Objects()) {
            if (object->Id().rfind("utility-pole-", 0) == 0)
                stand(object->Id(), [](Rectangle ledge) { DrawUtilityPole(ledge); });
            if (object->Id().rfind("sky-cloud-", 0) == 0) stand(object->Id(), [](Rectangle p) { DrawCloud(p); });
        }
        // The grounded masts, then the insulators and live cables.
        for (std::size_t i = 0; i <= wires.size(); ++i)
            stand(NumberedId("wire-mast-", i), [i](Rectangle) { DrawWireMast(i); });
        // All three cable states use the same bounds and timing as collision.
        for (std::size_t i = 0; i < cableIds.size(); ++i) {
            stand(cableIds[i], [this, i](Rectangle) {
                const float elapsed = ElapsedSeconds();
                const int w = static_cast<int>(i);
                DrawElectricWire(wires[i], WireWarning(w, elapsed), WireLive(w, elapsed), elapsed);
            });
        }
        stand("checkpoint-flag", [this](Rectangle) {
            const Vector2 flag{firstCloud.x + 6, firstCloud.y - 27};
            DrawLineEx(flag, {flag.x, firstCloud.y}, 3, DARKBLUE);
            DrawTriangle(flag, {flag.x, flag.y + 13}, {flag.x + 20, flag.y + 6},
                         checkpoint.x >= firstCloud.x ? LIME : GOLD);
        });
        for (std::size_t k = 0; k < gasSlots; ++k) {
            stand(NumberedId("gas-", k), [this, k](Rectangle) {
                if (k < gasPuffs.size()) DrawGasPuff(gasPuffs[k].center, gasPuffs[k].age);
            });
        }
        for (std::size_t k = 0; k < pigSlots; ++k) {
            stand(NumberedId("pig-", k), [this, k](Rectangle) {
                if (k < pigs.size()) DrawFlyingPig(pigs[k].bounds, ElapsedSeconds());
            });
            Objects().Get(NumberedId("pig-", k))->SetFacing(Facing::Left);
        }
        for (std::size_t k = 0; k < starCount; ++k) {
            stand(NumberedId("star-warning-", k), [this, k](Rectangle) {
                const float halfView = worldViewport.height / (2 * Camera().zoom);
                DrawCloudStarWarning(stars[k].x, Camera().target.y - halfView + 16, Camera().target.y + halfView,
                                     stars[k].warningSeconds);
            });
            stand(NumberedId("falling-star-", k), [this, k](Rectangle) {
                const auto& star = stars[k];
                DrawLineEx({star.x - 10.0f, star.y - 65.0f}, {star.x, star.y}, 9.0f, {243, 184, 117, 255});
                DrawCircle(static_cast<int>(star.x), static_cast<int>(star.y), 17.0f, {255, 225, 149, 255});
            });
        }
        // Rainbow slide drops the player onto the campus sidewalk.
        stand("rainbow-slide", [](Rectangle) {
            for (int i = 0; i < 6; ++i) {
                const float shift = static_cast<float>(i - 2) * 7.0f;
                for (int step = 0; step < 60; ++step) {
                    auto a = SlidePoint(static_cast<float>(step) / 60);
                    auto b = SlidePoint(static_cast<float>(step + 1) / 60);
                    a.x += 15 + shift; b.x += 15 + shift;
                    a.y += 44; b.y += 44;
                    DrawLineEx(a, b, 8, rainbowColors[i]);
                }
            }
        });
        stand("rainbow-gate", [](Rectangle) {
            DrawRectangleRec({rainbowGate.x - 2, rainbowGate.y - 2, 12, rainbowGate.height + 4}, gateTrim);
            for (int stripe = 0; stripe * 12 < rainbowGate.height; ++stripe) {
                const float offset = stripe * 12.0f;
                DrawRectangleRec({rainbowGate.x, rainbowGate.y + offset, 8, std::min(12.0f, rainbowGate.height - offset)},
                                 rainbowColors[stripe % 6]);
            }
        });
        stand("sign-rainbow", [](Rectangle) {
            DrawRectangleRounded({rainbowGate.x + 6, rainbowEntrance.y - 38, 132, 24}, 0.2f, 4, gateTrim);
        });
        stand("campus-path", [](Rectangle) {
            DrawRectangleRec({6880, floorY, 325, 120}, {116, 130, 140, 255});
            DrawRectangleRec({6880, floorY, 325, 8}, {202, 212, 216, 255});
        });
        stand("lecture-hall", [](Rectangle) {
            DrawRectangleRec({6995, 1300, 210, 180}, {157, 139, 127, 255});
            DrawTriangle({6975, 1300}, {7100, 1245}, {7225, 1300}, {83, 79, 103, 255});
            DrawRectangleRec({6995, 1300, 210, 12}, {221, 204, 181, 255});
            for (int x : {7012, 7161}) {
                DrawRectangleRec({static_cast<float>(x), 1335, 30, 50}, {69, 113, 143, 255});
                DrawRectangleRec({static_cast<float>(x + 13), 1335, 3, 50}, {210, 218, 224, 255});
            }
            for (int x : {7045, 7145}) DrawRectangleRec({static_cast<float>(x), 1320, 9, 160}, {224, 211, 188, 255});
            DrawRectangleRec({7080, 1375, 75, 105}, {38, 55, 74, 255});
            DrawRectangleLinesEx({7080, 1375, 75, 105}, 3, {223, 210, 186, 255});
            DrawCircle(7142, 1430, 3, GOLD);
        });
        stand("anvil", [this](Rectangle) { DrawAnvil(Objects().Get(anvilId)->Bounds()); });
        stand("anvil-poof", [this](Rectangle) {
            DrawAnvilPoof(Objects().Get(anvilId)->Bounds(), ElapsedSeconds() - anvilLandedAt);
        });
    }

    void Level1::DrawWorldFront(const Resources& resources) const {
        // Words are drawn after every object, so they stay readable on top of sprites.
        DrawSignText(resources, "SCHOOL >", {500, 1353, 70, 18}, 12, RAYWHITE);
        DrawSignText(resources, "< WRONG WAY", {646, 1359, 88, 18}, 12, RAYWHITE);
        DrawSignText(resources, "< UNDERPASS", {roadStart - 110, floorY + 28, 79, 15}, 11, RAYWHITE);
        DrawSignText(resources, "TRAFFIC TUNNEL", {tunnelEntranceX + 53, floorY - 135 + 29, 124, 20}, 12, RAYWHITE);
        resources.DrawText("CHECKPOINT", {3703, 810}, 14, 1, RAYWHITE);
        DrawSignText(resources, "HOLD S: DUCK", {4750, 652, 102, 16}, 13, RAYWHITE);
        if (!pileupSettled && pileupStarted && !crashImpact) {
            const Color amber{255, 199, 69, 255};
            DrawSignText(resources, "CHICKEN CROSSING", {chickenX - 53, 1411, 130, 17}, 13, amber);
            const auto& lead = traffic[1].bounds;
            if (pileupSeconds >= 0 && lead.x - chickenX < 190)
                resources.DrawText("BRAKE!", {lead.x + 34, lead.y - 20}, 16, 1, GOLD);
        }
        if (crashImpact && pileupSeconds < blastDuration) {
            const Blast blast = BlastAt(CrashImpact(), pileupSeconds);
            DrawSignText(resources, "CRASH!", {blast.center.x - 49, blast.center.y - 14, 98, 28}, 28, Fade({100, 53, 55, 255}, blast.opacity), FontRole::Heading);
        }
        if (pileupStarted && !crashImpact && GetPlayer().Position().x > 2620.0f)
            resources.DrawText("CHICKEN CROSSING - WAIT FOR THE CRASH", {2648, 1320}, 12, 1, GOLD);
        if (pileupSettled) resources.DrawText("CLIMB UP!", {2710, 1338}, 17, 1, RAYWHITE);
        const Rectangle player = GetPlayer().Bounds();
        bool pigApproaching = false;
        for (const auto& flyer : pigs) {
            const float distance = flyer.bounds.x - (player.x + player.width);
            if (distance > 0 && distance < 220 && std::abs(flyer.bounds.y - player.y) < 55) pigApproaching = true;
        }
        if (!GetPlayer().Crouching() && pigApproaching) {
            const auto p = GetPlayer().Position();
            resources.DrawText("HOLD S!", {p.x - 12, p.y - 50}, 14, 1, GOLD);
        }
        DrawSignText(resources, "RAINBOW SLIDE >", {rainbowGate.x + 13, rainbowEntrance.y - 32, 118, 18}, 14, RAYWHITE);
        DrawSignText(resources, "LECTURE HALL", {7019, 1308, 162, 18}, 18, {44, 48, 66, 255});
        if (campusReached) resources.DrawText("YOU MADE IT!", {6915, 1420}, 15, 1, {160, 237, 179, 255});
    }

    void Level1::DrawHud(const Resources& resources) const {
        timer.Draw(resources);
    }
    std::unique_ptr<Level> MakeLevel1() { return std::make_unique<Level1>(); }
}
