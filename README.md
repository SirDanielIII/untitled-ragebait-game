# 🎮 Untitled Ragebait Game

## About

Group project for CISC 320 Group 7 (2026 Fall).

## Group Members

1. Daniel Zhuo (Leader)
2. Boaz Park (Architect)
3. Kavin Arasu (Tester)
4. Hunter Anwar (Documentation (Web Dev / UI Designer))
5. Ethan See-To (Interface Specialist)
6. Noah Segal (Programmer)
7. Reya Patel (Programmer)

## Start developing here

Open [`game/src/main.cpp`](game/src/main.cpp) for the level handler: level registration, screen switching, pause/restart and the frame loop.

```text
game/
  CMakeLists.txt       CMake target, raylib dependency and resource staging
  src/
    main.cpp          Level handler and game loop
    game.h            Game state declarations
    player/           Movement and collisions
    levels/           Shared Level class + level_0/ through level_3/
    ui/               Feature folders: menus, menu, slider, dialogue, text
    engine/           Feature folders: window, config, input, resources, graphics
  resources/          Runtime fonts, exported images, audio and defaults
  tests/              Gameplay and rendering checks
  projects/VSCode/    VS Code workspace
docs/                 Development guides
assets/               Source artwork (PSD, etc.)
build/                Generated builds and captures; do not commit
```

- [Build and run](BUILDING.md): Windows prerequisites, VS Code and CLion setup.
- [Code guide](docs/CODE_GUIDE.md): where to edit each feature and how the game runs.
- [Level development](docs/LEVEL_AUTHORING.md): layout, dialogue, events and adding levels.
- [Assets](docs/ART_ASSETS.md): fonts, images and runtime files.
- [Code conventions](game/CONVENTIONS.md): naming and organization.
