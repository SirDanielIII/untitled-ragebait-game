# Untitled Ragebait Game

CISC 320 Group 7 project. C++20 and raylib.

A sleep-deprived student has to get to class while hallucinating obstacles and listening to the Entity's unreliable advice.

- [Build instructions](../BUILDING.md)
- [Level development](../docs/LEVEL_AUTHORING.md)
- [Assets](../docs/ART_ASSETS.md)
- [Code guide and tests](../docs/CODE_GUIDE.md)

Start in [`src/main.cpp`](src/main.cpp), the level handler and game loop. Edit a level in `src/levels/`, a menu in `src/ui/menus/menus.cpp`, or movement in `src/player/player.cpp`. Window, settings and rendering support are in `src/engine/`.

Build with CMake (`CMakeLists.txt` plus `src/CMakeLists.txt`). Sources and headers anywhere under `src/` are discovered automatically, including new subfolders. Levels use numbered folders `src/levels/level_0/` through `level_3/`. Tests are in `tests/`, and runtime fonts, images, defaults and audio in `resources/`. Place Silkscreen-Regular.ttf, Silkscreen-Bold.ttf and Jersey10-Regular.ttf in `resources/fonts/`; archival source art stays in the repository's root `assets/` folder.

## Native rendering

Rendering is always native to the physical framebuffer. World, UI, clipping and input retain a 1600×900 logical coordinate system, aspect-fitted to the window at every size. Bitmap font atlases are cached by physical raster size, while layout uses resolution-independent logical metrics. Sprites remain point-filtered; fractional scaling can produce unavoidable one-pixel sampling differences.

## Fullscreen and controls

On desktop, toggle fullscreen from Display settings, with F11, or with either Alt key + Enter (including keypad Enter). The setting is saved as `window.fullscreen` and defaults to off in older settings files. Leaving fullscreen restores the previous windowed mode; the native framebuffer and 1600×900 logical layout are unchanged. The Controls panel is a read-only reference for current bindings. Its native keycaps are drawn by `DrawKeycap` in `src/ui/menus/menus.cpp`, using Jersey10-Regular with centered visible glyphs. No keyboard sprites are needed. Browser fullscreen is handled by the browser.

## Menu backgrounds

Menus default to the opaque solid `theme::background` color in `src/ui/theme.h`. To add main-menu artwork, export a 16:9 PNG (ideally 1600×900 or higher) to `resources/art/menus/main.png` and rebuild. The image fills the logical viewport behind the title, buttons and description; the right side and top are reserved for the artwork. Optional background images for the other menus still work.
