#include "levels/level.h"
#include "objects/moving_object.h"
#include "objects/object_manager.h"
#include <iostream>
#include <stdexcept>

namespace rage {
namespace {
void Check(bool condition, const char *message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

GameObjectDefinition StaticDefinition(const char *id, float x, float y, float width, float height) {
    return GameObjectDefinition{.id = id,
                                .type = ObjectType::Static,
                                .collision = CollisionType::Solid,
                                .position = {x, y},
                                .size = {width, height},
                                .assetId = "platform"};
}

// A bare Level subclass, so the object tests can drive Level's
// hazard / trigger / event wiring without a stage's scripted events.
class ObjectTestLevel final : public Level {
public:
    explicit ObjectTestLevel(const LevelDefinition &definition) : Level(definition) {
    }
    using Level::Events;
    using Level::MutablePlayer;
};

void TestManagerAndObjects() {
    ObjectManager manager;
    auto *floor = manager.Create(StaticDefinition("floor", 0, 650, 1500, 50));
    Check(floor != nullptr, "ObjectManager did not create a static object");
    Check(manager.Create(StaticDefinition("floor", 0, 0, 10, 10)) == nullptr,
          "ObjectManager allowed a duplicate id");
    Check(manager.Get("floor") == floor, "ObjectManager lookup by id failed");
    Check(manager.Get("missing") == nullptr, "ObjectManager lookup found a missing id");
    Check(floor->Bounds().width == 1500 && floor->CollisionBounds().width == 1500,
          "Static object bounds are incorrect");
    Check(floor->AssetId() == "platform" && floor->State() == "idle",
          "Static object did not keep its Asset ID and initial state");

    // Collision bounds are independent of visual size.
    auto *smallHitbox = manager.Create(GameObjectDefinition{
        .id = "small-hitbox", .type = ObjectType::Static, .collision = CollisionType::Solid,
        .position = {100, 100}, .size = {100, 100}, .collisionOffset = {25, 25}, .collisionSize = {50, 50},
        .assetId = "platform"});
    Check(smallHitbox->Bounds().width == 100 && smallHitbox->CollisionBounds().width == 50 &&
              smallHitbox->CollisionBounds().x == 125,
          "Collision bounds did not use the configured offset and size");

    // Moving objects ping-pong within their range and reset.
    auto *platform = manager.Create(GameObjectDefinition{
        .id = "moving", .type = ObjectType::Moving, .collision = CollisionType::Solid,
        .position = {0, 100}, .size = {100, 20}, .velocity = {100, 0}, .range = {200, 0},
        .assetId = "platform"});
    Check(platform != nullptr, "ObjectManager did not create a moving object");
    platform->Update(1.0f);
    Check(platform->Position().x == 100, "Moving object did not move");
    platform->Update(1.5f);
    Check(platform->Position().x == 200 && platform->Velocity().x < 0,
          "Moving object did not reverse at the end of its range");
    platform->Reset();
    Check(platform->Position().x == 0 && platform->Velocity().x == 100,
          "Reset did not restore a moving object");

    // Falling objects stay put until triggered, then land and stop.
    auto *anvil = manager.Create(GameObjectDefinition{
        .id = "anvil", .type = ObjectType::Moving, .collision = CollisionType::Hazard,
        .position = {0, 0}, .size = {40, 40}, .velocity = {0, 200}, .range = {0, 100}, .falls = true,
        .assetId = "anvil"});
    auto *falling = dynamic_cast<FallingObject *>(anvil);
    Check(falling != nullptr, "Falling definition did not create a FallingObject");
    falling->Update(1.0f);
    Check(falling->Position().y == 0 && !falling->HasLanded(), "Falling object moved before it was triggered");
    Check(!manager.StartFall("missing") && !manager.StartFall("floor"),
          "StartFall accepted a missing id or a non-falling object");
    Check(manager.StartFall("anvil"), "StartFall did not start the anvil");
    Check(anvil->State() == "fall", "Starting a fall did not select the fall animation state");
    falling->Update(1.0f);
    Check(falling->Position().y == 100 && falling->HasLanded(), "Falling object did not land at its range");
    Check(falling->TakeActionCompleted(), "Landing did not mark the action complete");
    Check(!falling->TakeActionCompleted(), "Action completion was reported twice");
    falling->Reset();
    Check(falling->Position().y == 0 && !falling->HasLanded(), "Reset did not restore a falling object");

    // Event-facing interface: activate / deactivate / state / spawn.
    Check(manager.IsActive("floor") && !manager.IsActive("missing"), "IsActive check is wrong");
    Check(manager.Deactivate("floor") && !manager.IsActive("floor"), "Deactivate did not deactivate");
    Check(manager.Activate("floor") && manager.IsActive("floor"), "Activate did not reactivate");
    Check(!manager.Activate("missing") && !manager.Deactivate("missing"),
          "Activate/Deactivate accepted a missing id");
    Check(manager.SetState("floor", "walk") && floor->State() == "walk", "SetState did not change the state");
    floor->Reset();
    Check(floor->State() == "idle", "Reset did not restore the initial animation state");
    Check(manager.Spawn(StaticDefinition("spawned", 0, 0, 10, 10)) != nullptr &&
              manager.Get("spawned") != nullptr,
          "Spawn did not create an object from a definition");

    // Manager queries: only solid objects collide with the player,
    // and hazards / triggers are found by overlap.
    Check(manager.SolidBounds().size() == 4, "SolidBounds did not list the solid objects");
    Check(manager.Overlapping({0, 640, 10, 20}, CollisionType::Solid).size() == 1,
          "Overlapping did not find the floor");
    Check(manager.Overlapping({0, 0, 10, 10}, CollisionType::Hazard).size() == 1,
          "Overlapping did not find the hazard");
    Check(manager.Remove("small-hitbox") && manager.Get("small-hitbox") == nullptr,
          "Remove did not delete the object");
    Check(!manager.Remove("missing"), "Remove reported success for a missing id");
    manager.ResetAll();
    Check(platform->Position().x == 0, "ResetAll did not reset the objects");
    manager.Clear();
    Check(manager.Objects().empty() && manager.SolidBounds().empty(), "Clear did not remove the objects");
}

void TestAssetIntegration() {
    // No Game Data rows are loaded here, so every named asset draws
    // its placeholder — at the same bounds, with collision untouched.
    // Invisible objects (empty Asset ID) queue nothing at all.
    AssetManager assets;
    ObjectManager manager;
    manager.Create(StaticDefinition("floor", 0, 650, 1500, 50));
    manager.Create(GameObjectDefinition{.id = "zone", .type = ObjectType::Trigger,
                                        .collision = CollisionType::Trigger, .position = {100, 100},
                                        .size = {50, 50}, .assetId = ""});
    manager.Create(GameObjectDefinition{.id = "hidden", .type = ObjectType::Static,
                                        .collision = CollisionType::Solid, .position = {0, 0},
                                        .size = {10, 10}, .assetId = "platform", .visible = false});
    // No row and drawPlaceholder off: the level draws its own stand-in, so nothing is queued.
    manager.Create(GameObjectDefinition{.id = "stand-in", .type = ObjectType::Static,
                                        .collision = CollisionType::None, .position = {0, 0},
                                        .size = {10, 10}, .assetId = "car", .drawPlaceholder = false});
    RenderQueue queue;
    bool placeholderCalled = false;
    Rectangle placeholderBounds{};
    manager.SubmitSprites(queue, assets, [&](Rectangle bounds, bool) {
        placeholderCalled = true;
        placeholderBounds = bounds;
    });
    Check(queue.Size() == 1,
          "SubmitSprites queued an invisible, hidden or stand-in object, or skipped the visible one");
    queue.Flush();
    Check(placeholderCalled && placeholderBounds.width == 1500 && placeholderBounds.height == 50,
          "Placeholder was not drawn at the object's bounds when its Game Data row is missing");
}

void TestLevelIntegration() {
    GameConfig config;
    config.charactersPerSecond = 8;
    auto definition = Level0Definition();
    definition.objects = {
        {.id = "floor", .type = ObjectType::Static, .collision = CollisionType::Solid,
         .position = {0, 650}, .size = {1500, 50}, .assetId = "platform"},
        {.id = "checkpoint-1", .type = ObjectType::Trigger, .collision = CollisionType::Trigger,
         .position = {200, 600}, .size = {60, 50}},
        {.id = "hazard-1", .type = ObjectType::Hazard, .collision = CollisionType::Hazard,
         .position = {500, 600}, .size = {60, 50}, .assetId = "spikes"},
        {.id = "exit-1", .type = ObjectType::Trigger, .collision = CollisionType::Trigger,
         .position = {1200, 600}, .size = {60, 50}},
    };
    ObjectTestLevel scene(definition);
    Check(scene.Objects().SolidBounds().size() == 1, "Level did not create its objects from the definition");
    int checkpointSeen = 0;
    scene.Events().RegisterEvent({
        .id = "checkpoint-seen",
        .execute = [&] { ++checkpointSeen; },
        .matchesNotification = [](const EventNotification &notice) {
            return notice.topic == objectEvents::triggerEntered &&
                   notice.sourceId == ObjectNotificationId("checkpoint-1");
        },
        .repeatable = true,
    });
    scene.Reset(config);

    // Entering the checkpoint sets the respawn point and notifies.
    scene.MutablePlayer().Reset({200, 606});
    scene.Update(0.016f, {}, config);
    Check(scene.RespawnPoint().x == 200 && scene.RespawnPoint().y == 600,
          "Checkpoint did not set the respawn point");
    Check(checkpointSeen == 1, "Trigger entry did not notify the EventManager");

    // A hazard respawns the player at the checkpoint, not the spawn.
    scene.MutablePlayer().Reset({500, 606});
    scene.Update(0.016f, {}, config);
    Check(scene.GetPlayer().Position().x == 200 && scene.GetPlayer().Position().y == 600,
          "Hazard did not respawn the player at the checkpoint");

    // The exit completes the level.
    scene.MutablePlayer().Reset({1200, 606});
    scene.Update(0.016f, {}, config);
    Check(scene.IsComplete(), "Level exit did not complete the level");

    // Restart clears completion, respawn and object state.
    scene.Update(0, {.resetPlayer = true}, config);
    Check(!scene.IsComplete() && scene.RespawnPoint().x == definition.spawn.x,
          "Level reset did not clear completion and respawn");
}
void TestShortcuts() {
    // SolidPlatform and ArtObject must build the same definitions as the full form.
    const auto platform = SolidPlatform("step", {350, 550, 180, 24});
    Check(platform.id == "step" && platform.type == ObjectType::Static &&
          platform.collision == CollisionType::Solid && platform.position.x == 350 &&
          platform.position.y == 550 && platform.size.x == 180 && platform.size.y == 24 &&
          platform.assetId == "platform" && platform.drawPlaceholder,
          "SolidPlatform built the wrong definition");
    Check(SolidPlatform("wire", {0, 0, 10, 10}, "").assetId.empty(), "SolidPlatform ignored an empty Asset ID");
    const auto art = ArtObject("car", {760, 1428, 108, 52}, "car");
    Check(art.collision == CollisionType::None && art.assetId == "car" && !art.drawPlaceholder &&
          art.size.x == 108 && art.size.y == 52, "ArtObject built the wrong definition");
    // Level 0's objects are still four solid platforms, with the floor where it was.
    const auto &level0 = Level0Definition().objects;
    Check(level0.size() == 4 && level0[0].id == "floor" && level0[0].position.y == 650 &&
          level0[0].size.x == 1500 && level0[0].collision == CollisionType::Solid,
          "Level 0's objects changed");
}

void TestLevel1ArtObjects() {
    // Level 1 moves its cars with its own script. Their art-only objects must
    // follow those hitboxes, add no collision, and go back on restart.
    GameConfig config;
    auto level = CreateLevel(LevelId::Level1, config);
    for (const char *id : {"trash-can", "hydrant", "car-1", "car-2", "car-3", "bird-1", "bird-2",
                           "flying-pig", "chicken"}) {
        const auto *object = level->Objects().Get(id);
        Check(object && object->Collision() == CollisionType::None && !object->AssetId().empty(),
              "Level 1 art object is missing, collides, or has no Asset ID");
    }
    const float carStart = level->Objects().Get("car-1")->Position().x;
    const float birdStart = level->Objects().Get("bird-1")->Position().x;
    level->Update(0.5f, {}, config);
    Check(level->Objects().Get("car-1")->Position().x < carStart &&
          level->Objects().Get("bird-1")->Position().x < birdStart,
          "Level 1 art objects did not follow the scripted cars and birds");
    level->Update(0, {.resetPlayer = true}, config);
    Check(level->Objects().Get("car-1")->Position().x == carStart, "Restart did not move the car art back");
}
}  // namespace

void TestObjects() {
    TestManagerAndObjects();
    TestAssetIntegration();
    TestLevelIntegration();
    TestShortcuts();
    TestLevel1ArtObjects();
    std::cout << "PASS: game objects, manager, AssetManager placeholders, EventManager notifications, "
                 "hazards, checkpoints, level completion, shortcuts and Level 1 art objects\n";
}
}  // namespace rage
