# Assets

## Fonts

The UI uses `game/resources/fonts/ARCADEPI.TTF`. Select the runtime font in `game/src/engine/resources/asset_paths.h`:

```cpp
inline constexpr const char* uiFontPath = "fonts/ARCADEPI.TTF";
```

Put runtime fonts in `game/resources/fonts/`. CMake stages the complete `game/resources/` directory on every build of the game target. On desktop it goes beside the executable; see Runtime files below for Web builds. Rebuild and relaunch after changing a font.

The custom font loads once at startup into a shared 64-pixel glyph atlas. Drawing and measurement use that same font. There is no per-size font cache.

All drawing uses a fixed 1600 by 900 canvas, scaled to the window with bilinear filtering. Normal window resizing keeps a 16:9 content area without bars; maximized and fullscreen windows allow letterboxing. The final copy preserves the colors already composited in the canvas.

Use the shared text functions for UI:

```cpp
DrawTextFit(resources, "SETTINGS", {70, 64, 500, 50}, 44, WHITE);
DrawParagraph(resources, description, {70, 140, 500, 120}, 24, WHITE);
resources.DrawText("FPS", {1410, 18}, 20, 1, WHITE);
```

Positions and sizes above are canvas coordinates. `resources.MeasureText(text, size)` returns dimensions in those same coordinates. For UI clipping, use `resources.BeginClip(bounds)` and `EndScissorMode()`.

Missing or unreadable fonts fall back to Raylib's default font and log a warning. The custom-font loader currently requests printable ASCII; supply an explicit codepoint list to `LoadFontEx` in `engine/resources/resources.cpp` when adding localized text.

## Exported artwork

Export runtime PNGs under `game/resources/art/`. Desktop builds copy them to the executable's `resources/art/` directory; Web builds preload them with the other runtime files. Paths are defined in `asset_paths.h`; images are loaded at startup.

The table lists optional image slots recognized by the loader. A slot does not require an exported PNG to exist; missing images use the placeholder drawings below. To add a slot, add an entry before `ArtId::Count` and a matching path in `artPaths`, in the same order, in `engine/resources/asset_paths.h`.

| PNG under `game/resources/art/` | Use | Placement |
| --- | --- | --- |
| `menus/main.png` | Main-menu background | Full logical canvas |
| `menus/settings.png` | Settings background | Full logical canvas |
| `menus/stage_select.png` | Stage-selection background | Full logical canvas |
| `menus/pause.png` | Pause overlay | Full logical canvas |
| `characters/student.png` | Student | Aspect preserved, bottom aligned |
| `characters/entity.png` | Entity | Aspect preserved, bottom aligned |
| `world/platform.png` | Platform surface | Stretched to each platform rectangle |
| `levels/level_0.png` | House backdrop | World bounds |
| `levels/level_1.png` | Streets backdrop | World bounds |
| `levels/level_2.png` | Stairwell backdrop | World bounds |
| `levels/level_3.png` | Lecture-hall backdrop | World bounds |
| `ui/dialogue.png` | Dialogue panel | Panel bounds |
| `ui/button.png` | Filled button | Button bounds |

Missing textures use the shape drawings in `engine/graphics/placeholder_art.cpp`. `Resources::DrawArt` returns whether an image was drawn so each caller can supply its fallback. Character PNGs should be tightly framed; transparent padding affects the displayed size. A complete menu background replaces its fallback illustration, including the characters in that illustration.

Keep labels and button text out of background images. Menus draw text, focus outlines and controls separately. Widget drawing is in `ui/`; colors are in `ui/theme.h`.

Artwork does not define collision geometry. Edit platforms and hazard bounds in level/gameplay code. Animation, parallax, tile maps and nine-slice panels can be added to the rendering code as those assets are developed.

## Runtime files

`game/resources/` holds runtime fonts, exported images, defaults and audio. `game/CMakeLists.txt` stages this directory on every build of the game target using `cmake -E copy_directory`. Desktop builds copy it beside the executable. With `PLATFORM=Web`, it goes in `${CMAKE_BINARY_DIR}/resources` for Emscripten's `--preload-file resources` link option. Source files such as PSDs stay in the root `assets/` folder and are not copied.

For desktop distribution, include the executable and its adjacent `resources/` directory. Include the license for any third-party asset added to the game.

The resource copy overwrites matching files but does not remove old exports. Use a fresh build directory or remove the specific staged file when testing a deleted asset.
