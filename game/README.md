# Untitled Ragebait Game

CISC 320 Group 7 project. C++20 and raylib.

A sleep-deprived student has to get to class while hallucinating obstacles and listening to the Entity's unreliable advice.

- [Build instructions](../BUILDING.md)
- [Level development](../docs/LEVEL_AUTHORING.md)
- [Assets](../docs/ART_ASSETS.md)
- [Code guide and tests](../docs/CODE_GUIDE.md)

Start in [`src/main.cpp`](src/main.cpp), the level handler and game loop. Edit a level in `src/levels/`, a menu in `src/ui/menus/menus.cpp`, or movement in `src/player/player.cpp`. Window, settings and rendering support are in `src/engine/`.

Build with CMake (`CMakeLists.txt` plus `src/CMakeLists.txt`). Sources and headers anywhere under `src/` are discovered automatically, including new subfolders. Levels use numbered folders `src/levels/level_0/` through `level_3/`. Tests are in `tests/`, and runtime fonts, images, defaults and audio in `resources/`. The ARCADEPI font is in `resources/fonts/`; archival source art stays in the repository's root `assets/` folder.

See `LICENSE` and the resource license files for third-party copyright notices.
