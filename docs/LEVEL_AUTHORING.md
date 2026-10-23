# Level development

## Files

Each numbered level has a folder under `game/src/levels/`, containing its layout, dialogue and gameplay code.

The four current levels provide placeholder platforms, movement, camera follow and opening dialogue. Their `OnReset` and `OnUpdate` hooks contain TODOs for stage-specific gameplay. The objectives below describe the intended content; reaching a door or classroom does not currently complete a level.

Paths in this guide are relative to `game/src/` unless shown with a `game/` prefix.

| Stage | File | Planned objective |
| --- | --- | --- |
| Level 0 (tutorial) | `levels/level_0/level_0.cpp` | Leave the house through the front door |
| Level 1 | `levels/level_1/level_1.cpp` | Cross the streets and reach campus |
| Level 2 | `levels/level_2/level_2.cpp` | Climb to the top-floor classroom |
| Level 3 | `levels/level_3/level_3.cpp` | Face the Entity outside the lecture hall |

The `LevelDefinition` at the top of each file contains the title, description, intro, backdrop, world bounds, spawn and platforms. All shared types are declared in `levels/level.h`. The class below the definition holds the stage's runtime state. `main.cpp` registers definitions and factories for stage selection.

## Loading and switching levels

Open **Dev Level Select** from the main or pause menu to load a stage directly for testing. Its preview includes the stage's `.plannedContent` development notes.

`ui/menus/menus.cpp` creates buttons that send a `StartLevel` action with the chosen `LevelId`. `Game::Apply` in `main.cpp` handles that action: it calls `CreateLevel`, replaces the active scene, and enters the Playing screen. `CreateLevel` in `main.cpp` constructs the registered stage class and calls `Reset`.

Automatic progression to the next level has not been implemented.

## Layout

Edit `.world`, `.spawn` and `.platforms` in the stage definition. Coordinates are world pixels; rectangles use `{x, y, width, height}`. The player's collision box is 30 by 44 pixels.

`.backdrop` selects a background image. Collision geometry comes from `.platforms`, independently of the image. Add stage-specific drawing in `DrawWorld(const Resources&) const`; it runs inside the world camera, before platforms and the player are drawn.

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
    // Check triggers and update active events.
}
```

`dt` is gameplay time in seconds, clamped to 0-0.1 by `GameWindow::Frame`. Store event flags, timers and other mutable data as class members. Event dependencies and scheduling belong in this code; there is no shared event system yet.

| Member | Purpose |
| --- | --- |
| `GetPlayer()` | Read position, bounds, velocity and grounded state |
| `Dialogue()` | Read the current message and its reveal state |
| `ElapsedSeconds()` | Read time since the stage was reset |
| `Say(DialogueCue)` | Replace the current message |
| `Definition()` | Read the authored level data |

The update order is dialogue, player movement, `OnUpdate`, then camera follow. Pause and Settings suspend scene updates. `CreateLevel` calls the registered `MakeLevelN` factory, then calls `Reset` after construction. `Reset` restores the player, elapsed time and opening dialogue, calls `OnReset`, and updates the camera. Restart Stage and R call `Reset` on the existing level; they do not reconstruct it, so clear all stage-specific state in `OnReset`. Opening Main Menu or Dev Level Select destroys the active level.

Keep update logic out of drawing functions. Use the application's existing frame loop rather than adding a loop inside a stage.

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
    .allowReplay = true
},
```

`speedMultiplier` multiplies the text-speed setting: 1 is normal speed, 0.5 is half speed and 2 is double speed. Zero reveals the message immediately and skips the initial delay, as does the player's Instant setting. `Say` clamps multipliers to 0-10, text sizes to 12-48 logical pixels, and negative delays to zero.

Enter reveals the whole message. T replays it, including the initial delay. The cue's flags enable or disable those controls. Changing the text-speed setting takes effect on the current message.

Call `Say` from stage logic to start another message:

```cpp
Say({.text = "The stairs do that sometimes.", .speedMultiplier = 1.5f});
```

Call it once when the event starts. Calling it every frame resets the typewriter every frame. `Say` replaces text immediately; queueing and interruptions are part of the stage's event logic.

## Design notes

The student is sleep deprived and late for class. The Entity gives unreliable advice while the surroundings turn into hallucinated obstacles.

- **Level 0 (house):** falling lights and pipes, floor holes, flooded rooms with electrical hazards, furniture platforming, false gas-leak doors, a fireplace monster.
- **Level 1 (streets):** traffic, cloud jumps, birds, live telephone poles, rainbow slides, a crossing chicken, falling satellites, flying pigs.
- **Level 2 (stairwell):** missing stairs, exposed bricks to climb, falling bricks. Design the level blueprint before expanding these mechanics.
- **Level 3 (lecture hall):** shifting floors, floating desks and chairs, paper airplanes, wall climbing and the Entity blocking entry.

These hazards, lives, checkpoints, death, stage completion, climbing controls and the boss are pending gameplay work.

## Adding a stage

1. Create the next numbered folder, for example `game/src/levels/level_4/`. Copy an existing level to `level_4.cpp` and use `Level4`, `Level4Definition`, `MakeLevel4` and `LevelId::Level4` throughout.
2. Add an explicit `LevelId` value in `levels/level.h`; preserve the existing numeric values used by saved settings.
3. Declare `Level4Definition` and `MakeLevel4` in `levels/level.h`, then add `{Level4Definition, MakeLevel4}` to `registrations` in `main.cpp`. This list drives both loading and the level selector.
4. Extend the `session.last_level` range in `engine/config/config.cpp`.
5. Choose an existing `.backdrop`, or add an `ArtId::Level4` entry before `ArtId::Count` and its matching path to `artPaths` in `engine/resources/asset_paths.h`. Keep the enum and path array in the same order. Export the optional PNG to `game/resources/art/levels/level_4.png`. For a distinct fallback illustration, extend `art::LevelBackdrop` in `engine/graphics/placeholder_art.cpp`.
6. Adjust `LevelSelectItems` and `DrawLevelSelect` in `ui/menus/menus.cpp` to fit the new entry; the current layout has space for four level buttons before the Back button.
7. Build normally: CMake discovers the new source and any helpers under its folder automatically. Registration in `main.cpp` is still required to make a level selectable. Run the tests in [CODE_GUIDE.md](CODE_GUIDE.md); their level loops use `LevelCatalog()` and will include the new entry. Add checks for any new gameplay mechanics.

## Menus and artwork

Menu layouts are in `ui/menus/menus.cpp`. Shared controls are in `ui/`; navigation and settings actions are handled by `main.cpp`.

Use `DrawTextFit(resources, ...)`, `DrawParagraph(resources, ...)` or `resources.DrawText(...)` for UI text. They use the font rasterized for the current output size. Use `resources.BeginClip(...)` for a clip rectangle in logical UI coordinates.

Font selection, PNG paths and resource copying are covered in [ART_ASSETS.md](ART_ASSETS.md).
