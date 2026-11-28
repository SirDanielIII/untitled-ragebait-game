#include "test_support.h"
#include "engine/collision/collision_system.h"
#include "objects/moving_object.h"

#include <algorithm>
#include <fstream>
#include <iostream>

namespace rage {
namespace {
using test::Check;
using test::Near;
Collider Box(const char *id, Rectangle bounds, CollisionType type = CollisionType::Solid) {
    return {id, bounds, bounds, type};
}
bool Has(const CollisionResult &result, const char *id) {
    return std::ranges::any_of(result.contacts, [&](const auto &c) { return c.objectId == id; });
}

void TestDebugObstacles() {
    const Rectangle bounds{10, 20, 30, 40};
    for (auto type : {CollisionType::Solid, CollisionType::OneWay, CollisionType::Hazard}) {
        Check(Box("obstacle", bounds, type).IsDebugObstacle(), "Physical obstacle omitted from debug overlay");
    }
    Check(!Box("art", bounds, CollisionType::None).IsDebugObstacle(), "Decorative object shown in debug overlay");
    auto bird = Box("bird", bounds, CollisionType::Trigger);
    Check(!bird.IsDebugObstacle(), "Ordinary trigger shown in obstacle overlay");
    bird.obstacleSensor = true;
    Check(bird.IsDebugObstacle(), "Non-blocking bird obstacle omitted from overlay");
    bird.type = CollisionType::None;
    Check(!bird.IsDebugObstacle(), "Disabled obstacle sensor shown in debug overlay");
    Check(!Box("empty", {10, 20, 0, 40}).IsDebugObstacle(), "Empty collider shown in debug overlay");
}

void TestSweeps() {
    CollisionSystem system;
    system.SetColliders({Box("floor", {-100, 100, 300, 1}), Box("ceiling", {-100, -1, 300, 1}),
                         Box("left", {-1, 0, 1, 100}), Box("right", {100, 0, 1, 100})});
    for (auto velocity : {Vector2{20000, 0}, Vector2{-20000, 0}, Vector2{0, 20000}, Vector2{0, -20000}}) {
        Rectangle body{40, 40, 10, 10};
        const auto result = system.Move(body, velocity, 0.1f);
        Check(body.x >= 0 && body.x <= 90 && body.y >= 0 && body.y <= 90, "High speed tunneled through a thin surface");
        Check(velocity.x == 0 && velocity.y == 0 && !result.contacts.empty(), "Impact did not stop velocity or identify the obstacle");
    }
    Rectangle embedded{97, 40, 10, 10};
    Vector2 still{};
    system.Move(embedded, still, 0);
    Check(!CollisionSystem::Overlaps(embedded, {100, 0, 1, 100}), "Stationary overlap was not resolved");

    system.SetColliders({Box("cloud", {0, 50, 100, 1}, CollisionType::OneWay)});
    Rectangle body{20, 60, 10, 10};
    Vector2 velocity{0, -1000};
    system.Move(body, velocity, 0.1f);
    Check(body.y < 0 && velocity.y == -1000, "One-way platform blocked an upward crossing");
    velocity = {0, 1000};
    Check(system.Move(body, velocity, 0.1f).grounded && Near(body.y, 40), "One-way platform missed a fast landing");
}

void TestMovingSolids() {
    for (const Vector2 travel : {Vector2{30, 0}, Vector2{0, 30}, Vector2{0, -30}}) {
        CollisionSystem system;
        system.SetColliders({{"lift", {travel.x, 100 + travel.y, 100, 10}, {0, 100, 100, 10}}});
        Rectangle body{20, 90, 10, 10};
        Vector2 velocity{0, 10};
        const auto result = system.Move(body, velocity, 0.1f);
        Check(result.grounded && Near(body.x, 20 + travel.x) && Near(body.y, 90 + travel.y), "Moving platform did not carry its rider");
        body = {20, 90, 10, 10}; velocity = {0, -500};
        system.Move(body, velocity, 0.1f);
        Check(Near(body.x, 20) && body.y < 90, "Jumping actor remained attached to platform");
    }
    CollisionSystem system;
    system.SetColliders({{"pusher", {200, 0, 10, 100}, {0, 0, 10, 100}}});
    Rectangle body{50, 20, 10, 10}; Vector2 velocity{};
    const auto push = system.Move(body, velocity, 0.1f);
    Check(Near(body.x, 210) && Has(push, "pusher"), "Fast moving obstacle passed through a stationary actor");
    system.SetColliders({{"pusher", {60, 0, 20, 100}, {0, 0, 20, 100}}, Box("wall", {70, 0, 20, 100})});
    body = {50, 20, 10, 10}; velocity = {};
    Check(system.Move(body, velocity, 0.1f).crushed, "Impossible moving-solid squeeze was not reported");

    ObjectManager objects;
    objects.Create({.id = "floor", .position = {-100, 100}, .size = {300, 10}});
    auto *fall = objects.Create({.id = "falling", .type = ObjectType::Moving, .position = {0, 0}, .size = {10, 10},
        .velocity = {0, 2000}, .range = {0, 500}, .falls = true, .collideWithEnvironment = true});
    objects.StartFall("falling");
    objects.UpdateAll(0.1f);
    Check(Near(fall->Position().y, 90) && dynamic_cast<FallingObject *>(fall)->HasLanded(), "Falling solid ignored environment geometry");
    Check(fall->TakeActionCompleted() && !fall->TakeActionCompleted(), "Environment landing repeated its completion event");

    GameConfig config;
    system.SetColliders({{"lift", {30, 100, 100, 10}, {0, 100, 100, 10}}});
    Player rider;
    rider.Reset({20, 56}, config);
    rider.Update(0.1f, {}, config, system);
    Check(Near(rider.Position().x, 50) && rider.Grounded(), "Player substeps applied a platform's frame motion more than once");

    GameObjectDefinition path{.id = "ping-pong", .type = ObjectType::Moving, .position = {0, 0},
        .size = {10, 10}, .velocity = {100, 0}, .range = {25, 0}};
    MovingObject whole(path), partitioned(path);
    whole.Update(0.7f);
    for (int i = 0; i < 7; ++i) partitioned.Update(0.1f);
    Check(Near(whole.Position().x, partitioned.Position().x) && whole.Velocity().x == partitioned.Velocity().x,
          "Moving path lost travel time at reversals");
}

void TestSensors() {
    CollisionSystem system;
    system.SetColliders({Box("trigger", {30, 0, 1, 100}, CollisionType::Trigger),
        Box("hazard", {60, 0, 1, 100}, CollisionType::Hazard), Box("decor", {10, 0, 1, 100}, CollisionType::None)});
    Rectangle body{0, 10, 10, 10}; Vector2 velocity{1000, 0};
    auto result = system.Move(body, velocity, 0.1f);
    Check(Near(body.x, 100) && Has(result, "trigger") && Has(result, "hazard") && !Has(result, "decor"), "Swept sensors blocked movement or missed a crossing");
    Check(system.TrackContacts("actor", result.contacts).size() == 2, "Sensor entry was not reported");
    Check(system.TrackContacts("actor", result.contacts).empty(), "Continuous contact emitted duplicate entry events");
    const auto exits = system.TrackContacts("actor", {});
    Check(exits.size() == 2 && exits[0].phase == ContactPhase::Exit, "Sensor exit was not reported");
    Check(system.TrackContacts("actor", result.contacts).size() == 2, "Sensor did not rearm after exit");

    auto filtered = Box("filtered", {20, 0, 10, 100}); filtered.filter.mask = collisionLayers::environment;
    system.SetColliders({filtered}); body = {0, 10, 10, 10}; velocity = {1000, 0};
    Check(system.Move(body, velocity, 0.1f).contacts.empty() && Near(body.x, 100), "Collision masks were ignored");
    system.SetColliders({Box("wall", {20, 0, 10, 100}), Box("behind-wall", {40, 0, 10, 100}, CollisionType::Hazard)});
    body = {0, 10, 10, 10}; velocity = {1000, 0};
    Check(!Has(system.Move(body, velocity, 0.1f), "behind-wall"), "Sensor detected an unreachable position beyond a solid");
    system.SetColliders({{"fast-hazard", {100, 0, 1, 100}, {0, 0, 1, 100}, CollisionType::Hazard}});
    body = {50, 10, 10, 10}; velocity = {};
    Check(Has(system.Move(body, velocity, 0.1f), "fast-hazard") && Near(body.x, 50),
          "Moving hazard crossed a stationary player undetected");
}

class CollisionLevel : public Level {
public:
    using Level::Level;
    using Level::MutablePlayer;
    using Level::Events;
};

class ScriptCollisionLevel : public CollisionLevel {
public:
    using CollisionLevel::CollisionLevel;
    Rectangle shape{100, 0, 100, 20};
    bool enabled = false;
    bool entered = false;
private:
    void OnUpdate(float, const InputFrame &) override {
        entered = ScriptContact("script-hazard", shape, CollisionType::Hazard, enabled);
    }
};

void TestIntegration() {
    GameConfig config;
    config.maxHealth = 3;
    auto definition = Level0Definition();
    definition.objects = {{.id = "damage", .collision = CollisionType::Hazard, .position = {100, 590},
                           .size = {100, 100}, .damage = 1, .lethal = false},
                          {.id = "floor", .position = {0, 650}, .size = {1500, 20}}};
    CollisionLevel level(definition);
    int hits = 0;
    level.Events().RegisterEvent({.id = "hit", .execute = [&] { ++hits; },
        .matchesNotification = [](const auto &e) { return e.topic == objectEvents::hazardHit && e.objectId == "damage"; }, .repeatable = true});
    level.Reset(config);
    for (int i = 0; i < 60; ++i) level.Update(1.0f / 60, {}, config);
    Check(level.GetPlayer().GetHealth() == 2 && hits == 1 && level.GetPlayer().LastDamageSource() == "damage", "Hazard damaged every frame or lost its source ID");
    level.MutablePlayer().Reset({300, 606}); level.Update(0.02f, {}, config);
    level.MutablePlayer().Reset({100, 606}); level.Update(0.02f, {}, config);
    Check(hits == 2, "Hazard did not rearm after leaving");
    level.Objects().Create({.id = "once", .collision = CollisionType::Trigger,
        .position = {100, 590}, .size = {100, 100}, .triggerOnce = true});
    level.Update(0.02f, {}, config);
    level.Update(0.02f, {}, config);
    Check(level.Objects().IsTriggered("once") && !level.Objects().IsActive("once"),
          "Exiting an inactive one-shot trigger cleared its completion state");
    definition.objects.clear();
    CollisionLevel falling(definition); falling.Reset(config);
    for (int i = 0; i < 120 && falling.GetPlayer().LastDamageSource().empty(); ++i) falling.Update(1.0f / 60, {}, config);
    Check(falling.GetPlayer().LastDamageSource() == "world-fall", "Bottom boundary clamped instead of triggering a kill zone");
    for (const auto &source : LevelCatalog()) {
        for (const int fps : {30, 60, 144, 240}) {
            auto scene = CreateLevel(source.id, config);
            for (int i = 0; i < fps; ++i) scene->Update(1.0f / fps, {}, config);
            Check(scene->GetPlayer().Grounded(), "Level spawn did not remain grounded across frame rates");
        }
    }
    Vector2 baseline{};
    for (const int fps : {15, 30, 60, 144, 240}) {
        auto movingDefinition = Level0Definition();
        movingDefinition.spawn = {100, 606};
        movingDefinition.objects = {{.id = "lift", .type = ObjectType::Moving, .position = {50, 650},
                                     .size = {250, 10}, .velocity = {80, -20}, .range = {1000, 0}}};
        CollisionLevel moving(movingDefinition); moving.Reset(config);
        for (int i = 0; i < fps; ++i) moving.Update(1.0f / fps, {}, config);
        const auto position = moving.GetPlayer().Position();
        if (fps == 15) baseline = position;
        Check(Near(position.x, baseline.x, 0.1f) && Near(position.y, baseline.y, 0.1f) && moving.GetPlayer().Grounded(), "Moving platform depends on rendering frame rate");
    }
    config.collisionOffsetX = 7; config.collisionOffsetY = 9;
    Player player; player.Reset({10, 20}, config);
    Check(player.Bounds().x == 17 && player.Bounds().y == 29 && player.Position().x == 10
          && player.VisualBounds().x == 10 && player.VisualBounds().y == 20,
          "Player collision offset moved its sprite or object position");

    config.collisionOffsetX = 0; config.collisionOffsetY = 0;
    auto scriptDefinition = Level0Definition();
    ScriptCollisionLevel scripted(scriptDefinition); scripted.Reset(config);
    scripted.Update(0, {}, config);
    scripted.shape.y = 800; scripted.enabled = true;
    scripted.Update(0, {}, config);
    Check(!scripted.entered, "Activating a hazard swept through its old inactive position");
    scripted.shape.y = 590;
    scripted.Update(0, {}, config);
    Check(scripted.entered && scripted.GetPlayer().LastDamageSource() == "script-hazard",
          "Scripted hazard did not detect its active movement");
    scripted.Update(0, {}, config);
    Check(!scripted.entered, "Scripted hazard repeated an entry during continued contact");

    CollisionLevel instant(scriptDefinition); instant.Reset(config);
    instant.Objects().Create({.id = "instant", .collision = CollisionType::Trigger,
        .position = {100, 590}, .size = {100, 100}});
    instant.Update(0, {}, config);
    Check(instant.Objects().IsTriggered("instant"), "Zero-time update failed to query current contacts");
    instant.MutablePlayer().Reset({300, 606}); instant.Update(0, {}, config);
    Check(!instant.Objects().IsTriggered("instant"), "Zero-time update retained a stale trigger contact");
}

void TestConfigFiles() {
    test::TestDirectory directory;
    ConfigStore store;
    store.values.lastLevel = 3; store.values.moveSpeed = -123; store.values.targetFps = 900;
    store.values.charactersPerSecond = -40; store.values.showHitboxes = true;
    Check(store.SaveFile(directory.path / "settings.yaml", ConfigFile::Settings) &&
          store.SaveFile(directory.path / "config.yaml", ConfigFile::Tuning) &&
          store.SaveFile(directory.path / "game.yaml", ConfigFile::SaveData), "Split YAML files did not save");
    ConfigStore loaded;
    Check(loaded.LoadFile(directory.path / "settings.yaml", ConfigFile::Settings), "Settings did not load");
    Check(loaded.values.lastLevel == 0 && loaded.values.moveSpeed == GameConfig{}.moveSpeed &&
          loaded.values.targetFps == 900 && loaded.values.charactersPerSecond == -40, "Preferences contain save/tuning data or restrict numeric values");
    Check(loaded.LoadFile(directory.path / "config.yaml", ConfigFile::Tuning) && loaded.values.moveSpeed == -123 && loaded.values.showHitboxes, "Tuning/debug values did not load");
    Check(loaded.LoadFile(directory.path / "game.yaml", ConfigFile::SaveData) && loaded.values.lastLevel == 3, "Save data did not load");
    { std::ofstream f(directory.path / "settings.yaml"); f << "audio:\n  master_volume: 1.1\n  music_volume: -1\n  sfx_volume: 0.3\n"; }
    Check(!loaded.LoadFile(directory.path / "settings.yaml", ConfigFile::Settings) && Near(loaded.values.sfxVolume, 0.3f) &&
          loaded.values.masterVolume == GameConfig{}.masterVolume && loaded.values.musicVolume == GameConfig{}.musicVolume, "Volume validation did not preserve valid values");
}
}
void TestCollisions() {
    TestDebugObstacles(); TestSweeps(); TestMovingSolids(); TestSensors(); TestIntegration(); TestConfigFiles();
    std::cout << "PASS: swept AABBs, moving solids, sensors, filters, contact IDs, damage, kill zones, four levels and split YAML\n";
}
}
