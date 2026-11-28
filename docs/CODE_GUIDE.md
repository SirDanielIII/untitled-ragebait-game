# Code guide

Start with [`game/src/main.cpp`](../game/src/main.cpp). It lists the levels, chooses the current screen, dispatches updates/drawing, and runs the frame loop.

The current game supports menus, settings, level selection, movement/collision, pause/restart and dialogue. Level 0 and Level 3 have preliminary layouts; Level 1 has the street, traffic, sky and campus route, and Level 2 has a vertical climbing route with patrol birds. Level 0 includes a scripted idle trap and hallway dialogue. See [LEVEL_AUTHORING.md](LEVEL_AUTHORING.md) for stage mechanics.

## Where to make a change

| Task | File |
| --- | --- |
| Start, switch, pause, resume or restart a level | `game/src/main.cpp` (`Game::Apply`, `Game::Update`) |
| Register a new level | `registrations` near the top of `main.cpp` |
| Change a numbered level (0-3) | `game/src/levels/level_<number>/level_<number>.cpp` |
| Assign artwork to level objects | `game/src/levels/level_<number>/level_<number>_sprites.cpp` |
| Change level selection descriptions or menu artwork IDs | `game/src/ui/menus/menus.cpp` |
| Edit a level's scripted events | `game/src/levels/level_<number>/level_<number>_events.cpp` |
| Change shared level behavior or camera follow | `game/src/levels/level.cpp` |
| Change movement or collisions | `game/src/player/player.cpp`, `game/src/engine/collision/collision_system.*` |
| Change obstacle debug outlines | `game/src/engine/collision/collision_system.*`, `game/src/levels/level.cpp` |
| Change crouch-jump air particles | `game/src/player/jump_particles.cpp` |
| Change menu layouts, labels or buttons | `game/src/ui/menus/menus.cpp` |
| Change button/slider interactions | `game/src/ui/menu/menu.cpp`, `game/src/ui/slider/slider.cpp` |
| Change dialogue reveal or panel drawing | `game/src/ui/dialogue/dialogue.cpp` |
| Change colors or text layout | `game/src/ui/theme.h`, `game/src/ui/text/text.cpp` |
| Change input bindings | `game/src/engine/input/input.h` |
| Change player preferences / gameplay tuning / save defaults | `game/resources/settings.yaml` / `config.yaml` / `game.yaml` |
| Add settings keys or validation | `game/src/engine/config/config.cpp` |
| Choose fonts and legacy non-menu artwork slots | `game/src/engine/resources/asset_paths.h` |
| Add sprites/animations (no code) | Game Data sheet, exported to `game/resources/data/game_data.csv` ([ART_ASSETS.md](ART_ASSETS.md#game-data-sprites-and-animations)) |
| Change sprite loading, pivots, flipping or layers | `game/src/engine/assets/` |
| Add sounds/music (no code) | Audio tab of Game Data, exported to `game/resources/data/audio_data.csv` ([guide](../game/src/engine/audio/README.md)) |
| Change audio loading, playback, music or volumes | `game/src/engine/audio/` |
| Change resource loading or window rendering | `game/src/engine/resources/resources.cpp`, `game/src/engine/window/game_window.cpp` |

## Source layout

```text
game/
  CMakeLists.txt                 Target, dependencies, resource staging and CTest
  resources/                    Runtime fonts, images, Game Data CSV, defaults and audio
  tests/                        Regression and rendering checks
  projects/VSCode/              Original template workspace (opens repository root)
  projects/VS2022/              Original template projects (not maintained)
  src/
    CMakeLists.txt              Recursive source/header/test discovery
    main.cpp                    Level registration, screen handler, loop, startup
    game.h                      Game handler declarations
    player/                     Movement, stamina and jump particles
    objects/                    Object ownership, colliders and reusable behaviors
    levels/
      level.h / level.cpp       Shared level data, lifecycle and camera
      level_0/level_0.cpp        Level 0 (tutorial)
      level_1/level_1.cpp        Level 1
      level_2/level_2.cpp        Level 2
      level_3/level_3.cpp        Level 3
      level_*/level_*_sprites.cpp Asset ID assignments by object ID
      event_manager.*           Scripted events and notifications
    ui/
      menus/menus.*             All four menu layouts
      menu/menu.*               Shared buttons and menu navigation
      slider/slider.*           Slider control
      dialogue/dialogue.*       Typewriter and dialogue panel
      text/text.*               Text fitting and paragraphs
      hud/                      Gameplay HUD
      theme.h                   Shared UI colors
    engine/
      window/game_window.*      Window, frame rendering and settings persistence
      config/config.*           Settings and YAML reader/writer
      input/input.h             Keyboard and mouse capture
      graphics/layout.h         Canvas size and coordinate conversion
      graphics/placeholder_art.* Fallback drawings for missing artwork
      resources/resources.*     Fonts, texture/audio ownership and native viewport
      resources/asset_paths.h   Runtime fonts and legacy non-menu artwork paths
      collision/                Swept AABBs, contacts and debug overlay
      assets/game_data.*        Game Data CSV import, validation and frame math
      assets/asset_manager.*    Game Data textures, pivots, flipping and placeholders
      assets/animator.h         Per-object animation state and playback time
      assets/render_queue.h     Background/Middle/Foreground/UI draw order
      audio/audio_data.*        Audio Data CSV import and validation
      audio/audio_manager.*     Sound effects, music streaming/crossfades, volumes and pause
      data/csv.*                CSV helpers shared by Game Data and Audio Data
```

Includes are relative to `game/src`, for example `#include "levels/level.h"`. `src/CMakeLists.txt` uses `GLOB_RECURSE` with `CONFIGURE_DEPENDS`: `.cpp`, `.h` and `.hpp` files under `src/` and `.cpp` tests under `tests/` are discovered recursively. Adding, removing or moving a source file triggers a refresh on the next build. Register new playable levels in `main.cpp` as described in the level guide.

CMake stages `game/resources/` on every build of the game target, including asset-only edits. Desktop resources go beside the executable; Web resources go in `${CMAKE_BINARY_DIR}/resources` for Emscripten preloading. Generated output belongs under `build/`. See [BUILDING.md](../BUILDING.md) for the CMake workflow. Makefile and Ninja generators also produce `compile_commands.json` in the build directory; other generators ignore that option.

## How the game runs

1. `main()` loads configuration and creates a `GameWindow`.
2. `GameWindow::Run()` in `main.cpp` captures input and runs one frame at a time.
3. `GameWindow::Frame()` applies timing, audio and preferences, calls `Game::Update()`, and renders the result.
4. `Game::Update()` in `main.cpp` updates the active level or reads a menu action. `Game::Apply()` handles transitions.
5. `Game::Draw()` in `main.cpp` draws the active level or selected menu. `ui/menus/menus.cpp` defines how menus look.

`Game` is declared in `game.h` and implemented in `main.cpp`, so all level routing is in one place. It owns the active `Level` and menu state. Starting a level constructs its registered class and resets it after construction. Pause and Settings retain it; Main Menu and Level Select release it. Level 1 completion automatically opens Level 2; other level-to-level transitions and the ending are not yet wired.

`GameWindow` handles platform and rendering details. It owns configuration, the Raylib context, resources and `Game`. Destruction releases the level first, then GPU/audio resources, then the context. Navigation tests can create `Game` without opening a window.

`Level` is the common base class. Each numbered folder holds a level file defining its content, a concrete class for its events, and a factory. `LevelDefinition`, `LevelId`, the base class and factory declarations are together in `levels/level.h`. Level IDs keep explicit values 0-3 so existing saved preferences still work.

## Frame loop

Each frame captures input, limits `dt` to 0-0.1 seconds, updates the current screen, applies audio/FPS changes, saves pending settings and draws.

Scene updates run dialogue, shared object/player physics substeps and contact handling, player animation/audio, the stage's `OnUpdate`, scripted contact/event processing, then camera follow. Pause and Settings suspend this sequence. R/Restart resets the player, opening dialogue and elapsed time, then calls `OnReset`. Desktop focus loss pauses play.

Define scripted events in the stage's events file. Use `OnReset` to restore stage state and `OnUpdate` to track values the events need, such as time spent standing still. See [LEVEL_AUTHORING.md](LEVEL_AUTHORING.md).

## Rendering

The game draws directly to the physical framebuffer, using 1600×900 logical coordinates. `GameWindow::Draw` passes the aspect-fitted viewport to `Game::DrawFrame`. Normal desktop windows keep a 16:9 resize constraint; non-16:9 outputs letterbox. `MouseOnCanvas` reverses the same mapping, including display DPI.

Draw UI in logical coordinates. `Level::DrawScene` composes the world camera with the native viewport using `ScopedModelview` and `CameraModelview`; ordinary `BeginMode2D` would replace the viewport. The render queue retains its layer-vector storage between frames and clears draw jobs after flushing. Sprite assignment files, stand-ins and `DrawWorldFront` preserve world layering. Opaque menus, including Pause, skip drawing the world entirely. `Level::DrawUi` runs after the world. `resources.BeginClip(bounds)` and `EndScissorMode()` use logical clip bounds.

Font files are read once; `Resources` caches bitmap atlases by role and physical size. `MeasureText` uses logical metrics independently of raster size. Use shared text helpers for UI and world labels, with `FontRole::Heading` (Silkscreen Bold) for headings and `FontRole::Typewriter` (Jersey10) for dialogue, level previews and keycap labels. The cache uses a 48-size / 16 MiB soft atlas budget per face and evicts unused sizes between frames; logical text metrics remain independent of raster size. Desktop fullscreen is toggled from Settings, F11 or Alt+Enter. The Controls panel is a read-only keybinding reference.

All nine menu artwork slots use `resources.Assets()` through `art::DrawMenuArt`, with Game Data Asset IDs and fractional aspect fitting. Missing backgrounds use the solid theme color, missing pause art is omitted, and missing level previews leave a dark rectangle. See [ART_ASSETS.md](ART_ASSETS.md) for the CSV rows, dimensions and paths.

## Input and menus

`CaptureInput` produces one `InputFrame` per frame. Gameplay consumes its movement/jump values; menus return typed actions to `Game`.

Mouse movement or clicks can change focus. A stationary pointer does not override keyboard navigation. Sliders capture the pointer until release, including outside their bounds. Slider changes keep the existing widget and save on release.

| Context | Controls |
| --- | --- |
| Menus | Mouse, Up/Down or W/S, Tab, Enter, Esc/Backspace |
| Movement | A/D or Left/Right; Space/W/Up to jump |
| Crouch | Hold S/Down/Shift (either Shift key) |
| Climb (Level 2) | Hold W/Up near a wall |
| Fullscreen (desktop) | F11 or Alt+Enter |
| Pause | Esc/P |
| Dialogue | Enter to reveal; T to replay, when enabled |
| Restart | R or Restart |
| Sliders | Drag, Left/Right or A/D, Home/End |

## Movement and dialogue

The player uses seconds and substeps of at most 1/120 second. Objects and the player share those steps. `engine/collision/collision_system.*` resolves swept AABBs, including relative object movement, platform carrying, overlap recovery and crush reporting. Ground support is checked before jump input, including immediately after spawn/reset. Level boundaries are explicit colliders; the bottom is a kill zone rather than a clamp. See the collision authoring notes in [LEVEL_AUTHORING.md](LEVEL_AUTHORING.md).

Tune movement in the `player` section of `game/resources/config.yaml`, then rebuild/restart (or supply a tuning file with `--config`). It includes acceleration, counter-strafe acceleration, starting speed, coasting friction, speed, jump impulse, gravity, climbing, body dimensions, crouch multipliers and all stamina values. `GameConfig` supplies fallback defaults when a key is missing; packaged tuning can override them.

Hold Space (or W/Up) to jump whenever grounded and enough stamina is available. Climbing is exclusive to Level 2, where W/Up prioritizes climbing beside a wall; the other levels retain W/Up as jump controls. S/Down/Shift (either Shift key) crouches on the ground to half standing height with feet fixed and moves at 20% speed. Airborne duck input is ignored; jumping or leaving a ledge restores standing height as soon as there is headroom. Low ceilings still prevent expanding the collider through geometry. A jump from crouch, including releasing crouch and jumping together, multiplies takeoff speed by 1.5 with the packaged tuning (1.2 in the C++ fallback) and pays the larger crouch jump cost. Successful crouch jumps emit a brief burst of square air particles at the takeoff point, managed by `player/jump_particles.*` and the shared level lifecycle. Add `crouch` and optionally `crouch_walk` player animation states for dedicated art; until then the standing artwork is compressed to the crouch height.

With packaged `config.yaml`, stamina costs 18 per ordinary jump or 50 per crouch jump (the C++ fallback is 30). Climbing drains 45/second and crouching drains 2/second; these costs stack. Walking and coasting have no stamina cost, and there is no passive airborne drain. Standing or walking on the ground restores 40/second when not crouching or climbing, including during exhaustion. Below `low_stamina_threshold` (30% by default), the bar turns red and both walking and crouch movement use `exhausted_speed_multiplier` (50%): default speeds drop from 220 to 110 and from 44 to 22 respectively. Hitting zero also latches exhaustion and prevents jumping/climbing until the 15% recovery threshold is reached; movement stays slow until stamina leaves the red range. Walking can continue throughout recovery. All thresholds, rates and multipliers are configurable. Respawn restores the configured maximum stamina and clears crouch/exhaustion.

`ui/dialogue/dialogue.h` declares both the reveal state and dialogue panel. `Typewriter` stores the full message and fractional reveal progress. `DialogueBox` wraps the full text, reserves word positions while revealing, clips overflowing lines and follows the latest revealed line. UTF-8 codepoints reveal together; the loaded font supplies glyph coverage.

Each `DialogueCue` specifies text, speaker, speed multiplier, delay, size and skip/replay controls. `Say` replaces the current cue. The Instant setting skips both reveal time and delay.

## Preferences

The three YAML files have distinct purposes:

- `resources/config.yaml`: developer tuning under `player`, plus `debug.show_hitboxes`. The obstacle-only debug overlay labels solid bounds (green), one-way platforms (sky blue), hazards (red) and explicitly marked obstacle sensors such as birds (yellow). Decorative bounds, disabled shapes, ordinary triggers and the player box are omitted. Generated world walls and fall hazards remain included because they are collision geometry. Player offsets are `collision_offset_x/y`; `standing_width/height` define the body independently of animation frames.
- `resources/settings.yaml`: packaged player preferences (`window`, `audio`, `dialogue`). Runtime edits save to `settings.yaml` beside the executable, or the path supplied by `--settings`.
- `resources/game.yaml`: initial save data (`session.last_level` for now). Runtime save data lives in `game.yaml` beside the runtime settings file. An older settings file's session section is migrated when no game save exists.

Load order is C++ defaults, packaged tuning (or `--config`), packaged settings/save defaults, then runtime settings/save data. `ConfigStore::LoadFile`/`SaveFile` select the file's scope so settings cannot mask tuning or contain save data. The legacy `Load`/`Save` API remains available for combined files in tests/tools.

The reader accepts section maps with two-space-indented numeric/bool scalars, comments and a UTF-8 BOM. Unknown keys, duplicate keys and invalid scalar types retain the preceding valid value and produce diagnostics. Finite numeric tuning is accepted without range restrictions. Audio volumes alone must lie in [0, 1]. Existing menu sliders provide convenient ranges; the YAML reader does not impose those ranges.

`--self-test` returns before configuration or window creation. `--smoke-test` loads packaged files (and `--config` when supplied) while skipping runtime preferences/save data and all persistence. Writes during ordinary gameplay wait until mouse release, with pending changes saved on exit. The game opens the main menu on startup.

## Build and tests

See [BUILDING.md](../BUILDING.md) for compiler and IDE setup. After configuring a desktop build with `BUILD_TESTING=ON` (the default), run from the repository root:

```powershell
cmake --build build/clion --parallel 4
ctest --test-dir build/clion --output-on-failure
& './build/clion/untitled-ragebait-game/untitled-ragebait-game.exe' --self-test
& './build/clion/untitled-ragebait-game/untitled-ragebait-game.exe' --smoke-test --capture-dir './build/test-captures'
```

Run with the same MinGW runtime used to compile the executable. CLion supplies that environment automatically; a separate terminal may need the toolchain's `bin` directory on `PATH`.

These paths assume a single-configuration Windows build. Use `build/vscode` for a VS Code build. Multi-configuration generators add a configuration folder such as `Debug` below `untitled-ragebait-game`; pass `--config Debug` to the build and `-C Debug` to CTest, and adjust the executable path. `BUILD_TESTING=OFF` disables CTest registration; test sources and the executable's test options are still included.

Self-tests cover dialogue, config, movement, sliders, navigation, pause/reset, stage hooks, shared bird flight, swoops and hit cooldowns, sprite mappings, collision filtering and obstacle debug eligibility. Independent test groups continue after a failure and the process returns a failure status if any group fails. The smoke test uses a hidden Raylib window, synthetic input and real rendering. It checks font fallback, texture replacement, bird animation and facing, all screens and stage lifecycles.

Rendering tests cover ten framebuffer sizes, including portrait, ultrawide and 16:10, with native geometry, glyph sharpness, sprite transforms, mapped sprite replacement, front text, clipping and input mapping. On Windows the smoke test also exercises native resizing. Tests do not write user preferences.

Desktop is the tested target. Browser startup code is present; browser persistence still requires IDBFS integration.
