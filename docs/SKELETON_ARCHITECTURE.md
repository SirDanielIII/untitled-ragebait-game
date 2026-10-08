# Untitled Ragebait Game: foundation

This runnable skeleton follows the seven-page outline: **tutorial plus Levels 1–3**, four stages total. It includes functional menus, a student rectangle, shared platformer movement/collision/camera code, and the Entity's demonstration dialogue. Shapes and text are intentionally placeholders.

Lives, checkpoints, enemies, hazards, fake timer, tutorial tricks, limb physics, combat, completion and ending are **not implemented**. Climbing uses the shared jump controller until its controls are designed. The final stage reserves space for the lecture hall and Entity encounter. Switch stages through pause; there is no automatic completion.

## Components and directory structure

Application source stays under the existing `game/src/` directory.

| Location | Responsibility |
| --- | --- |
| `raylib_game.cpp` | Entry point, configuration layers, command-line modes |
| `core/application.*` | One frame loop, delta time, canvas scaling, persistence |
| `core/resources.*` | Window/audio RAII, resource paths, owned canvas/click sound |
| `core/config.*` | Typed defaults, restricted YAML reader/writer, diagnostics |
| `core/input.h` | Capture Raylib input into one `InputFrame` per frame |
| `core/layout.h` | 1600x900 canvas, world viewport and coordinate mapping |
| `screens/game_session.*` | Navigation, settings actions, active scene ownership |
| `ui/menu.*` | Shared buttons/checkboxes, slider items, hit testing, keyboard focus |
| `ui/slider.*` | Reusable range/value widget, pointer capture, dragging and keyboard adjustment |
| `levels/level_catalog.*` | Four stages: names, preview, geometry, spawn, dialogue |
| `levels/level_scene.*` | Shared scene reset/update/draw, player/camera/dialogue |
| `entities/player.*` | Walking, jumping, gravity, rectangle collisions, reset |
| `dialogue/typewriter.*` | Message ownership and reveal, independent of rendering |
| `dialogue/dialogue_box.*` | Wrapping, multiline drawing, overflow scrolling |
| `resources/config.yaml` | Packaged example/default configuration |
| `tests/foundation_tests.cpp` | Headless tests and graphical smoke test |

`screens.h` and the five original template screen implementations were replaced by these modules. Existing assets, licenses and all build files remain in place. No new dependency, screen inheritance hierarchy, ECS, event bus or physics engine was introduced.

## Application lifecycle

1. `main` loads packaged configuration, then optional preferences, before creating the window.
2. `Application` owns config, `RaylibContext`, `Resources`, and `GameSession`. Declaration order guarantees scene/resources destruction before audio/window shutdown. The default font is borrowed from Raylib; the render texture and optional click sound are owned.
3. Each frame captures input, clamps delta time to 0–0.1 seconds, updates the session, applies master volume/FPS, saves changed preferences, and renders. Slider values apply live; writes wait until the mouse is released. Units are seconds and pixels/second. Excess stall time is discarded to avoid large jumps.
4. `GameSession` owns a typed `ScreenId`, one menu, and an optional `unique_ptr<LevelScene>`. Buttons return `Action` values, applied after menu traversal so transitions cannot invalidate an executing button.
5. Starting a stage creates a scene. Pause and pause→settings retain it and stop updates. Resume continues its state; reset restores player/camera/dialogue. Selection/main destroys the scene.
6. Quit or the window close button ends the loop. Destructors release resources. Escape belongs to pause/back navigation. Losing window focus pauses desktop play.

Flow: main → tutorial or selection → play → pause → resume/reset/settings/selection/main. Settings returns to its opening screen.

Rendering uses a **1600×900 logical canvas** (also the default window size), scaled with letterboxing on resize. Mouse coordinates map back onto that canvas. Only the world viewport between the header and dialogue uses `Camera2D`; UI remains fixed.

## Adding a stage

1. Add a stable `LevelId` in `level_catalog.h`.
2. Add a `LevelDefinition` to `LevelCatalog()` in `level_catalog.cpp`: title, preview/future-content text, dialogue, color, world, spawn and solid platform rectangles. Selection registers automatically.
3. Increase the `session.last_level` validation bound in `ConfigStore::Load` for another numeric ID. IDs are persisted as the recently selected stage.
4. The world viewport and canvas are defined in `core/layout.h`. Smaller worlds center in the viewport; larger worlds scroll. Spawn the 30×44 player above a floor. All geometry uses world coordinates.

Example platform: `{350, 550, 180, 24}` = x, y, width, height. Ordinary stages reuse `LevelScene`; no extra loop/class is needed. Add agreed triggers in `LevelScene::Update`, extracting a module when needed. Definitions must outlive scenes; the catalog has static lifetime.

## Menus and controls

`GameSession::BuildMenu` creates `MenuItem` variants containing a `Button` or `SliderItem`. Buttons have an `Action`, logical rectangle, fill color, and filled/checkbox/back-arrow style. A `SliderItem` binds a generic `Slider` to a setting action and supplies its display units. `Menu::Update` handles focus/clicks and returns actions; `Draw` only renders. A stationary mouse does not steal keyboard focus.

The main menu follows the Chan_Wars reference: large green title banner, cream rules, green Play button, blue secondary buttons, red Quit, and a lower character area with primitive placeholders. Options uses Video/Dialogue/Audio columns, checkboxes, a triangular Back button, and red slider handles.

To add a button, register it in `BuildMenu` and handle its `ActionKind` in `Apply`. A new screen needs a `ScreenId`, menu/navigation and draw branch. Discrete setting changes rebuild labels while preserving focus; slider actions retain the existing widget to preserve drag capture.

A slider owns its numeric value, bounds, min/max, step and drag state; it does not know about configuration. Click anywhere on the track, drag even beyond its bounds, or use keyboard adjustment. Values clamp to the range. Pointer changes snap to the step; programmatic `SetValue` preserves valid values. Volume uses 0–1 in 0.01 steps; dialogue uses 0–240 characters/sec in steps of 5, with zero meaning instant.

```cpp
SliderItem{"MASTER VOLUME", ActionKind::SetVolume,
    Slider({1120, 555, 390, 60}, 0, 1, 0.01f, config.masterVolume),
    "%", 100, {}}
```

Slider changes return the chosen value in `Action::scalar`. They apply silently and save on release rather than playing sounds or writing a file each drag frame.

| Context | Controls |
| --- | --- |
| Menus | Mouse, Up/Down or W/S, Tab next, Enter choose, Esc/Backspace back |
| Play | A/D or Left/Right; Space/W/Up jump; Esc/P pause |
| Options sliders | Click/drag; Left/Right or A/D adjust selected value; Home/End select limits |
| Helpers | R reset player; Enter complete dialogue; T replay dialogue |
| Pause | Esc/P resume; buttons also reset or leave the scene |

Bindings live in `CaptureInput`. Entities/screens consume input values rather than querying Raylib. Horizontal input has one axis, so diagonal normalization does not apply.

## Movement and camera

`Player::Update` takes seconds, input, typed config, solid rectangles and world bounds. Walking sets horizontal velocity; grounded jumping sets upward velocity. Steps of at most 1/120 second apply gravity and axis-separated AABB collisions. World edges bound movement. Every scaffold has a continuous safe floor.

`Reset` clears velocity and grounded state; the next update detects support **before** checking jump input, so jumping immediately after a reset works. Ground contact is also preserved when a tiny final physics step rounds to zero displacement. A 0.01-pixel support tolerance prevents irregular frame timing from incorrectly clearing `grounded`. Rendering lives in `Draw`. Change rectangle size in `Bounds`; speed, jump impulse and gravity come from config. Extend here for acceleration, jump buffering or agreed climbing controls. This is a simple controller, not general rigid-body physics.

`LevelScene::UpdateCamera` follows both axes and clamps to stage bounds. Horizontal/climbing stages have larger worlds to exercise scrolling.

## Dialogue

Use one `DialogueBox` per independent message and one string containing all lines:

```cpp
dialogue.writer.SetSpeed(config.charactersPerSecond);
dialogue.writer.Replace("First line.\nSecond line.");
dialogue.writer.Update(dt); // update phase
dialogue.Draw(font, {40, 714, 1520, 162}, "The Entity", 24); // draw phase
```

`Complete()` skips, `Reset()` replays, and `Replace()` clears old state. Empty text is complete. CRLF/CR normalize to LF. Speed is characters/second, including spaces/newlines; zero reveals instantly. Fractional progress carries across frames. UTF-8 codepoints reveal together; the default font supports only its own glyph set, and grapheme clusters/complex scripts are outside this implementation.

`WrapText` computes ranges from the **whole message**, preserving explicit blank lines and splitting long words at character boundaries. Drawing intersects those ranges with the revealed prefix, so text stays in place. The box clips and follows the latest revealed lines on overflow. Layout depends on width/font size; rendering never advances text. Supply a suitable font for additional languages. The demo has entry/reset messages, not random narrator scheduling.

## Configuration and persistence

Raylib is the only existing dependency. Adding `yaml-cpp` would require build changes, so `ConfigStore` supports a **restricted YAML subset**, not general YAML:

- Top-level sections and exactly two-space-indented scalar keys.
- Plain finite numbers, lowercase `true`/`false`, whitespace-separated `#` comments, UTF-8 BOM.
- No strings, sequences, anchors, multiline scalars or deeper nesting.
- Unknown/duplicate keys, bad indentation, malformed/out-of-range values produce diagnostics and preserve prior/default values. Other valid keys still load. Missing keys/files retain defaults.

Load order: `GameConfig` defaults → `resources/config.yaml` (or `--config`) → user preferences (or `--settings`). Existing preferences override packaged values; edit the user file or use a fresh `--settings` path when testing new defaults. Consumers access typed fields and never parse YAML.

Configuration covers initial window dimensions/FPS, FPS visibility, menu-click volume/mute, movement, dialogue speed, and recently selected stage. Initial size applies at startup; frame-rate choices in Options apply immediately as well as at startup. Settings controls apply immediately; resizing does not change saved initial dimensions. No music is loaded yet.

Windows preferences: `%LOCALAPPDATA%/UntitledRagebaitGame/settings.yaml`. Other desktops use `XDG_DATA_HOME` or `~/.local/share/untitled_ragebait_game/`. Changes to settings/selected stage save automatically; slider drags save on mouse release (pending changes also save on exit). The new placeholder name uses its own preferences directory; files from the earlier `TheOriginalPlatformer` directory are left untouched and are not loaded, so the old 1000x700 setting cannot override the new default. Save failure is logged/shown; in-memory changes remain usable. The writer rewrites known settings directly, losing comments/unknown keys; it is not a crash-safe save-game serializer. No unlocks, lives or player position are stored.

To add a value, update its `GameConfig` default, validated reader/writer branches in `config.cpp`, and example YAML; pass the typed value to its consumer. Configuration does not load or instantiate assets.

## Chan_Wars decisions

| Original pattern | Foundation decision |
| --- | --- |
| Separate menus/levels, colored rectangular buttons and settings sliders | Retained/adapted as modules, catalog, shared menu styles and reusable `Slider` |
| `Level.restore` / per-level `reload` | Adapted into explicit scene/player/dialogue reset |
| Numeric `Main.lvl` switching | Enums, returned actions, one scene owner |
| Each `run()` owns a loop/event pump | One application loop/input snapshot |
| `dt *= 60`, event timers, render/update mixing | Seconds, separate updates/draws, fractional reveal |
| Two typewriter queues, locks, timers, conditional line-1 drawing | One multiline message with stable wrapping; original `messages.size() > 1` condition could omit a revealed first line |
| YAML config eagerly loads assets and large nested game definitions | Typed scalar config separate from RAII resources |
| Options and last-level persistence | Validated preferences and recently selected stage |
| Card combat, shops, health bars, channel mixer, boot/logo/fades | Discarded for this foundation; outside current skeleton scope |

The archive was extracted only to a temporary inspection directory. The PDF/archive were not edited.

## Build, run and verification

The supported workflow is the original **CMake** project: C++20, existing Raylib 6.0 `FetchContent`, recursive source/header globbing with `CONFIGURE_DEPENDS`, and automatic resource copying beside the executable. No CMake, Makefile, IDE project, build script or dependency setting changed. Legacy Makefile/VS/Android paths were not adapted/verified; some do not discover nested C++20 modules. Browser callback ownership is retained, but browser builds/persistence are unverified (cross-session persistence needs IDBFS).

From the repository root, exact commands tested with the current CLion cache:

```powershell
& 'C:/Users/danie/AppData/Local/Programs/CLion/bin/cmake/win/x64/bin/cmake.exe' --build build/clion --parallel 4
# Match runtime DLLs to the compiler in build/clion/CMakeCache.txt.
$env:PATH = 'C:/Users/danie/AppData/Local/Programs/CLion/bin/mingw/bin;' + $env:PATH
& './build/clion/untitled-ragebait-game/untitled-ragebait-game.exe'
```

This machine also has `C:/Tools/mingw64` with different runtime DLLs. This CLion-built executable can fail before `main` if only that runtime is on PATH. CLion Run normally supplies its toolchain environment. No system PATH or build/compiler setting was changed.

For a fresh configuration using the existing files and prerequisites in `BUILDING.md`:

```powershell
# Only configure when no suitable cache exists; keep your IDE toolchain.
& 'C:/Tools/cmake/bin/cmake.exe' -S game -B build/vscode -G Ninja
& 'C:/Tools/cmake/bin/cmake.exe' --build build/vscode --parallel 4
& './build/vscode/untitled-ragebait-game/untitled-ragebait-game.exe'
```

Fresh configuration needs the existing Raylib download; the tested cache already had Raylib. Resources resolve beside the executable first, then repository-relative fallbacks, so any working directory is supported.

```powershell
# CLion executable requires the matching compiler PATH above.
& './build/clion/untitled-ragebait-game/untitled-ragebait-game.exe' --self-test
& './build/clion/untitled-ragebait-game/untitled-ragebait-game.exe' --smoke-test --capture-dir "$env:TEMP/ragebait_foundation_captures"
# Optional interactive overrides:
& './build/clion/untitled-ragebait-game/untitled-ragebait-game.exe' --config './game/src/resources/config.yaml' --settings "$env:TEMP/ragebait_test_preferences.yaml"
```

Self-tests open no graphics/audio context and verify reveal/wrapping/reset/UTF-8, config fallback/save-load, movement/jump/collision (including irregular frame intervals and immediate spawn/reset jumps), slider dragging/keyboard/ranges, canvas mapping, navigation and all stage lifecycles. The smoke test opens a hidden **real Raylib/OpenGL window**, sends synthetic input through the same controller, renders every screen/stage, optionally captures frames, and shuts down. It leaves user preferences untouched.

Verified here: CMake build; passing self-tests/smoke test; audio initialization/asset loading; 1600x900 and resized/letterboxed layouts inspected; multiline text and sliders checked; clean GPU/audio/window teardown; protected-file hashes unchanged. The original jump rejection was confirmed in the native debugger at dt=0.0166766681, with jump pressed, player y=606 on the floor, and grounded=false; the irregular-time regression now passes. Physical keyboard/mouse events, focus loss, other platforms/machines and gameplay balance were not manually tested.
