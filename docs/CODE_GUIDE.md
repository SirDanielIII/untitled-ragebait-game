# Code guide

Start with [`game/src/main.cpp`](../game/src/main.cpp). It lists the levels, chooses the current screen, dispatches updates/drawing, and runs the frame loop.

The current game supports menus, settings, level selection, movement/collision, pause/restart and dialogue. All four levels have placeholder layouts; stage-specific events, hazards and completion/progression are still pending. See [LEVEL_AUTHORING.md](LEVEL_AUTHORING.md) for those planned mechanics.

## Where to make a change

| Task | File |
| --- | --- |
| Start, switch, pause, resume or restart a level | `game/src/main.cpp` (`Game::Apply`, `Game::Update`) |
| Register a new level | `registrations` near the top of `main.cpp` |
| Change a numbered level (0-3) | `game/src/levels/level_<number>/level_<number>.cpp` |
| Change shared level behavior or camera follow | `game/src/levels/level.cpp` |
| Change movement or collisions | `game/src/player/player.cpp` |
| Change menu layouts, labels or buttons | `game/src/ui/menus/menus.cpp` |
| Change button/slider interactions | `game/src/ui/menu/menu.cpp`, `game/src/ui/slider/slider.cpp` |
| Change dialogue reveal or panel drawing | `game/src/ui/dialogue/dialogue.cpp` |
| Change colors or text layout | `game/src/ui/theme.h`, `game/src/ui/text/text.cpp` |
| Change input bindings | `game/src/engine/input/input.h` |
| Change default settings | `game/resources/config.yaml` |
| Add settings keys or validation | `game/src/engine/config/config.cpp` |
| Choose fonts and artwork | `game/src/engine/resources/asset_paths.h` |
| Change resource loading or window rendering | `game/src/engine/resources/resources.cpp`, `game/src/engine/window/game_window.cpp` |

## Source layout

```text
game/
  CMakeLists.txt                 Target, dependencies, resource staging and CTest
  resources/                    Runtime fonts, images, defaults and audio
  tests/                        Regression and rendering checks
  projects/VSCode/              Original template workspace (opens repository root)
  projects/VS2022/              Original template projects (not maintained)
  src/
    CMakeLists.txt              Recursive source/header/test discovery
    main.cpp                    Level registration, screen handler, loop, startup
    game.h                      Game handler declarations
    player/player.*             Movement and collision
    levels/
      level.h / level.cpp       Shared level data, lifecycle and camera
      level_0/level_0.cpp        Level 0 (tutorial)
      level_1/level_1.cpp        Level 1
      level_2/level_2.cpp        Level 2
      level_3/level_3.cpp        Level 3
    ui/
      menus/menus.*             All four menu layouts
      menu/menu.*               Shared buttons and menu navigation
      slider/slider.*           Slider control
      dialogue/dialogue.*       Typewriter and dialogue panel
      text/text.*               Text fitting and paragraphs
      theme.h                   Shared UI colors
    engine/
      window/game_window.*      Window, frame rendering and settings persistence
      config/config.*           Settings and YAML reader/writer
      input/input.h             Keyboard and mouse capture
      graphics/layout.h         Canvas size and coordinate conversion
      graphics/placeholder_art.* Fallback drawings for missing artwork
      resources/resources.*     Fonts, textures, audio and render target
      resources/asset_paths.h   Runtime font and optional artwork paths
```

Includes are relative to `game/src`, for example `#include "levels/level.h"`. `src/CMakeLists.txt` uses `GLOB_RECURSE` with `CONFIGURE_DEPENDS`: `.cpp`, `.h` and `.hpp` files under `src/` and `.cpp` tests under `tests/` are discovered recursively. Adding, removing or moving a source file triggers a refresh on the next build. Register new playable levels in `main.cpp` as described in the level guide.

CMake stages `game/resources/` on every build of the game target, including asset-only edits. Desktop resources go beside the executable; Web resources go in `${CMAKE_BINARY_DIR}/resources` for Emscripten preloading. Generated output belongs under `build/`. See [BUILDING.md](../BUILDING.md) for the CMake workflow. Makefile and Ninja generators also produce `compile_commands.json` in the build directory; other generators ignore that option.

## How the game runs

1. `main()` loads configuration and creates a `GameWindow`.
2. `GameWindow::Run()` in `main.cpp` captures input and runs one frame at a time.
3. `GameWindow::Frame()` applies timing, audio and preferences, calls `Game::Update()`, and renders the result.
4. `Game::Update()` in `main.cpp` updates the active level or reads a menu action. `Game::Apply()` handles transitions.
5. `Game::Draw()` in `main.cpp` draws the active level or selected menu. `ui/menus/menus.cpp` defines how menus look.

`Game` is declared in `game.h` and implemented in `main.cpp`, so all level routing is in one place. It owns the active `Level` and menu state. Starting a level constructs its registered class and resets it after construction. Pause and Settings retain it; Main Menu and Dev Level Select release it. Automatic level completion/progression is not implemented yet.

`GameWindow` handles platform and rendering details. It owns configuration, the Raylib context, resources and `Game`. Destruction releases the level first, then GPU/audio resources, then the context. Navigation tests can create `Game` without opening a window.

`Level` is the common base class. Each numbered folder holds a level file defining its content, a concrete class for its events, and a factory. `LevelDefinition`, `LevelId`, the base class and factory declarations are together in `levels/level.h`. Level IDs keep explicit values 0-3 so existing saved preferences still work.

## Frame loop

Each frame captures input, limits `dt` to 0-0.1 seconds, updates the current screen, applies audio/FPS changes, saves pending settings and draws.

Scene updates run dialogue, player movement, the stage's `OnUpdate`, then camera follow. Pause and Settings suspend this sequence. R/Restart resets the player, opening dialogue and elapsed time, then calls `OnReset`. Desktop focus loss pauses play.

Stage-specific events belong in `OnReset` and `OnUpdate`. See [LEVEL_AUTHORING.md](LEVEL_AUTHORING.md).

## Rendering

Everything draws into one fixed 1600 by 900 render texture. `GameWindow::Draw` renders the game, then scales that texture to `Destination()`. Normal desktop windows keep a 16:9 content area through GLFW's native resize constraint and fill the window without bars. Maximized, fullscreen and browser windows fit the canvas with letterboxing. `MouseOnCanvas` reverses the same transform for input. DPI conversion uses the display scale rather than rounded logical window dimensions.

Draw UI directly in canvas coordinates. The level uses an ordinary Raylib `BeginMode2D(camera)` / `EndMode2D()` pair for the world. `resources.BeginClip(bounds)` and `EndScissorMode()` clip in those same canvas coordinates. Window/context setup lives in `engine/window/game_window.cpp`; asset loading lives in `engine/resources/resources.cpp`.

The font loads once at startup. `resources.DrawText` and `resources.MeasureText` use that shared font; missing fonts fall back to Raylib's default. The font and final canvas copy use bilinear filtering. Resizing scales the completed canvas, with no render-target reallocation, font cache or extra UI camera transforms. Keep layout changes in `ui/menus/menus.cpp` and use the shared text helpers.

Placeholder artwork and optional texture slots are described in [ART_ASSETS.md](ART_ASSETS.md).

## Input and menus

`CaptureInput` produces one `InputFrame` per frame. Gameplay consumes its movement/jump values; menus return typed actions to `Game`.

Mouse movement or clicks can change focus. A stationary pointer does not override keyboard navigation. Sliders capture the pointer until release, including outside their bounds. Slider changes keep the existing widget and save on release.

| Context | Controls |
| --- | --- |
| Menus | Mouse, Up/Down or W/S, Tab, Enter, Esc/Backspace |
| Movement | A/D or arrows; Space/W/Up to jump |
| Pause | Esc/P |
| Dialogue | Enter to reveal; T to replay, when enabled |
| Restart | R or Restart Stage |
| Sliders | Drag, Left/Right or A/D, Home/End |

## Movement and dialogue

The player uses seconds, small collision substeps and axis-separated rectangle collisions. Ground support is checked before jump input, including immediately after spawn/reset. World bounds and authored platform rectangles define collision geometry.

`ui/dialogue/dialogue.h` declares both the reveal state and dialogue panel. `Typewriter` stores the full message and fractional reveal progress. `DialogueBox` wraps the full text, reserves word positions while revealing, clips overflowing lines and follows the latest revealed line. UTF-8 codepoints reveal together; the loaded font supplies glyph coverage.

Each `DialogueCue` specifies text, speaker, speed multiplier, delay, size and skip/replay controls. `Say` replaces the current cue. The Instant setting skips both reveal time and delay.

## Preferences

Load order is `GameConfig` defaults, packaged `config.yaml` (or `--config`), then user settings (or `--settings`). The reader accepts section maps with two-space-indented numeric/bool scalars, comments and a UTF-8 BOM. Invalid keys or values are reported and leave the previous value intact.

`--self-test` returns before configuration or window creation. `--smoke-test` loads the packaged config (or `--config`) but skips loading and saving user settings, including a supplied `--settings` file.

The FPS cap ranges from 30 to 360. `window.unlimited_fps` bypasses the saved cap using `SetTargetFPS(0)`. Volume, mute, text speed and FPS visibility apply immediately. Writes wait until mouse release; pending changes also save on exit.

Windows settings are in `%LOCALAPPDATA%/UntitledRagebaitGame/settings.yaml`. Other desktops use `XDG_DATA_HOME` or `~/.local/share/untitled_ragebait_game/`. The file stores preferences and the last selected stage, not gameplay progress.

If the required environment variables are unavailable, settings fall back to `untitled_ragebait_game/settings.yaml` under the system temporary directory. Startup still opens the main menu; saving the last selected stage does not automatically resume it.

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

Self-tests cover dialogue, config, movement, sliders, navigation, pause/reset and stage hooks. The smoke test uses a hidden Raylib window, synthetic input and real rendering. It checks font fallback, texture replacement, all screens and stage lifecycles.

Rendering tests run at 960x540, 1280x720, 1366x768, 1600x900 and 1920x1080. They check the fixed canvas, scaled screen colors, clipping, input mapping and the maximize/fullscreen transitions. On Windows they also exercise the native resize handler for all eight edges and corners. Captures include menus and level dialogue at each size. Tests do not write user preferences.

Desktop is the tested target. Browser startup code is present; browser persistence still requires IDBFS integration.
