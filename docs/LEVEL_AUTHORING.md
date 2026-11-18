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

The `LevelDefinition` at the top of each file contains the title, description, intro, backdrop, world bounds, spawn and platforms. The level instance owns the stage's runtime state. `main.cpp` registers definitions and factories for stage selection.

## Loading and switching levels

Open **Dev Level Select** from the main or pause menu to load a stage directly for testing. Its preview includes the stage's `.plannedContent` development notes.

`ui/menus/menus.cpp` creates buttons that send a `StartLevel` action with the chosen `LevelId`. `Game::Apply` in `main.cpp` handles that action: it calls `CreateLevel`, replaces the active scene, and enters the Playing screen. `CreateLevel` in `main.cpp` constructs the registered stage class and calls `Reset`.

Automatic progression to the next level has not been implemented.

## Layout

Edit `.world`, `.spawn` and `.platforms` in the stage definition. Coordinates are world pixels; rectangles use `{x, y, width, height}`. The player's collision box is 30 by 44 pixels.

`.backdrop` selects a background image. Collision geometry comes from `.platforms`, independently of the image. Add stage-specific drawing in `DrawWorld(const Resources&) const`; it runs inside the world camera, before platforms and the player are drawn.

For objects with art in the Game Data sheet (hazards, props, effects), override `SubmitSprites` and queue them by ID. Each one is drawn on its sheet layer, at its pivot, with a placeholder box until its PNG exists:

```cpp
void SubmitSprites(RenderQueue& queue, const Resources& resources) const override
{
    const auto& assets = resources.Assets();
    if (const auto* anvil = assets.Find("anvil_fall"))
        queue.Submit(assets, *anvil, anvilAnimator.Frame(*anvil), anvilPosition);
}
```

Keep an `Animator` member per animated object, call `animator.SetState("fall")` and `animator.Update(dt)` from `OnUpdate`, and reset it in `OnReset`. Sprite size never affects collision; keep hazard rectangles in the level code. See [ART_ASSETS.md](ART_ASSETS.md#game-data-sprites-and-animations).

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
| `Events()` | Register events, post notifications and query completion |
| `Definition()` | Read the authored level data |

The update order is dialogue, player movement, `OnUpdate`, event evaluation, then camera follow. Pause and Settings suspend scene updates. Restart Stage and R clear event completion and pending notifications, restore the player, elapsed time and opening dialogue, then call `OnReset` and update the camera. Clear stage-specific state in `OnReset`. Opening Main Menu or Dev Level Select destroys the active level and its events.

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

These hazards, lives, checkpoints, death, stage completion, climbing controls and the boss are pending gameplay work.

## Adding a stage

1. Copy a numbered level folder, for example `game/src/levels/level_1/` to `level_4/`. Rename its header, implementation and events file for the new level. Update the includes and use `Level4`, `Level4Definition`, `MakeLevel4` and `LevelId::Level4` throughout.
2. Add an explicit `LevelId` value in `levels/level.h`; preserve the existing numeric values used by saved settings.
3. Declare `Level4Definition` and `MakeLevel4` in `levels/level.h`, then add `{Level4Definition, MakeLevel4}` to `registrations` in `main.cpp`. This list drives both loading and the level selector.
4. Extend the `session.last_level` range in `engine/config/config.cpp`.
5. Choose an existing `.backdrop`, or add an `ArtId::Level4` entry before `ArtId::Count` and its matching path to `artPaths` in `engine/resources/asset_paths.h`. Keep the enum and path array in the same order. Export the optional PNG to `game/resources/art/levels/level_4.png`. For a distinct fallback illustration, extend `art::LevelBackdrop` in `engine/graphics/placeholder_art.cpp`.
6. Adjust `LevelSelectItems` and `DrawLevelSelect` in `ui/menus/menus.cpp` to fit the new entry; the current layout has space for four level buttons before the Back button.
7. Build normally: CMake discovers the new source and any helpers under its folder automatically. Registration in `main.cpp` is still required to make a level selectable. Run the tests in [CODE_GUIDE.md](CODE_GUIDE.md); their level loops use `LevelCatalog()` and will include the new entry. Add checks for any new gameplay mechanics.

## Menus and artwork

Menu layouts are in `ui/menus/menus.cpp`. Shared controls are in `ui/`; navigation and settings actions are handled by `main.cpp`.

Use `DrawTextFit(resources, ...)`, `DrawParagraph(resources, ...)` or `resources.DrawText(...)` for UI text. They use the shared font on the fixed 1600 by 900 canvas. Use `resources.BeginClip(...)` for a clip rectangle in canvas coordinates.

Font selection, PNG paths and resource copying are covered in [ART_ASSETS.md](ART_ASSETS.md).
