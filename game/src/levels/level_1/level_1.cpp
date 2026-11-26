#include "levels/level_1/level_1.h"

#include "rlgl.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace rage {
    namespace {
        // All coordinates use world pixels. The player's collider is 30x44.
        // Default movement (260 px/s) and jump (620 px/s, gravity 1500) allow
        // about 128 px of jump height. Ledges in the climb are 78-102 px apart.
        constexpr float floorY = 1480.0f;
        constexpr float collisionGraceSeconds = 0.35f;
        constexpr float roadStart = 580.0f;
        // Traffic enters the visible road at the right portal and descends
        // beneath the sidewalk at the left. The player's floor stays flat.
        constexpr float tunnelEntranceX = 1740.0f;
        constexpr float tunnelSpawnX = 1880.0f;
        constexpr float rampStartX = 820.0f;
        constexpr float rampEndX = 580.0f;
        constexpr float underpassDepth = 110.0f;
        constexpr float tunnelEndX = 2140.0f;
        constexpr float blastDuration = 1.05f;
        constexpr float wreckFlightSeconds = 1.25f;
        constexpr float wreckStagger = 0.16f;
        constexpr float foundationY = 1572.0f;
        constexpr float pigSpeed = 155.0f;
        constexpr float starWarning = 0.24f;
        constexpr Rectangle pigStart{4250, 642, 94, 38};
        constexpr float duckHeight = 18.0f;
        constexpr float duckTransition = 0.09f;
        constexpr std::array<Rectangle, 3> refuges{{
            {3075, 705, 96, 16},
            {3360, 690, 96, 16},
            {3635, 675, 92, 16}
        }};

        // The solid surfaces are the flat roofs, not the entire car chassis.
        // Keep the original step centres and heights; the chassis overhangs the roof.
        constexpr std::array<Rectangle, 3> wreckSteps{{
            {1878, 1378, 88, 16},
            {2006, 1300, 88, 16},
            {2134, 1222, 88, 16}
        }};
        constexpr float pileupDuration = wreckFlightSeconds + (wreckSteps.size() - 1) * wreckStagger;
        constexpr std::array<const char*, 3> wreckIds{{"pileup-step-1", "pileup-step-2", "pileup-step-3"}};
        constexpr Rectangle trashCan{260, 1426, 46, 54};
        constexpr Rectangle hydrant{445, 1437, 38, 43};
        constexpr Vector2 slideStart{4700.0f, 741.0f};
        constexpr Vector2 slideEnd{4925.0f, 1436.0f};
        constexpr float slideDuration = 1.8f;
        constexpr std::array<Rectangle, 3> wires{{
            {2950.0f, 796.0f, 240.0f, 12.0f},
            {3250.0f, 781.0f, 220.0f, 12.0f},
            {3530.0f, 766.0f, 220.0f, 12.0f}
        }};

        bool Intersects(Rectangle a, Rectangle b) {
            return CheckCollisionRecs(a, b);
        }
        float RoadDepth(float x) {
            const float t = std::clamp((rampStartX - x) / (rampStartX - rampEndX), 0.0f, 1.0f);
            return underpassDepth * t * t;
        }

        void MoveRoadCar(Rectangle& body, float speed, float dt, float spawnX) {
            body.x += speed * dt;
            // Wrap only once the entire vehicle has passed behind the sidewalk.
            if (body.x + body.width <= roadStart) body.x = spawnX;
            body.y = floorY - body.height + RoadDepth(body.x + body.width * 0.5f);
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
        // The merged engine owns solid collision through ObjectManager.
        // Local helper keeps each authored ledge a concise GameObjectDefinition.
        GameObjectDefinition SolidPlatform(const char* id, Rectangle bounds,
                                           const char* assetId = "", bool active = true) {
            return {
                .id = id,
                .type = ObjectType::Static,
                .collision = CollisionType::Solid,
                .position = {bounds.x, bounds.y},
                .size = {bounds.width, bounds.height},
                .assetId = assetId,
                .active = active
            };
        }
        // Clouds are top-only platforms: jump through them from below.
        GameObjectDefinition CloudPlatform(const char* id, Rectangle bounds) {
            auto cloud = SolidPlatform(id, bounds);
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
            DrawText("TRAFFIC TUNNEL", static_cast<int>(x + 53), static_cast<int>(top + 31), 12, RAYWHITE);
            DrawRectangleRec({x + 16, floorY - 5, width - 16, 5}, seam);
        }

        void DrawUnderpassForeground() {
            // A departing car is drawn first; the opaque sidewalk hides its
            // left portion as it descends. Never cover the playable surface.
            DrawRectangleRec({0, floorY, roadStart, 120}, {108, 119, 133, 255});
            DrawRectangleRec({0, floorY, roadStart, 10}, {191, 199, 206, 255});
            for (int x = 0; x < static_cast<int>(roadStart); x += 72)
                DrawLine(x, 1488, x, 1520, {76, 91, 108, 255});
            DrawRectangleRec({roadStart - 12, floorY + 10, 12, 110}, {65, 76, 91, 255});
            // Mask any car pixels still above the deck just inside the mouth.
            DrawRectangleRec({roadStart - 17, floorY - 5, 17, 15}, {191, 199, 206, 255});
            DrawRectangleRounded({roadStart - 116, floorY + 23, 91, 20}, 0.2f, 4, {48, 63, 81, 255});
            DrawText("< UNDERPASS", static_cast<int>(roadStart - 110), static_cast<int>(floorY + 29), 11, RAYWHITE);
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

        void DrawCrashBlast(Vector2 impact, float seconds) {
            const float t = seconds / blastDuration;
            if (t >= 1) return;
            const float expansion = 1 - (1 - t) * (1 - t);
            const float radius = 18 + 100 * expansion;
            const float opacity = 1 - t * t;
            const Vector2 center{impact.x, impact.y - 23 * expansion};
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
            DrawText("CRASH!", static_cast<int>(center.x - 49), static_cast<int>(center.y - 14),
                     28, Fade({100, 53, 55, 255}, opacity));
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
        void DrawWireSupports() {
            const Color shadow{71, 50, 43, 255};
            const Color timber{137, 98, 71, 255};
            const Color highlight{189, 146, 106, 255};
            const Color grain{103, 71, 53, 255};
            const Color steel{65, 78, 94, 255};
            const Color bolt{183, 195, 202, 255};

            // There are N+1 masts for N spans. End masts sit under the
            // outermost insulators; interior masts sit between adjacent spans.
            for (std::size_t i = 0; i <= wires.size(); ++i) {
                const bool hasLeft = i > 0;
                const bool hasRight = i < wires.size();
                const Rectangle left = hasLeft ? wires[i - 1] : wires.front();
                const Rectangle right = hasRight ? wires[i] : wires.back();
                const float leftX = left.x + left.width - 6.0f;
                const float rightX = right.x + 6.0f;
                const float center = !hasLeft ? rightX : !hasRight ? leftX
                    : (leftX + rightX) * 0.5f;
                // Match the existing insulator base at wire.y + 25..29.
                const float top = !hasLeft ? right.y + 28 : !hasRight ? left.y + 28
                    : std::min(left.y, right.y) + 28;

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
                if (hasLeft) attach(leftX, left.y);
                if (hasRight) attach(rightX, right.y);
            }
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
    }

    const LevelDefinition& Level1Definition() {
        static const LevelDefinition definition{
            .id = LevelId::Level1,
            .title = "Level 1 - Walk to School",
            .description = "Sidewalk -> road -> telephone poles -> clouds -> rainbow -> campus",
            .plannedContent = "Jump sidewalk obstacles, dodge right-to-left traffic and its chicken pile-up, "
                              "climb to electrified telephone wires, duck a flying pig (hold S), "
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
            // The merged GameObject system replaces LevelDefinition::platforms.
            // Authored collision geometry is independent of the procedural artwork.
            .objects = {
                SolidPlatform("sidewalk-road", {0, floorY, 2140, 120}),
                SolidPlatform(wreckIds[0], wreckSteps[0], "", false),
                SolidPlatform(wreckIds[1], wreckSteps[1], "", false),
                SolidPlatform(wreckIds[2], wreckSteps[2], "", false),
                SolidPlatform("telephone-step-1", {2260, 1148, 124, 18}),
                SolidPlatform("telephone-step-2", {2392, 1070, 124, 18}),
                SolidPlatform("telephone-step-3", {2524, 992, 124, 18}),
                SolidPlatform("telephone-step-4", {2656, 914, 128, 18}),
                SolidPlatform("sky-checkpoint-ledge", {2798, 832, 140, 18}),
                SolidPlatform("wire-1", wires[0]),
                SolidPlatform("wire-2", wires[1]),
                SolidPlatform("wire-3", wires[2]),
                CloudPlatform("wire-refuge-1", refuges[0]),
                CloudPlatform("wire-refuge-2", refuges[1]),
                CloudPlatform("wire-refuge-3", refuges[2]),
                CloudPlatform("cloud-1", {3790, 700, 170, 22}),
                CloudPlatform("cloud-2", {4000, 715, 170, 22}),
                CloudPlatform("cloud-3", {4210, 665, 180, 22}),
                CloudPlatform("cloud-4", {4440, 755, 170, 22}),
                CloudPlatform("rainbow-entrance", {4605, 785, 160, 22}),
                SolidPlatform("campus-sidewalk", {4875, floorY, 325, 120}),
                {.id = "exit-level1", .type = ObjectType::Trigger, .collision = CollisionType::Trigger,
                 .position = {5075, 1375}, .size = {75, 105}, .active = false, .triggerOnce = true}
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
        pig = pigStart;
        pigStarted = false;
        pigFinished = false;
        crashImpact = false;
        stars = {{
            {3650.0f, 3515.0f, 510.0f, 0.0f, false, false, false},
            {4290.0f, 4160.0f, 510.0f, 0.0f, false, false, false}
        }};
        checkpoint = Definition().spawn;
        collisionCooldown = 0.0f;
        duckBlend = 0.0f;
        pileupSeconds = 0.0f;
        slideProgress = 0.0f;
        chickenX = 1460.0f;
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
        duckBlend = 0.0f;
        if (skyCheckpointReached) {
            pig = pigStart;
            pigStarted = pigFinished = false;
            for (auto& star : stars) {
                star.warned = star.falling = star.finished = false;
                star.warningSeconds = 0;
            }
        }
        RequestSound(playerSounds::death);
        ++collisionCount;
        Events().Notify({notification, static_cast<std::uint64_t>(collisionCount)});
    }

    void Level1::OnUpdate(float dt, const InputFrame& input) {
        dt = std::min(dt, 0.1f); // Match the physics engine maximum step.
        // Slide motion stays inside the level rather than changing shared movement code.
        if (sliding) {
            slideProgress = std::min(1.0f, slideProgress + dt / slideDuration);
            MutablePlayer().Reset(SlidePoint(slideProgress));
            if (slideProgress >= 1.0f) {
                sliding = false;
                slideFinished = true;
                checkpoint = slideEnd;
                ActivateObject("exit-level1");
            }
            return;
        }
        if (campusReached) {
            if (GetPlayer().Position().y >= 1520.0f) Respawn("level1.fall_hit");
            return;
        }
        // A held key controls the crouch. The same blend drives its art and pig hitbox.
        const auto pose = GetPlayer().Position();
        const bool inSky = pose.x >= 2770.0f && pose.x < 4650.0f && pose.y < 1000.0f;
        if (!inSky || !GetPlayer().Grounded()) {
            duckBlend = 0.0f;
        } else {
            const float direction = input.duck ? 1.0f : -1.0f;
            duckBlend = std::clamp(duckBlend + direction * dt / duckTransition, 0.0f, 1.0f);
        }
        collisionCooldown = std::max(0.0f, collisionCooldown - dt);
        const Rectangle player = GetPlayer().Bounds();
        const Vector2 position = GetPlayer().Position();
        // Once the chicken event is requested, let ordinary traffic finish
        // its current trip. Park each vehicle only after it is fully concealed.
        // A negative pileup timer is the staging phase; zero begins the crash.
        if (!pileupStarted && position.x >= 1290.0f) {
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
        if (!pileupStarted) {
            for (auto& car : traffic)
                MoveRoadCar(car.bounds, car.speed, dt, tunnelSpawnX);
        } else if (pileupSeconds < 0.0f) {
            parkAfterExit(traffic[0], tunnelSpawnX);
            parkAfterExit(traffic[1], tunnelSpawnX);
            parkAfterExit(traffic[2], tunnelSpawnX);
            chickenX = std::max(1420.0f, chickenX - 13.0f * dt);
            if (traffic[0].speed == 0.0f && traffic[1].speed == 0.0f && traffic[2].speed == 0.0f)
                pileupSeconds = 0.0f;
        } else if (!pileupSettled) {
            chickenX = std::max(1420.0f, chickenX - 13.0f * dt);
            const float brakeX = chickenX + 58.0f;
            auto& lead = traffic[1].bounds;
            auto& follower = traffic[2].bounds;
            if (!crashImpact) {
                const float brakingSpeed = std::clamp((lead.x - brakeX) * 2.8f, 42.0f, 230.0f);
                lead.x = std::max(brakeX, lead.x - brakingSpeed * dt);
                // Release the follower from the tunnel as the lead car finishes braking.
                if (lead.x - brakeX < 32)
                    follower.x = std::max(lead.x + lead.width, follower.x - 345.0f * dt);
                if (lead.x <= brakeX + 1.0f && follower.x <= lead.x + lead.width + 1.0f) {
                    crashImpact = true;
                    pileupSeconds = 0;
                }
            } else {
                pileupSeconds += dt;
                if (pileupSeconds >= pileupDuration) {
                    pileupSettled = true;
                    for (const auto* id : wreckIds) ActivateObject(id);
                }
            }
        }
        // A single low-flying pig approaches the second cloud, once per attempt.
        if (!pigStarted && position.x >= 3920.0f && position.y < 800.0f) pigStarted = true;
        if (pigStarted && !pigFinished) {
            pig.x -= pigSpeed * dt;
            if (pig.x + pig.width < 3750.0f) pigFinished = true;
        }
        for (auto& star : stars) {
            if (star.finished) continue;
            if (!star.warned && position.x >= star.triggerX && position.x < star.x + 170.0f
                && position.y < 1000.0f) {
                star.warned = true;
                star.warningSeconds = starWarning;
                star.y = std::max(0.0f, position.y - 90.0f);
            }
            if (star.warned && !star.falling) {
                star.warningSeconds -= dt;
                if (star.warningSeconds <= 0.0f) star.falling = true;
            } else if (star.falling) {
                star.y += 720.0f * dt;
                if (star.y >= 1600.0f) {
                    star.falling = false;
                    star.finished = true;
                }
            }
        }
        if (position.x >= 4650.0f && position.x < 4765.0f && GetPlayer().Grounded()
            && position.y < 900.0f && !slideFinished) {
            sliding = true;
            duckBlend = 0.0f;
            slideProgress = 0.0f;
            MutablePlayer().Reset(slideStart);
            Events().Notify({"level1.slide_started"});
            return;
        }
        if (collisionCooldown > 0.0f) return;
        // Sidewalk: two stationary objects to jump over.
        if (Intersects(player, trashCan) || Intersects(player, hydrant)) {
            Respawn("level1.obstacle_hit");
            return;
        }
        // Cars approach from the RIGHT and move LEFT across the roadway.
        if (!crashImpact) {
            for (const auto& car : traffic) {
                const Rectangle visibleCar = VisibleRoadCar(car.bounds);
                if (visibleCar.width > 0.0f && Intersects(player, visibleCar)) {
                    Respawn("level1.traffic_hit");
                    return;
                }
            }
        }
        // Wire collision is a grounded contact, not simply passing beneath the wire.
        for (std::size_t i = 0; i < wires.size(); ++i) {
            if (!WireLive(static_cast<int>(i), ElapsedSeconds())) continue;
            const Rectangle wire = wires[i];
            const float feet = player.y + player.height;
            const bool standingOnWire = GetPlayer().Grounded()
                && player.x < wire.x + wire.width && player.x + player.width > wire.x
                && std::abs(feet - wire.y) < 5.0f;
            if (standingOnWire) {
                Respawn("level1.wire_hit");
                return;
            }
        }
        // Feet remain fixed; only the pig-specific collision profile gets shorter.
        Rectangle duckBody = player;
        const float reduction = (player.height - duckHeight) * duckBlend;
        duckBody.y += reduction;
        duckBody.height -= reduction;
        if (pigStarted && !pigFinished && Intersects(duckBody, pig)) {
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
        if (position.y >= 1520.0f) {
            Respawn("level1.fall_hit");
        }
    }

    void Level1::DrawPileup() const {
        const auto& follower = traffic[2].bounds;
        const Vector2 impact{follower.x, follower.y + follower.height * 0.45f};
        const Rectangle debrisOrigin{impact.x, impact.y, 0, 0};
        constexpr Color scrapColors[] = {{112, 115, 132, 255}, {132, 107, 111, 255}, {103, 125, 139, 255}};

        for (std::size_t i = 0; i < wreckSteps.size(); ++i) {
            const Rectangle roof = wreckSteps[i];
            const float elapsed = pileupSeconds - i * wreckStagger;
            const float t = std::clamp(elapsed / wreckFlightSeconds, 0.0f, 1.0f);
            int layer = 0;
            for (float y = floorY - 50; y > roof.y + 34; y -= 42, ++layer) {
                // The low layers land first, underneath the car that forms the roof.
                const float supportT = std::clamp(elapsed / (0.75f + layer * 0.09f), 0.0f, 1.0f);
                if (supportT <= 0) continue;
                const float shift = layer % 2 == 0 ? -3.0f : 3.0f;
                const Rectangle slot{roof.x - 22 + shift, y, roof.width + 44, 50};
                const Rectangle body = WreckFlight(debrisOrigin, slot, supportT, 35 + 9 * layer);
                rlPushMatrix();
                rlTranslatef(body.x, body.y, 0);
                rlScalef(body.width / slot.width, body.height / slot.height, 1);
                DrawCar({0, 0, slot.width, slot.height}, scrapColors[(i + layer) % 3]);
                rlPopMatrix();
            }
            const auto& car = traffic[(i + 1) % traffic.size()];
            const Rectangle source = i < 2 ? car.bounds : debrisOrigin;
            const Rectangle slot{roof.x - 22, roof.y, roof.width + 44, 54};
            const Rectangle body = WreckFlight(source, slot, t, 80 + 18 * i);
            if (body.width > 0) DrawCrashCar(body, car.color, roof.width, std::min(1.0f, t * 5), -18 * std::sin(PI * t));
        }
        DrawCrashBlast(impact, pileupSeconds);
    }

    void Level1::DrawWorld(const Resources&) const {
        // An ordinary sidewalk abruptly meets an oncoming carriageway.
        DrawRectangleRec({0, floorY, 580, 120}, {108, 119, 133, 255});
        DrawRectangleRec({0, floorY, 580, 10}, {191, 199, 206, 255});
        for (int x = 0; x < 580; x += 72)
            DrawLine(x, 1488, x, 1520, {76, 91, 108, 255});
        DrawRectangleRec({580, floorY, 1560, 120}, {49, 55, 70, 255});
        DrawRectangleRec({580, floorY, 1560, 5}, {105, 113, 126, 255});
        for (int x = 660; x < 2140; x += 150)
            DrawRectangleRec({static_cast<float>(x), 1528, 85, 5}, {241, 216, 130, 255});
        DrawUnderpassRoad();
        DrawTunnelBackdrop();
        DrawStreetPost(534.5f, 1378, 5);
        DrawRectangleRounded({494, 1342, 82, 36}, 0.15f, 4, {57, 110, 142, 255});
        DrawText("SCHOOL >", 500, 1353, 12, RAYWHITE);
        DrawStreetPost(690, 1380, 5);
        DrawRectangleRounded({640, 1350, 100, 30}, 0.15f, 4, {176, 66, 70, 255});
        DrawText("< WRONG WAY", 646, 1359, 12, RAYWHITE);
        DrawText("CHECKPOINT", 2803, 810, 14, RAYWHITE);
        DrawRectangleRounded({3845, 646, 112, 22}, 0.2f, 4, {43, 59, 79, 235});
        DrawText("HOLD S: DUCK", 3850, 652, 13, RAYWHITE);
        // Sidewalk hazards: clear silhouettes and consistent collision bounds.
        DrawRectangleRounded(trashCan, 0.16f, 6, {69, 116, 109, 255});
        for (int x = 270; x <= 296; x += 8)
            DrawRectangleRec({static_cast<float>(x), 1435, 3, 39}, {108, 151, 142, 255});
        DrawRectangleRounded({255, 1423, 56, 9}, 0.4f, 6, {126, 165, 154, 255});
        DrawRectangleRec({277, 1419, 12, 5}, {69, 116, 109, 255});
        DrawRectangleRounded(hydrant, 0.35f, 6, {201, 68, 80, 255});
        DrawCircle(464, 1439, 14, {236, 106, 106, 255});
        DrawCircle(450, 1454, 7, {234, 105, 109, 255});
        DrawCircle(478, 1454, 7, {234, 105, 109, 255});
        DrawRectangleRec({445, 1474, 38, 6}, {152, 46, 58, 255});
        // Traffic lights do not perfectly agree with the approaching cars.
        for (int x : {780, 1170, 1500}) {
            DrawStreetPost(x + 3, 1340, 7);
            DrawRectangleRec({static_cast<float>(x) - 13, 1290.0f, 33.0f, 55.0f}, {36, 38, 49, 255});
            const bool carNear = !crashImpact && std::any_of(traffic.begin(), traffic.end(), [x](const Vehicle& car) {
                const Rectangle visible = VisibleRoadCar(car.bounds);
                const float distance = visible.x - x;
                return visible.width > 0 && distance > -110 && distance < 230;
            });
            const bool green = SignalGreen(static_cast<float>(x), ElapsedSeconds(), carNear);
            DrawCircle(x + 3, 1304, 7, green ? Color{73, 78, 77, 255} : Color{235, 80, 79, 255});
            DrawCircle(x + 3, 1328, 7, green ? Color{112, 227, 144, 255} : Color{67, 73, 65, 255});
        }
        if (!crashImpact) {
            if (pileupStarted && pileupSeconds >= 0) {
                DrawCrashCar(traffic[1].bounds, traffic[1].color, wreckSteps[0].width, 0);
                DrawCrashCar(traffic[2].bounds, traffic[2].color, wreckSteps[1].width, 0);
            } else {
                for (const auto& car : traffic) DrawCar(car.bounds, car.color);
            }
        }
        // Draw opaque tunnel surfaces AFTER the cars, BEFORE the chicken and
        // wreck staircase. The wrecks can still be climbed in front of the wall.
        DrawUnderpassForeground();
        DrawTunnelForeground();
        // The chicken is scenery, but its presence causes the traffic event.
        if (!pileupSettled) {
            const int cx = static_cast<int>(chickenX);
            DrawLine(cx + 12, 1473, cx + 9, 1479, ORANGE);
            DrawLine(cx + 20, 1473, cx + 23, 1479, ORANGE);
            DrawEllipse(cx + 18, 1465, 17, 11, {250, 245, 225, 255});
            DrawCircle(cx + 4, 1454, 8, {250, 245, 225, 255});
            DrawCircle(cx + 3, 1447, 4, RED);
            DrawCircle(cx + 1, 1452, 1, BLACK);
            DrawTriangle({chickenX - 3, 1452}, {chickenX - 13, 1455}, {chickenX - 3, 1458}, ORANGE);
        }
        if (crashImpact) DrawPileup();
        if (pileupStarted && !crashImpact && GetPlayer().Position().x > 1720.0f)
            DrawText("ROAD BLOCKED - WAIT FOR THE CRASH", 1748, 1320, 12, GOLD);
        if (pileupSettled) DrawText("CLIMB UP!", 1810, 1338, 17, RAYWHITE);

        // Every wooden crossarm uses its actual collision rectangle.
        for (const auto& object : Definition().objects) {
            if (object.id.rfind("telephone-step-", 0) != 0 && object.id != "sky-checkpoint-ledge") continue;
            DrawUtilityPole({object.position.x, object.position.y, object.size.x, object.size.y});
        }

        // Draw the grounded masts first, then the insulators and live cables.
        DrawWireSupports();

        // All three cable states use the same bounds and timing as collision.
        for (std::size_t i = 0; i < wires.size(); ++i) {
            const float elapsed = ElapsedSeconds();
            DrawElectricWire(wires[i], WireWarning(static_cast<int>(i), elapsed),
                             WireLive(static_cast<int>(i), elapsed), elapsed);
        }

        // Cloud refuges above the wires allow the player to wait out electricity.
        for (const auto& refuge : refuges) DrawCloud(refuge);
        for (const auto& object : Definition().objects) {
            if (object.id.rfind("cloud-", 0) != 0 && object.id != "rainbow-entrance") continue;
            DrawCloud({object.position.x, object.position.y, object.size.x, object.size.y});
        }
        if (pigStarted && !pigFinished) {
            const int x = static_cast<int>(pig.x);
            const int y = static_cast<int>(pig.y);
            const float flap = std::sin(ElapsedSeconds() * 17.0f) * 5.0f;
            DrawEllipse(x + 49, y + 21, 34, 16, {237, 154, 174, 255});
            DrawCircle(x + 20, y + 18, 15, {244, 170, 188, 255});
            DrawEllipse(x + 7, y + 23, 8, 5, {212, 115, 144, 255});
            DrawCircle(x + 17, y + 15, 2, BLACK);
            DrawTriangle({pig.x + 13, pig.y + 8}, {pig.x + 12, pig.y}, {pig.x + 25, pig.y + 6}, PINK);
            DrawEllipse(x + 47, static_cast<int>(pig.y + 9 + flap), 19, 9, RAYWHITE);
            DrawLine(x + 81, y + 19, x + 92, y + 11, PINK);
        }
        if (duckBlend > 0.0f && GetPlayer().Grounded()) {
            const auto p = GetPlayer().Bounds();
            const float feet = p.y + p.height;
            DrawText("DUCKING", static_cast<int>(p.x - 12), static_cast<int>(feet - 58),
                     12, {137, 239, 186, 255});
            DrawRectangleRounded({p.x - 4, feet + 3, 38, 4}, 0.5f, 4, {35, 53, 70, 200});
            DrawRectangleRec({p.x - 4, feet + 3, 38 * duckBlend, 4}, {137, 239, 186, 255});
        } else if (pigStarted && !pigFinished && pig.x > GetPlayer().Position().x
                   && pig.x - GetPlayer().Position().x < 220.0f) {
            const auto p = GetPlayer().Position();
            DrawText("HOLD S!", static_cast<int>(p.x - 12), static_cast<int>(p.y - 50), 14, GOLD);
        }
        // A brief flash near the top of the local view precedes each star.
        for (const auto& star : stars) {
            if (star.warned && !star.falling && !star.finished) {
                DrawCircle(static_cast<int>(star.x), static_cast<int>(star.y), 17, {255, 239, 141, 255});
                DrawLineEx({star.x - 24, star.y}, {star.x + 24, star.y}, 3, RAYWHITE);
                DrawLineEx({star.x, star.y - 24}, {star.x, star.y + 24}, 3, RAYWHITE);
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
            const float shift = static_cast<float>(i - 2) * 7.0f;
            for (int step = 0; step < 60; ++step) {
                auto a = SlidePoint(static_cast<float>(step) / 60);
                auto b = SlidePoint(static_cast<float>(step + 1) / 60);
                a.x += 15 + shift; b.x += 15 + shift;
                a.y += 44; b.y += 44;
                DrawLineEx(a, b, 8, rainbow[i]);
            }
        }
        DrawText("RAINBOW SLIDE", 4605, 756, 14, {58, 77, 107, 255});
        DrawRectangleRec({4875, floorY, 325, 120}, {116, 130, 140, 255});
        DrawRectangleRec({4875, floorY, 325, 8}, {202, 212, 216, 255});
        DrawRectangleRec({4990, 1300, 210, 180}, {157, 139, 127, 255});
        DrawTriangle({4970, 1300}, {5095, 1245}, {5220, 1300}, {83, 79, 103, 255});
        DrawRectangleRec({4990, 1300, 210, 12}, {221, 204, 181, 255});
        for (int x : {5007, 5156}) {
            DrawRectangleRec({static_cast<float>(x), 1335, 30, 50}, {69, 113, 143, 255});
            DrawRectangleRec({static_cast<float>(x + 13), 1335, 3, 50}, {210, 218, 224, 255});
        }
        for (int x : {5040, 5140}) DrawRectangleRec({static_cast<float>(x), 1320, 9, 160}, {224, 211, 188, 255});
        DrawRectangleRec({5075, 1375, 75, 105}, {38, 55, 74, 255});
        DrawRectangleLinesEx({5075, 1375, 75, 105}, 3, {223, 210, 186, 255});
        DrawCircle(5137, 1430, 3, GOLD);
        DrawText("LECTURE HALL", 5014, 1308, 18, {44, 48, 66, 255});
        if (campusReached) DrawText("YOU MADE IT!", 4910, 1420, 15, {160, 237, 179, 255});
    }

    std::unique_ptr<Level> MakeLevel1() { return std::make_unique<Level1>(); }
}