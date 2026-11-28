#include "player/jump_particles.h"
#include "test_support.h"

#include <iostream>
#include <limits>

namespace rage {
namespace {
using test::Check;
using test::Near;

constexpr float tick = 1.0f / 120.0f;
constexpr Rectangle world{0, -2000, 20000, 2700};
const std::vector<Rectangle> floor{{0, 650, 20000, 50}};

void TestGroundedCrouch() {
    GameConfig config;
    Player player;
    player.Reset({100, 606}, config);
    player.Update(tick, {.duck = true}, config, floor, world);
    Check(player.Grounded() && player.Crouching(), "Ground contact after reset did not allow crouching");
    player.Update(tick, {.jump = true, .duck = true}, config, floor, world);
    Check(!player.Grounded() && !player.Crouching() && player.Bounds().height == config.standingHeight,
          "Holding crouch kept the player compact after takeoff");
    Check(Near(player.Velocity().y, -config.jumpSpeed * config.crouchJumpMultiplier + config.gravity * tick),
          "Ground-only crouch removed the crouch-jump boost");
    const float stamina = player.StaminaPercent();
    for (int i = 0; i < 12; ++i) {
        player.Update(tick, {.duck = true}, config, floor, world);
        Check(!player.Crouching() && player.Bounds().height == config.standingHeight,
              "Duck input crouched the player while rising");
    }
    Check(Near(player.StaminaPercent(), stamina, 0.0001f), "Ignored airborne crouch still drained stamina");

    Player regular;
    regular.Reset({100, 606}, config);
    regular.Update(tick, {.jump = true}, config, floor, world);
    Player ducking = regular;
    for (int i = 0; i < 50; ++i) {
        regular.Update(tick, {}, config, floor, world);
        ducking.Update(tick, {.duck = true}, config, floor, world);
        Check(!ducking.Crouching() && Near(ducking.Position().y, regular.Position().y) &&
                  Near(ducking.StaminaPercent(), regular.StaminaPercent(), 0.0001f),
              "Pressing crouch in the air changed the hitbox, trajectory or stamina");
    }
    Check(ducking.Velocity().y > 0, "Airborne crouch test did not exercise falling");
    for (int i = 0; i < 100 && !ducking.Crouching(); ++i)
        ducking.Update(tick, {.duck = true}, config, floor, world);
    Check(ducking.Grounded() && ducking.Crouching(), "Held crouch did not work again after landing");

    const std::vector<Rectangle> ledge{{0, 650, 130, 50}};
    player.Reset({129.9f, 606}, config);
    player.Update(tick, {.move = 1, .duck = true}, config, ledge, world);
    player.Update(tick, {.move = 1, .duck = true}, config, ledge, world);
    Check(!player.Grounded() && !player.Crouching() && player.Bounds().height == config.standingHeight,
          "Walking off a ledge retained crouch");

    const std::vector<Rectangle> clouds{{0, 650, 20000, 20}};
    player.Reset({100, 606}, config);
    player.Update(tick, {.jump = true, .duck = true}, config, {}, world, clouds);
    Check(!player.Crouching() && player.TakeCrouchJump().has_value(), "One-way support lost crouch jumps");
}

void TestTakeoffEvents() {
    GameConfig config;
    Player player;
    player.Reset({100, 606}, config);
    player.Update(tick, {.duck = true}, config, floor, world);
    Check(!player.TakeCrouchJump(), "Crouching on the ground emitted a jump burst");
    player.Update(tick, {.jump = true}, config, floor, world); // Release crouch on takeoff.
    const auto feet = player.TakeCrouchJump();
    Check(feet && Near(feet->x, 115) && Near(feet->y, 650), "Crouch jump lost its actual takeoff position");
    Check(!player.TakeCrouchJump(), "One crouch jump emitted more than one burst");
    player.Update(0.1f, {.jump = true, .duck = true}, config, floor, world);
    Check(!player.TakeCrouchJump(), "Held airborne inputs emitted another burst");
    player.Reset({100, 606}, config);
    player.Update(tick, {.jump = true}, config, floor, world);
    Check(!player.TakeCrouchJump(), "Regular jump emitted a crouch-jump burst");
    player.Reset({100, 606}, config);
    player.Bounce({0, -500});
    player.Update(tick, {.duck = true}, config, floor, world);
    Check(!player.Crouching() && !player.TakeCrouchJump(), "Bounce enabled crouching or emitted a jump burst");

    config.crouchJumpStaminaCost = config.maxStamina + 1;
    player.Reset({100, 606}, config);
    player.Update(tick, {.jump = true, .duck = true}, config, floor, world);
    Check(player.Grounded() && !player.TakeCrouchJump(), "Unaffordable crouch jump emitted a burst");
    config.crouchJumpStaminaCost = 30;
    player.Update(tick, {.jump = true, .duck = true}, config, floor, world);
    player.Reset({100, 606});
    Check(!player.TakeCrouchJump(), "Reset retained an undelivered crouch-jump burst");
}

void TestParticleLifetime() {
    JumpParticles particles;
    particles.Emit({100, 100});
    const std::size_t burst = particles.Count();
    Check(burst > 0, "Crouch-jump burst did not emit particles");
    for (float dt : {0.0f, -1.0f, std::numeric_limits<float>::quiet_NaN()}) particles.Update(dt);
    Check(particles.Count() == burst, "Paused or invalid time aged the particles");
    particles.Update(0.2f);
    particles.Emit({200, 100});
    Check(particles.Count() == burst * 2, "A second jump replaced the previous burst");
    particles.Update(0.3f);
    Check(particles.Count() > 0 && particles.Count() <= burst, "Particles did not expire independently");
    particles.Update(0.5f);
    Check(particles.Count() == 0, "Jump particles did not disappear");
    particles.Emit({100, 100});
    particles.Clear();
    Check(particles.Count() == 0, "Reset did not clear jump particles");
}
}

void TestCrouchJumps() {
    TestGroundedCrouch();
    TestTakeoffEvents();
    TestParticleLifetime();
    std::cout << "PASS: ground-only crouching, crouch-jump takeoffs, stamina, landing and pixel particle lifetime\n";
}

void TestJumpParticleRendering(const Resources &resources) {
    JumpParticles particles;
    particles.Emit({100, 100});
    RenderQueue queue;
    particles.SubmitSprites(queue, 1);
    Check(queue.Size() == particles.Count(), "Particle renderer did not queue the burst");
    BeginTextureMode(resources.Canvas());
    ClearBackground(BLACK);
    queue.Flush();
    EndTextureMode();
    Image rendered = LoadImageFromTexture(resources.Canvas().texture);
    ImageFlipVertical(&rendered);
    // The innermost square is 2x2, snapped to the art grid, with sharp edges.
    const Color square = GetImageColor(rendered, 98, 98);
    const Color corner = GetImageColor(rendered, 99, 99);
    const Color outside = GetImageColor(rendered, 100, 98);
    const bool crisp = square.r == RAYWHITE.r && square.g == RAYWHITE.g &&
                       corner.r == square.r && corner.g == square.g && outside.r == 0;
    UnloadImage(rendered);
    Check(crisp, "Jump particles did not render as crisp, grid-aligned squares");
    particles.Update(1);
    particles.SubmitSprites(queue, 1);
    Check(queue.Size() == 0, "Expired jump particles were still queued");
    std::cout << "PASS: crisp square air-particle rendering and expiry\n";
}
}
