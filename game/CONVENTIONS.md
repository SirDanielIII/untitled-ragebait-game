# C++ conventions

Use C++20 and four spaces. Match the surrounding code; open braces on their own line. Initialize members and keep ownership explicit (`std::unique_ptr` for an owned polymorphic level).

## Names

| Element | Convention | Example |
| --- | --- | --- |
| Files and folders | `snake_case` | `levels/level_3/level_3.cpp` |
| Classes, enums and enum values | `PascalCase` | `Level3`, `LevelId::Level3` |
| Functions and methods | `PascalCase` | `MakeLevel3`, `OnUpdate` |
| Variables, members and constants | `camelCase` | `activeLevel`, `moveSpeed`, `canvasWidth` |
| Preprocessor platform flags | `UPPER_SNAKE_CASE` | `PLATFORM_WEB` |

Number levels consistently across folders, files, IDs, classes, definitions and factories. For example: `levels/level_1/level_1.cpp`, `LevelId::Level1`, `Level1`, `Level1Definition`, `MakeLevel1`. Level 0 is the tutorial; levels 1-3 follow it. Keep numeric IDs stable because saved preferences use them. Location descriptions belong in level content and documentation.

## Keep related code together

- `main.cpp` is the level handler. Put level registration, screen transitions and the frame loop there; `game.h` declares its state.
- A playable level gets its own numbered folder: `levels/level_0/`, `levels/level_1/`, etc. Keep its definition, class, event state and factory in `level_<number>.cpp`; place future level-specific helpers beside it. Shared behavior belongs in `levels/level.*`.
- Put source/header pairs together in a feature folder: `player/player.*`, `ui/dialogue/dialogue.*`, `engine/window/game_window.*`.
- Keep menu layouts in `ui/menus/`; shared menu controls, sliders, dialogue and text each have their own folder under `ui/`. `ui/theme.h` contains shared colors.
- `player/` owns movement/collision. `engine/` groups `window/`, `config/`, `input/`, `resources/` and `graphics/` support.
- Include paths start at `game/src`, for example `#include "ui/dialogue/dialogue.h"`.
- `src/CMakeLists.txt` recursively discovers `.cpp`, `.h` and `.hpp` files. Add or remove files and build normally; no manual source list is needed. Keep scratch programs out of feature folders.
- Tests live in `game/tests/`; their `.cpp` files are also discovered recursively and provide the existing `--self-test` / `--smoke-test` commands.
- Runtime assets go under `game/resources/`; original artwork goes under root `assets/`. Build output stays under `build/`.

## Runtime rules

Pass frame time in seconds. Levels implement `OnReset`, `OnUpdate` and optional `DrawWorld`; the central handler owns the loop and the window lifetime. Store level event state as class members so restart clears it and separate levels do not share it.

Use the shared text/clip helpers for UI so rendering and mouse hit testing agree at different window sizes. Keep screen navigation in `main.cpp` and menu appearance in `ui/menus/menus.cpp`.

See the [code guide](../docs/CODE_GUIDE.md) and [level guide](../docs/LEVEL_AUTHORING.md) for examples and verification commands.
