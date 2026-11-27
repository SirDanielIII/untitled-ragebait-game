#include "test_support.h"

#include <fstream>
#include <iostream>

namespace rage {
namespace {
using test::Check;
using test::Near;

constexpr float tick = 1.0f / 120.0f;
constexpr Rectangle world = {0, -2000, 20000, 2700};
const std::vector<Rectangle> floor = {{0, 650, 20000, 50}};

void Advance(Player &player, const GameConfig &config, int frames, InputFrame input = {}) {
    for (int i = 0; i < frames; ++i) player.Update(tick, input, config, floor, world);
}

void TestTuningConfig() {
    ConfigStore packaged;
    Check(packaged.Load(std::filesystem::path(GetApplicationDirectory()) / "resources/config.yaml"),
          "Packaged movement tuning has invalid/unrecognized keys");
    Check(packaged.values.moveSpeed == GameConfig{}.moveSpeed
          && packaged.values.jumpSpeed == GameConfig{}.jumpSpeed
          && packaged.values.gravity == GameConfig{}.gravity
          && packaged.values.lowStaminaThreshold == GameConfig{}.lowStaminaThreshold,
          "Packaged physics and fallback defaults disagree");

    test::TestDirectory directory;
    const auto tuning = directory.path / "tuning.yaml";
    {
        std::ofstream file(tuning);
        file << "player:\n  ground_acceleration: 1300\n  counter_strafe_acceleration: 2100\n"
             << "  start_speed: 105\n  ground_friction: 450\n  standing_width: 32\n  standing_height: 48\n"
             << "  crouch_height_multiplier: 0.4\n  crouch_speed_multiplier: 0.12\n  crouch_jump_multiplier: 1.4\n"
             << "  climb_speed: 170\n  climb_contact_distance: 3\n  bounce_climb_lock: 0.3\n"
             << "  max_stamina: 160\n  climb_stamina_drain: 55\n  jump_stamina_cost: 24\n"
             << "  crouch_jump_stamina_cost: 42\n  crouch_stamina_drain: 3\n"
             << "  stamina_regen: 35\n  low_stamina_threshold: 0.4\n"
             << "  exhaustion_recovery_threshold: 0.25\n  exhausted_speed_multiplier: 0.35\n";
    }
    ConfigStore store;
    Check(store.Load(tuning), "Custom player tuning failed to load");
    Check(store.Save(directory.path / "roundtrip.yaml"), "Player tuning failed to save");
    ConfigStore reloaded;
    Check(reloaded.Load(directory.path / "roundtrip.yaml"), "Saved tuning failed to load");
    const auto &c = reloaded.values;
    Check(c.groundAcceleration == 1300 && c.counterStrafeAcceleration == 2100 && c.startSpeed == 105
          && c.groundFriction == 450 && c.standingWidth == 32 && c.standingHeight == 48
          && Near(c.crouchHeightMultiplier, 0.4f) && Near(c.crouchSpeedMultiplier, 0.12f)
          && Near(c.crouchJumpMultiplier, 1.4f) && c.climbSpeed == 170 && c.climbContactDistance == 3
          && Near(c.bounceClimbLock, 0.3f) && c.maxStamina == 160 && c.climbStaminaDrain == 55
          && c.jumpStaminaCost == 24 && c.crouchJumpStaminaCost == 42 && c.crouchStaminaDrain == 3
          && c.staminaRegen == 35 && Near(c.lowStaminaThreshold, 0.4f)
          && Near(c.exhaustionRecoveryThreshold, 0.25f) && Near(c.exhaustedSpeedMultiplier, 0.35f),
          "Movement/stamina tuning did not survive loading and saving");
    Player player;
    player.Reset({100, 602}, c);
    Check(player.Bounds().height == 48 && player.StaminaPercent() == 1, "Reset ignored custom body/stamina");
    player.Update(tick, {.jump = true}, c, floor, world);
    Check(Near(player.StaminaPercent(), (160.0f - 24) / 160, 0.001f), "Jump ignored custom stamina capacity");
    player.Reset({100, 602});
    Check(player.Bounds().height == 48 && player.StaminaPercent() == 1, "Respawn lost configured values");

    ConfigStore preferences;
    Check(preferences.Load(tuning, false, true) && preferences.values.maxStamina == GameConfig{}.maxStamina,
          "Legacy user preferences overrode player tuning");
    Check(store.Save(directory.path / "preferences.yaml", true), "Preference save failed");
    Check(preferences.Load(directory.path / "preferences.yaml")
          && preferences.values.startSpeed == GameConfig{}.startSpeed,
          "Runtime preferences saved gameplay tuning");
    {
        std::ofstream file(tuning);
        file << "player:\n  max_stamina: 0\n  crouch_height_multiplier: 2\n"
             << "  exhaustion_recovery_threshold: 1.1\n  low_stamina_threshold: -0.1\n  ground_acceleration: -5\n";
    }
    Check(!store.Load(tuning) && store.values.maxStamina == 160 && store.values.groundAcceleration == 1300
          && Near(store.values.exhaustionRecoveryThreshold, 0.25f) && Near(store.values.lowStaminaThreshold, 0.4f),
          "Invalid tuning replaced valid values");
}

void TestSlidingAndCrouch() {
    GameConfig config;
    Player moving;
    moving.Reset({100, 606}, config);
    Advance(moving, config, 60, {.move = 1});
    Player coasting = moving;
    Player braking = moving;
    Advance(coasting, config, 12);
    Advance(braking, config, 12, {.move = -1});
    Check(coasting.Velocity().x > 0 && coasting.Position().x > moving.Position().x,
          "Releasing movement no longer preserves the slide");
    Check(braking.Velocity().x < coasting.Velocity().x, "Counter-strafing did not brake faster than coasting");
    Advance(braking, config, 12, {.move = -1});
    Check(braking.Velocity().x < 0, "Counter-strafing did not reverse direction");

    const float feet = moving.Bounds().y + moving.Bounds().height;
    moving.Update(tick, {.move = 1, .duck = true}, config, floor, world);
    Check(moving.Crouching() && moving.Grounded() && Near(moving.Bounds().height, config.standingHeight / 2)
          && Near(moving.Bounds().width, config.standingWidth)
          && Near(moving.Bounds().y + moving.Bounds().height, feet), "Crouch did not halve the feet-anchored hitbox");
    Check(moving.Velocity().x > 0 && moving.Velocity().x <= config.moveSpeed * config.crouchSpeedMultiplier,
          "Crouch did not limit existing horizontal momentum");

    Player crouched;
    crouched.Reset({100, 606}, config);
    Advance(crouched, config, 120, {.duck = true});
    Check(Near(crouched.StaminaPercent() * config.maxStamina, config.maxStamina - config.crouchStaminaDrain),
          "Stationary crouch did not slowly drain stamina");
    const float before = crouched.StaminaPercent() * config.maxStamina;
    Advance(crouched, config, 120, {.move = 1, .duck = true});
    Check(Near(crouched.Velocity().x, config.moveSpeed * config.crouchSpeedMultiplier)
          && Near(crouched.StaminaPercent() * config.maxStamina,
                  before - config.crouchStaminaDrain),
          "Crouch movement speed or stamina drain is wrong");

    const std::vector<Rectangle> tunnel = {floor[0], {80, 600, 80, 26}};
    crouched.Reset({100, 606}, config);
    crouched.Update(tick, {.duck = true}, config, tunnel, world);
    crouched.Update(tick, {}, config, tunnel, world);
    Check(crouched.Crouching() && crouched.Bounds().y >= 626, "Standing up clipped into a ceiling");
    for (int i = 0; i < 240; ++i) crouched.Update(tick, {.move = 1}, config, tunnel, world);
    Check(!crouched.Crouching() && Near(crouched.Bounds().height, config.standingHeight)
          && crouched.Grounded(), "Player did not stand after leaving the low ceiling");
}

void TestJumpCostsAndRepetition() {
    GameConfig config;
    Player regular;
    regular.Reset({100, 606}, config);
    regular.Update(tick, {.jump = true}, config, floor, world);
    Check(Near(regular.StaminaPercent() * config.maxStamina, config.maxStamina - config.jumpStaminaCost),
          "Jump did not charge its cost at takeoff");
    const float afterJump = regular.StaminaPercent();
    Advance(regular, config, 12);
    Check(Near(regular.StaminaPercent(), afterJump, 0.001f), "Airborne time charged an additional jump cost");

    Player boosted;
    boosted.Reset({100, 606}, config);
    boosted.Update(tick, {.duck = true}, config, floor, world);
    const float before = boosted.StaminaPercent() * config.maxStamina;
    boosted.Update(tick, {.jump = true}, config, floor, world); // Release crouch and jump together.
    Check(Near(boosted.Velocity().y, -config.jumpSpeed * config.crouchJumpMultiplier + config.gravity * tick)
          && Near(boosted.StaminaPercent() * config.maxStamina, before - config.crouchJumpStaminaCost),
          "Leaving crouch did not boost jump speed and pay the larger cost");

    config.staminaRegen = 0;
    Player hopping;
    hopping.Reset({100, 606}, config);
    Advance(hopping, config, 1200, {.jump = true});
    Check(hopping.Grounded() && hopping.StaminaPercent() * config.maxStamina < config.jumpStaminaCost
          && hopping.StaminaPercent() * config.maxStamina > 0,
          "Held jump did not repeat until the next jump was unaffordable");
    const float leftover = hopping.StaminaPercent();
    Advance(hopping, config, 120, {.jump = true});
    Check(hopping.Grounded() && hopping.StaminaPercent() == leftover, "Unaffordable jumps spent stamina");

    Player cloudHopping;
    cloudHopping.Reset({100, 606}, config);
    const std::vector<Rectangle> clouds = {{0, 650, 20000, 20}};
    for (int i = 0; i < 240; ++i) cloudHopping.Update(tick, {.jump = true}, config, {}, world, clouds);
    Check(cloudHopping.StaminaPercent() * config.maxStamina <= config.maxStamina - 3 * config.jumpStaminaCost,
          "Held jump did not repeat on one-way platforms");
}

void TestLowStaminaSpeed() {
    GameConfig config;
    config.maxStamina = 200;
    config.staminaRegen = 0;
    config.crouchStaminaDrain = 0;
    const auto atStamina = [&](float remaining) {
        GameConfig spending = config;
        spending.jumpStaminaCost = config.maxStamina - remaining;
        Player player;
        player.Reset({1000, 606}, spending);
        player.Update(tick, {.jump = true}, spending, floor, world);
        Advance(player, spending, 120);
        Check(player.Grounded() && !player.Exhausted(), "Low-stamina fixture must never hit zero");
        return player;
    };

    // Exercise both sides and the exact boundary, with a custom capacity and threshold too.
    for (const float threshold: {0.3f, 0.45f}) {
        config.lowStaminaThreshold = threshold;
        for (const float offset: {-0.01f, 0.0f, 0.01f}) {
            const Player initial = atStamina(config.maxStamina * (threshold + offset));
            Check(initial.LowStamina(config) == (offset < 0), "Low stamina did not match the red threshold");
            for (const bool crouch: {false, true}) {
                for (const float direction: {-1.0f, 1.0f}) {
                    Player player = initial;
                    Advance(player, config, 60, {.move = direction, .duck = crouch});
                    const float expected = direction * config.moveSpeed
                        * (crouch ? config.crouchSpeedMultiplier : 1.0f)
                        * (offset < 0 ? config.exhaustedSpeedMultiplier : 1.0f);
                    Check(Near(player.Velocity().x, expected),
                          "Walking/crouch speed did not follow the red threshold before reaching zero");
                }
            }
        }
    }

    config.lowStaminaThreshold = 0.3f;
    Player recovering = atStamina(40); // 20%: above exhaustion recovery, still red.
    config.staminaRegen = 20;
    Advance(recovering, config, 60, {.move = 1});
    Check(recovering.LowStamina(config) && !recovering.Exhausted()
          && recovering.StaminaPercent() > 0.2f
          && Near(recovering.Velocity().x, config.moveSpeed * config.exhaustedSpeedMultiplier),
          "Walking recovery cleared the slowdown while stamina was still red");
    Advance(recovering, config, 120, {.move = 1});
    Check(!recovering.LowStamina(config) && Near(recovering.Velocity().x, config.moveSpeed),
          "Walking did not regain normal speed after recovering past the red threshold");

    config.staminaRegen = 0;
    Player jumping;
    jumping.Reset({100, 606}, config);
    Advance(jumping, config, 60, {.move = 1});
    config.jumpStaminaCost = 150;
    jumping.Update(tick, {.move = 1, .jump = true}, config, floor, world);
    Check(jumping.LowStamina(config) && !jumping.Exhausted()
          && Near(jumping.Velocity().x, config.moveSpeed * config.exhaustedSpeedMultiplier),
          "A jump into the red range did not immediately slow existing movement");
}

void TestExhaustionAndClimb() {
    GameConfig config;
    config.crouchStaminaDrain = 1000;
    config.exhaustionRecoveryThreshold = 0.25f;
    config.staminaRegen = 10;
    Player player;
    player.Reset({100, 606}, config);
    Advance(player, config, 24, {.duck = true});
    Check(player.Exhausted() && player.StaminaPercent() == 0, "Stamina depletion did not trigger exhaustion");
    Advance(player, config, 12, {.move = 1});
    Check(player.Exhausted() && Near(player.StaminaPercent() * config.maxStamina, 1.0f)
          && Near(player.Velocity().x, config.moveSpeed * config.exhaustedSpeedMultiplier),
          "Exhausted walking did not regenerate stamina while slowing movement");
    Advance(player, config, 120, {.move = 1});
    Check(player.StaminaPercent() > 0 && player.StaminaPercent() < config.exhaustionRecoveryThreshold
          && player.Exhausted(), "Exhaustion cleared before its recovery threshold");
    Advance(player, config, 360, {.move = 1});
    Check(!player.Exhausted(), "Walking did not recover enough stamina to clear exhaustion");
    Advance(player, config, 60, {.move = 1});
    Check(Near(player.Velocity().x, config.moveSpeed), "Recovered player did not regain regular speed");
    Advance(player, config, 1200, {.move = 1});
    Check(player.StaminaPercent() == 1, "Walking depleted stamina instead of recovering to full");

    config.climbStaminaDrain = 100;
    const std::vector<Rectangle> wall = {floor[0], {150, -1500, 30, 2150}};
    player.Reset({120, 606}, config);
    for (int i = 0; i < 60; ++i) player.Update(tick, {.climb = true}, config, wall, world);
    Check(player.Climbing() && Near(player.StaminaPercent(), 0.5f, 0.001f), "Climbing ignored its drain rate");
    for (int i = 0; i < 70; ++i) player.Update(tick, {.climb = true}, config, wall, world);
    Check(player.Exhausted() && !player.Climbing() && player.Velocity().y > 0,
          "Depleted climber did not release the wall and fall");
    player.Reset({120, 606});
    Check(!player.Exhausted() && !player.Crouching() && player.StaminaPercent() == 1,
          "Respawn retained exhaustion or crouch");
}

void TestLevelClimbingRestrictions() {
    GameConfig config;
    for (const auto &source: LevelCatalog()) {
        auto definition = source;
        definition.world = world;
        definition.spawn = {120, 606};
        definition.objects = {
            {.id = "floor", .type = ObjectType::Static, .collision = CollisionType::Solid,
             .position = {0, 650}, .size = {20000, 50}},
            {.id = "wall", .type = ObjectType::Static, .collision = CollisionType::Solid,
             .position = {150, -1500}, .size = {30, 2150}},
        };
        Level level(definition);
        level.Reset(config);
        level.Update(0.1f, {.climb = true}, config);
        const bool allowed = definition.id == LevelId::Level2;
        Check(level.CanClimb() == allowed && level.GetPlayer().Climbing() == allowed,
              "Climbing must be enabled only in Level 2");
        if (!allowed) {
            Check(level.GetPlayer().Grounded() && level.GetPlayer().StaminaPercent() == 1,
                  "Disabled climbing moved the player or spent stamina");
        }
        // CaptureInput sends both flags for W/Up; disabling climbing must preserve jumping.
        level.Reset(config);
        level.Update(tick, {.jump = true, .climb = true}, config);
        Check(level.GetPlayer().Climbing() == allowed && level.GetPlayer().Velocity().y < 0,
              "W/Up did not retain jumping outside Level 2 or climbing priority within it");
    }
}
}

void TestPlayerMovement() {
    TestTuningConfig();
    TestSlidingAndCrouch();
    TestJumpCostsAndRepetition();
    TestLowStaminaSpeed();
    TestExhaustionAndClimb();
    TestLevelClimbingRestrictions();
    std::cout << "PASS: configurable player tuning, counter-strafing, crouch clearance, jump costs, "
                 "held jumps, red-threshold slowdown, walking recovery, exhaustion and Level 2 climbing restriction\n";
}
}
