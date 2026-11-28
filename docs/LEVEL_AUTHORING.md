# Level development

## Files

Each numbered level has a folder under `game/src/levels/`, containing its layout, dialogue and gameplay code.

The four current levels provide placeholder platforms, movement, camera follow and opening dialogue. Level 0 also includes a scripted hallway sequence. The objectives below describe the intended content; reaching a door or classroom does not currently complete a level.

Paths in this guide are relative to `game/src/` unless shown with a `game/` prefix.

| Stage | File | Planned objective |
| --- | --- | --- |
| Level 0 (tutorial) | `levels/level_0/level_0.cpp` | Leave the house through the front door |
| Level 1 | `levels/level_1/level_1.cpp` | Cross the streets and reach campus |
| Level 2 | `levels/level_2/level_2.cpp` | Climb to the top-floor classroom |
| Level 3 | `levels/level_3/level_3.cpp` | Face the Entity outside the lecture hall |

The `LevelDefinition` at the top of each file contains the title, description, intro, backdrop, world bounds, spawn and objects. The level instance owns the stage's runtime state. `main.cpp` registers definitions and factories for stage selection.

## Loading and switching levels

Open **Dev Level Select** from the main or pause menu to load a stage directly for testing. Its preview includes the stage's `.plannedContent` development notes.

`ui/menus/menus.cpp` creates buttons that send a `StartLevel` action with the chosen `LevelId`. `Game::Apply` in `main.cpp` handles that action: it calls `CreateLevel`, replaces the active scene, and enters the Playing screen. `CreateLevel` in `main.cpp` constructs the registered stage class and calls `Reset`.

Automatic progression to the next level has not been implemented.

## Layout

Edit `.world`, `.spawn` and `.objects` in the stage definition. Coordinates are world pixels. The player's collision box is 30 by 44 pixels.

`.backdrop` selects a background image. Collision geometry comes from `.objects`, independently of the image. Add stage-specific drawing in `DrawWorld(const Resources&) const`; it runs inside the world camera, before objects and the player are drawn.

### Game objects

Levels do not list platform rectangles or write their own object
management. They list `GameObjectDefinition`s in `.objects`, and
`Level` creates them in its `ObjectManager` (see
`game/src/objects/`). The same definition can be reused with
different positions, sizes and speeds; no new class is needed for a
new obstacle.

```cpp
.objects = {
    {.id = "floor", .type = ObjectType::Static, .collision = CollisionType::Solid,
     .position = {0, 650}, .size = {1500, 50}, .assetId = "platform"},
    {.id = "anvil", .type = ObjectType::Moving, .collision = CollisionType::Hazard,
     .position = {400, 200}, .size = {40, 40}, .velocity = {0, 200},
     .range = {0, 400}, .falls = true, .assetId = "anvil"},
    {.id = "checkpoint-1", .type = ObjectType::Trigger, .collision = CollisionType::Trigger,
     .position = {200, 600}, .size = {60, 50}},  // no assetId: intentionally invisible
    {.id = "exit-1", .type = ObjectType::Trigger, .collision = CollisionType::Trigger,
     .position = {1200, 600}, .size = {60, 50}},
},
```

| Field | Meaning |
| --- | --- |
| `id` | Unique instance id, used to look the object up (`Objects().Get(id)`) |
| `type` | `Static`, `Moving`, `Hazard`, `Interactive` or `Trigger` |
| `collision` | `None`, `Solid`, `Hazard` or `Trigger` |
| `position` / `size` | Visual bounds, in world pixels |
| `velocity` / `range` | Moving objects ping-pong within `range`; falling objects fall by `range.y` |
| `falls` | The object only moves once triggered (e.g. an anvil) |
| `collisionOffset` / `collisionSize` | Collision box, relative to position; defaults to `size` |
| `assetId` | Game Data Asset ID, e.g. `"platform"`; empty means intentionally invisible |
| `initialState` | Animation state, e.g. `"idle"`; the animation is `<assetId>_<state>` |
| `active` / `visible` | Enabled and drawn state; both reset on restart |
| `triggerOnce` | The trigger / hazard deactivates after firing once |

The object system provides the shared behaviour; object-specific
mechanics stay in the object class (`MovingObject`,
`FallingObject`), never in the manager or the level:

- **Static:** floors, walls, platforms. Solid collision, no movement.
- **Moving:** horizontal / vertical platforms and obstacles, ping-ponging in their range.
- **Falling:** stays put until `StartFall()` (from a trigger or an event), then falls by `range.y` and stops. Landing posts `objectEvents::actionCompleted`.
- **Hazards:** contact respawns the player at the last checkpoint and posts `objectEvents::hazardHit`. They can be activated / deactivated, once or repeatedly.
- **Interactive / triggers:** checkpoints (`checkpoint-*` sets the respawn point), level exits (`exit-*` completes the level), and invisible trigger zones. Entering one posts `objectEvents::triggerEntered`.

Collision bounds are independent of sprite dimensions, so replacing
artwork never changes gameplay. Moving objects update their bounds
as they move, because `Level` passes `Objects().SolidBounds()` —
rebuilt every frame — to the player.

For art, objects use the AssetManager, not a fixed art slot: each
object keeps an `Animator` for its current state, and `Level`
queues its sprites through `SubmitSprites` on the sheet layer, at
the sheet pivot, with a placeholder when its texture is missing
(see [ART_ASSETS.md](ART_ASSETS.md#game-data-sprites-and-animations)).
Adding a Game Data row for an asset (e.g. `platform_idle`) makes
the finished art appear with no gameplay-code change. An object
with no `assetId` is never drawn.

The current layouts are placeholders. The stairwell needs a blueprint before its climbing sections are designed.

## Stage logic

Use `OnReset()` to initialize stage state and `OnUpdate()` to run it:

```cpp
void OnReset() override
{
    // Reset flags, timers and hazards.
}

void OnUpdate(float dt, const InputFrame& input) override
{
    // Update active hazards and notify Events() about interactions as they occur.
}
```

`dt` is gameplay time in seconds, clamped to 0-0.1 by `GameWindow::Frame`. Store timers, hazard state and other mutable gameplay data as class members.

| Member | Purpose |
| --- | --- |
| `GetPlayer()` | Read position, bounds, velocity and grounded state |
| `MutablePlayer()` | Call player gameplay methods from the stage |
| `Dialogue()` | Read the current message and its reveal state |
| `ElapsedSeconds()` | Read time since the stage was reset |
| `CanSay()` | Check whether a dialogue request can start this frame |
| `Say(DialogueCue)` | Start a message and return its level-local request ID, or zero if busy |
| `RequestSound(id)` | Play an Audio Data sound this frame (scripted moments). Player jump/land/hurt/death sounds are automatic |
| `MusicTrack()` | Override to choose the level's music (default `level_<number>_theme`); changes crossfade. See [audio README](../game/src/engine/audio/README.md) |
| `Events()` | Register events, post notifications and query completion |
| `Objects()` | The level's `ObjectManager`: look objects up by id, spawn / remove / activate / deactivate them, start a fall |
| `ActivateObject(id)` / `DeactivateObject(id)` / `StartObjectFall(id)` | Event-facing object actions; they also post `objectEvents::activated` |
| `IsComplete()` | True once an `exit-*` trigger has been entered |
| `RespawnPoint()` | Last checkpoint position, or the spawn |
| `Definition()` | Read the authored level data |

The update order is dialogue, object movement, player movement, hazard and trigger checks, `OnUpdate`, event evaluation, then camera follow. Pause and Settings suspend scene updates. Restart Stage and R clear event completion and pending notifications, restore the player, elapsed time and opening dialogue, reset all objects, then call `OnReset` and update the camera. Clear stage-specific state in `OnReset`. Opening Main Menu or Dev Level Select destroys the active level, its objects and its events.

Keep update logic out of drawing functions. Use the application's existing frame loop rather than adding a loop inside a stage.

## Scripted events

Each level's constructor registers the `Event` objects returned by `CreateEvents()` once per level instance. Add entries in the level's `level_<number>_events.cpp`, placing prerequisites earlier in the list. Callbacks can capture `this` to call `Say()`, access the player and update the level's timers. The tutorial has working examples in [level_0_events.cpp](../game/src/levels/level_0/level_0_events.cpp).

```cpp
std::vector<Event> Level0::CreateEvents()
{
    return {
        {
            .id = "at-door",
            .condition = [this] { return GetPlayer().Position().x >= 1100.0f && CanSay(); },
            .execute = [this] { doorCue = Say({.text = "Try the handle."}); }
        },
        {
            .id = "door-cue-revealed",
            .execute = [this] { handleEnabled = true; },
            .dependencies = {"at-door"},
            .matchesNotification = [this](const EventNotification& notice) {
                return notice.topic == dialogueEvents::revealCompleted && notice.sourceId == doorCue;
            }
        }
    };
}
```

In this example `doorCue` and `handleEnabled` are level members. See Level 0 for a complete sequence. `Events().IsCompleted(id)` becomes true after an event's callback returns and stays true until the level resets. Add `.repeatable = true` to run again when a condition goes false and then true, or when another matching notification arrives in a later update.

Post notifications with `Events().Notify({"door.interact", doorId})`. Events share the notifications for one update, and each event runs at most once per update. If its prerequisites or condition are unmet when evaluated, it needs a fresh notification. Notifications posted inside event callbacks are processed on the next update.

Objects post their own notifications, so events can react to them
without polling: `objectEvents::triggerEntered`,
`objectEvents::hazardHit`, `objectEvents::activated` and
`objectEvents::actionCompleted`, each with `sourceId` equal to
`ObjectNotificationId(objectId)`. Events act on objects through
`ActivateObject(id)`, `DeactivateObject(id)`, `StartObjectFall(id)`,
`SpawnObject(definition)` and `RemoveObject(id)`, and check their
state with `Objects().IsActive(id)` / `Objects().IsTriggered(id)`:

```cpp
{
    .id = "anvil-drops",
    .execute = [this] { StartObjectFall("anvil"); },
    .matchesNotification = [](const EventNotification& notice) {
        return notice.topic == objectEvents::triggerEntered &&
               notice.sourceId == ObjectNotificationId("anvil-trigger");
    }
},
{
    .id = "anvil-landed",
    .execute = [this] { Say({.text = "That was close."}); },
    .dependencies = {"anvil-drops"},
    .matchesNotification = [](const EventNotification& notice) {
        return notice.topic == objectEvents::actionCompleted &&
               notice.sourceId == ObjectNotificationId("anvil");
    }
}
```

## Dialogue

Edit `.intro` to set the opening message and typewriter options:

```cpp
.intro = {
    .text = "Try the front door.\nIgnore the furniture.",
    .speaker = "The Entity",
    .speedMultiplier = 0.75f,
    .startDelay = 0.4f,
    .fontSize = 24.0f,
    .allowSkip = true,
    .allowReplay = true,
    .effects = {.shakePixels = 2, .shakeHz = 9, .holdSeconds = 3, .fadeSeconds = 1}
},
```

`speedMultiplier` multiplies the text-speed setting: 1 is normal speed, 0.5 is half speed and 2 is double speed. Zero reveals the message immediately and skips the initial delay, as does the player's Instant setting. `Say` clamps multipliers to 0-10, text sizes to 12-48 logical pixels, and negative delays to zero.

Enter reveals the whole message. T replays the last accepted message, even after it fades, and restarts its delay and effects. The cue's flags enable or disable those controls. Changing the text-speed setting takes effect on the current message.

`shakePixels` sets the maximum text displacement in logical pixels (0–8) and `shakeHz` sets the shake rate (0–30). Zero displacement or frequency disables shake. A positive `fadeSeconds` holds the completed text for `holdSeconds`, then fades the panel and text over the specified duration. The default keeps the message visible. These timers use unpaused gameplay time.

Use `CanSay()` in an event condition so it stays eligible until the current cue finishes revealing and any hold/fade has played. Only one ordinary request can start per update. To interrupt a cue intentionally, pass `DialoguePolicy::Interrupt`.

```cpp
Say({.text = "The stairs do that sometimes.", .speedMultiplier = 1.5f}, DialoguePolicy::Interrupt);
```

Each accepted request gets a new `DialogueCue::id`, even for identical text. IDs restart with the introduction on level reset. Query `CurrentDialogueId()` and `Dialogue().IsComplete()` to inspect the current presentation. The level posts `dialogueEvents::revealStarted` when the start delay expires and `dialogueEvents::revealCompleted` when the entire message becomes visible. Match both the topic and `sourceId`. A completion notification carries a `revealMethod` of `Natural`, `Enter`, or `Instant`. Replay keeps the ID and publishes each milestone at most once across all presentations of that request.

## Design notes

The student is sleep deprived and late for class. The Entity gives unreliable advice while the surroundings turn into hallucinated obstacles.

- **Level 0 (house):** falling lights and pipes, floor holes, flooded rooms with electrical hazards, furniture platforming, false gas-leak doors, a fireplace monster.
- **Level 1 (streets):** traffic, cloud jumps, birds, live telephone poles, rainbow slides, a crossing chicken, falling satellites, flying pigs.
- **Level 2 (stairwell):** missing stairs, exposed bricks to climb, falling bricks. Design the level blueprint before expanding these mechanics.
- **Level 3 (lecture hall):** shifting floors, floating desks and chairs, paper airplanes, wall climbing and the Entity blocking entry.

Basic hazards (respawn at the last checkpoint), checkpoints and stage completion via `exit-*` / `IsComplete()` are provided by the object system. Lives, climbing controls and the boss are pending gameplay work.

## Adding a stage

1. Copy a numbered level folder, for example `game/src/levels/level_1/` to `level_4/`. Rename its header, implementation and events file for the new level. Update the includes and use `Level4`, `Level4Definition`, `MakeLevel4` and `LevelId::Level4` throughout.
2. Add an explicit `LevelId` value in `levels/level.h`; preserve the existing numeric values used by saved settings.
3. Declare `Level4Definition` and `MakeLevel4` in `levels/level.h`, then add `{Level4Definition, MakeLevel4}` to `registrations` in `main.cpp`. This list drives both loading and the level selector.
4. Keep the numeric level ID stable for `game.yaml`; no YAML range change is needed.
5. Choose an existing `.backdrop`, or add an `ArtId::Level4` entry before `ArtId::Count` and its matching path to `artPaths` in `engine/resources/asset_paths.h`. Keep the enum and path array in the same order. Export the optional PNG to `game/resources/art/levels/level_4.png`. For a distinct fallback illustration, extend `art::LevelBackdrop` in `engine/graphics/placeholder_art.cpp`.
6. Adjust `LevelSelectItems` and `DrawLevelSelect` in `ui/menus/menus.cpp` to fit the new entry; the current layout has space for four level buttons before the Back button.
7. Build normally: CMake discovers the new source and any helpers under its folder automatically. Registration in `main.cpp` is still required to make a level selectable. Run the tests in [CODE_GUIDE.md](CODE_GUIDE.md); their level loops use `LevelCatalog()` and will include the new entry. Add checks for any new gameplay mechanics.

## Menus and artwork

Menu layouts are in `ui/menus/menus.cpp`. Shared controls are in `ui/`; navigation and settings actions are handled by `main.cpp`.

Use `DrawTextFit(resources, ...)`, `DrawParagraph(resources, ...)` or `resources.DrawText(...)` for UI text. They use the shared font on the fixed 1600 by 900 canvas. Use `resources.BeginClip(...)` for a clip rectangle in canvas coordinates.

Font selection, PNG paths and resource copying are covered in [ART_ASSETS.md](ART_ASSETS.md).

## Shared collision system

`GameObjectDefinition.id` is a nonempty unique instance ID. Contacts expose that string in `CollisionContact.objectId` and `EventNotification.objectId`; `ObjectNotificationId(id)` provides a stable FNV-1a numeric ID for existing event predicates. `Player::LastCollision()` exposes the most recent step's contacts and normals, and `LastDamageSource()` survives respawn for death animations. A full level restart clears it.

Choose `collision` independently of `type`, `assetId` and visibility: `Solid` blocks every side, `OneWay` blocks descending top crossings, `Hazard` detects contact, `Trigger` detects contact without blocking, and `None` is decorative. `collisionOffset` and `collisionSize` are independent of art; a zero size pair defaults to the object's size. `filter.layer` and `filter.mask` must match in both directions to interact. Moving colliders supply previous and current rectangles for each simulation step.

`objectEvents::contactEntered` and `contactExited` report contact changes. Triggers also post `triggerEntered`, and hazards post `hazardHit`. Persistent overlap does not repeat entry or damage. Repeatable sensors rearm after exit; `triggerOnce` deactivates the object after entry. A hazard defaults to lethal; set `lethal = false` and `damage` for health damage, with `player.max_health` in config. Fatal contacts respawn at the checkpoint. Moving-solid crushes also respawn rather than leaving the player embedded.

Moving platforms carry riders, including downward motion, and release them on jump. Objects follow kinematic paths by default. Set `collideWithEnvironment = true` to resolve object motion against other solid/one-way geometry with the same solver. Falling objects then finish their action when they land on geometry, or at their configured travel limit. `OnBlocked` is a response hook for behavior such as reversing a moving obstacle; it does not implement detection.

`LevelDefinition.sideBoundaries`, `ceilingBoundary` and `bottomKillZone` control generated boundaries. The default bottom kill zone begins 100 pixels below the world. Disable it and author explicit hazard zones for a different fall rule. There is no implicit bottom floor. `CollisionSystem::SetStaticScene` is the legacy rectangle adapter and likewise only adds sides and a ceiling.

Existing scripted Level 1 hazards and Level 2 birds publish their geometry through `Level::ScriptContact`. Shared detection and entry tracking provide named contact events and debug outlines while the script retains its response (bounce, slide, restart). Gas uses an AABB around its drawn cloud. New reusable obstacles should use object definitions. Recycled scripted objects must receive new instance IDs; pigs and gas use monotonic spawn IDs.

Use `debug.show_hitboxes: true` in config to inspect boxes and IDs. The automated collision tests cover all directions, thin fast crossings, one-way surfaces, overlap recovery, moving platforms, pushes/crushes, falling-object landings, filters, damage on entry, kill zones, and level spawn grounding across 30/60/144/240 FPS.
