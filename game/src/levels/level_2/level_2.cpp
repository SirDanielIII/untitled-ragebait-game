
#include "levels/level_2/level_2.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace rage {

    namespace {
        bool Overlaps(Rectangle a, Rectangle b) {
            return CheckCollisionRecs(a, b);
        }

        constexpr float BIRD_WIDTH = 38.0f;
        constexpr float BIRD_HEIGHT = 22.0f;
        constexpr float SWOOP_DURATION = 0.8f;
        constexpr float SWOOP_DISTANCE = 100.0f;
        constexpr float KNOCKBACK_X = 420.0f;
        constexpr float KNOCKBACK_Y = -280.0f;
    }

    const LevelDefinition &Level2Definition() {
        static const LevelDefinition definition{
            .id = LevelId::Level2,
            .title = "Level 2",
            .description = "Climb the stairwell to the top-floor classroom.",
            .plannedContent = "Climb exposed wall bricks; watch for loose masonry. Full stairwell layout is still to be designed.",
            .intro = {
                .text = "The classroom is upstairs. The stairs appear to have other plans.\nThose bricks look perfectly load-bearing. I am almost certain.",
                .speaker = "The Entity",
                .speedMultiplier = 1.0f,
                .startDelay = 0.0f,
                .fontSize = 24.0f,
                .allowSkip = true,
                .allowReplay = true
            },
            .accent = {177, 143, 210, 255},
            .backdrop = ArtId::Level2,
            .world = {0, -120, 1000, 1570},
            .spawn = {180, 1406},

            .objects = {
                // STREET LEVEL
                {.id = "floor", .type = ObjectType::Static, .collision = CollisionType::Solid,
                 .position = {0, 1450}, .size = {1000, 50}, .assetId = "platform"},

                // LAMPPOST 1
                {.id = "lamppost-1", .type = ObjectType::Static, .collision = CollisionType::Solid,
                 .position = {120, 950}, .size = {45, 500}, .assetId = "platform"},

                // STREET AWNINGS
                {.id = "awning-1", .type = ObjectType::Static, .collision = CollisionType::Solid,
                 .position = {220, 1300}, .size = {120, 20}, .assetId = "platform"},

                {.id = "awning-2", .type = ObjectType::Static, .collision = CollisionType::Solid,
                 .position = {380, 1190}, .size = {110, 20}, .assetId = "platform"},

                {.id = "awning-3", .type = ObjectType::Static, .collision = CollisionType::Solid,
                 .position = {220, 1080}, .size = {100, 20}, .assetId = "platform"},

                // WINDOW LEDGES
                {.id = "window-ledge-1", .type = ObjectType::Static, .collision = CollisionType::Solid,
                 .position = {400, 1030}, .size = {90, 20}, .assetId = "platform"},

                {.id = "window-ledge-2", .type = ObjectType::Static, .collision = CollisionType::Solid,
                 .position = {530, 920}, .size = {90, 20}, .assetId = "platform"},

                // FIRE ESCAPE
                {.id = "fire-escape-1", .type = ObjectType::Static, .collision = CollisionType::Solid,
                 .position = {680, 940}, .size = {150, 20}, .assetId = "platform"},

                {.id = "fire-escape-2", .type = ObjectType::Static, .collision = CollisionType::Solid,
                 .position = {680, 800}, .size = {150, 20}, .assetId = "platform"},

                {.id = "fire-escape-3", .type = ObjectType::Static, .collision = CollisionType::Solid,
                 .position = {680, 660}, .size = {150, 20}, .assetId = "platform"},

                // CONNECTING WINDOW LEDGES
                {.id = "window-ledge-3", .type = ObjectType::Static, .collision = CollisionType::Solid,
                 .position = {490, 810}, .size = {100, 20}, .assetId = "platform"},

                {.id = "window-ledge-4", .type = ObjectType::Static, .collision = CollisionType::Solid,
                 .position = {480, 670}, .size = {100, 20}, .assetId = "platform"},

                // AIR CONDITIONING UNITS
                {.id = "ac-unit-1", .type = ObjectType::Static, .collision = CollisionType::Solid,
                 .position = {850, 870}, .size = {90, 30}, .assetId = "platform"},

                {.id = "ac-unit-2", .type = ObjectType::Static, .collision = CollisionType::Solid,
                 .position = {850, 560}, .size = {90, 30}, .assetId = "platform"},

                // LAMPPOST 2
                {.id = "lamppost-2", .type = ObjectType::Static, .collision = CollisionType::Solid,
                 .position = {900, 350}, .size = {40, 1100}, .assetId = "platform"},

                // UPPER LEDGES
                {.id = "upper-ledge-1", .type = ObjectType::Static, .collision = CollisionType::Solid,
                 .position = {730, 520}, .size = {100, 20}, .assetId = "platform"},

                {.id = "upper-ledge-2", .type = ObjectType::Static, .collision = CollisionType::Solid,
                 .position = {590, 420}, .size = {100, 20}, .assetId = "platform"},

                {.id = "upper-ledge-3", .type = ObjectType::Static, .collision = CollisionType::Solid,
                 .position = {740, 320}, .size = {100, 20}, .assetId = "platform"},

                // ROOFTOP CLIMB
                {.id = "roof-ledge-1", .type = ObjectType::Static, .collision = CollisionType::Solid,
                 .position = {560, 240}, .size = {100, 20}, .assetId = "platform"},

                {.id = "roof-ledge-2", .type = ObjectType::Static, .collision = CollisionType::Solid,
                 .position = {700, 150}, .size = {100, 20}, .assetId = "platform"},

                {.id = "rooftop", .type = ObjectType::Static, .collision = CollisionType::Solid,
                 .position = {350, 70}, .size = {200, 30}, .assetId = "platform"},
            },
        };

        return definition;
    }

    Level2::Level2() : Level(Level2Definition()) {
        for (auto &event : CreateEvents()) {
            Events().RegisterEvent(std::move(event));
        }
    }


    void Level2::OnReset() {
        birds = {{
            // Lower bird
            {{250, 1200, BIRD_WIDTH, BIRD_HEIGHT},
             150.0f, 200.0f, 520.0f, 1200.0f,
             1.5f, 0.0f, 0.0f, true, false},

            // Middle bird
            {{500, 760, BIRD_WIDTH, BIRD_HEIGHT},
             180.0f, 450.0f, 850.0f, 760.0f,
             2.5f, 0.0f, 0.0f, true, false},

            // Upper bird
            {{600, 280, BIRD_WIDTH, BIRD_HEIGHT},
             210.0f, 500.0f, 900.0f, 280.0f,
             3.5f, 0.0f, 0.0f, false, false}
        }};
    }

    void Level2::OnUpdate(float dt, const InputFrame &) {
        if (!std::isfinite(dt) || dt <= 0.0f) return;

        const Rectangle playerBounds = GetPlayer().Bounds();
        const Vector2 playerPosition = GetPlayer().Position();

        for (auto &bird : birds) {
            bird.swoopCooldown -= dt;
            bird.attackCooldown = std::max(0.0f, bird.attackCooldown - dt);

            // Horizontal patrol.
            bird.bounds.x += (bird.facingRight ? 1.0f : -1.0f)
                             * bird.speed * dt;

            if (bird.bounds.x >= bird.patrolRight) {
                bird.bounds.x = bird.patrolRight;
                bird.facingRight = false;
            } else if (bird.bounds.x <= bird.patrolLeft) {
                bird.bounds.x = bird.patrolLeft;
                bird.facingRight = true;
            }

            // Start a swoop when the player is nearby.
            const float distanceX = std::abs(
                (playerBounds.x + playerBounds.width / 2.0f)
                - (bird.bounds.x + bird.bounds.width / 2.0f));

            const float distanceY = std::abs(
                (playerBounds.y + playerBounds.height / 2.0f)
                - (bird.bounds.y + bird.bounds.height / 2.0f));

            if (!bird.swooping && bird.swoopCooldown <= 0.0f
                && distanceX < 260.0f && distanceY < 220.0f) {
                bird.swooping = true;
                bird.swoopTimer = 0.0f;
                bird.swoopCooldown = 3.5f;
            }

            // Dive downward, then return to normal flying height.
            if (bird.swooping) {
                bird.swoopTimer += dt;

                const float progress = std::clamp(
                    bird.swoopTimer / SWOOP_DURATION, 0.0f, 1.0f);

                bird.bounds.y = bird.homeY
                    + std::sin(progress * PI) * SWOOP_DISTANCE;

                if (progress >= 1.0f) {
                    bird.swooping = false;
                    bird.bounds.y = bird.homeY;
                }
            }

            // Hit the player.
            if (bird.attackCooldown <= 0.0f
                && Overlaps(bird.bounds, playerBounds)) {

                const float playerCenter =
                    playerBounds.x + playerBounds.width / 2.0f;

                const float birdCenter =
                    bird.bounds.x + bird.bounds.width / 2.0f;

                const float direction =
                    playerCenter < birdCenter ? -1.0f : 1.0f;

                MutablePlayer().ApplyKnockback({
                    direction * KNOCKBACK_X,
                    KNOCKBACK_Y
                });

                bird.attackCooldown = 1.0f;
                bird.swooping = false;
                bird.bounds.y = bird.homeY;
            }
        }
    }

    void Level2::DrawWorld(const Resources &) const {
        for (const auto &bird : birds) {
            const float centerX = bird.bounds.x + bird.bounds.width / 2.0f;
            const float centerY = bird.bounds.y + bird.bounds.height / 2.0f;

            // Body and head.
            DrawEllipse(static_cast<int>(centerX),
                        static_cast<int>(centerY),
                        16.0f, 9.0f, DARKGRAY);

            DrawCircle(static_cast<int>(centerX + (bird.facingRight ? 12 : -12)),
                       static_cast<int>(centerY - 5),
                       6.0f, BLACK);

            // Beak.
            if (bird.facingRight) {
                DrawTriangle(
                    {centerX + 17, centerY - 6},
                    {centerX + 27, centerY - 3},
                    {centerX + 17, centerY},
                    ORANGE
                );
            } else {
                DrawTriangle(
                    {centerX - 17, centerY - 6},
                    {centerX - 27, centerY - 3},
                    {centerX - 17, centerY},
                    ORANGE
                );
            }

            // Wing.
            DrawEllipse(static_cast<int>(centerX),
                        static_cast<int>(centerY - 5),
                        10.0f, 4.0f, GRAY);
        }
    }

    std::unique_ptr<Level> MakeLevel2() {
        return std::make_unique<Level2>();
    }

}
