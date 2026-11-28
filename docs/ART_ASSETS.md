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

These fixed slots still work. New characters, props, hazards and animations should use Game Data instead (next section).

Artwork does not define collision geometry. Edit platforms and hazard bounds in level/gameplay code. Animation, parallax, tile maps and nine-slice panels can be added to the rendering code as those assets are developed.

## Game Data sprites and animations

Sprites and animations are described by the **CISC 320 Game Data** Google Sheet, not by C++ code. For the steps to add a sprite (making the PNG, filling in the sheet, placing it in a level, animating it, troubleshooting), see [game/src/engine/assets/README.md](../game/src/engine/assets/README.md). The game reads an exported copy at startup, loads each PNG once, and draws placeholders for anything that has no art yet. Changing a size, frame count, FPS, pivot or layer in the sheet, or replacing a PNG, takes effect on the next launch without code changes. The sheet is the master copy; never edit the CSV by hand.

### Files

| Path under `game/resources/` | Contents |
| --- | --- |
| `data/game_data.csv` | Game Data sheet export (File → Download → Comma-separated values) |
| `art/static/` | One-frame PNGs (Frames = 1) |
| `art/animated/` | Sprite strips and grids (Frames > 1) |

The File Name column is looked up in the folder that matches its frame count, then the other folder. A File Name containing a folder, such as `tiles/street.png`, is read from `art/tiles/street.png`.

### Columns

Columns are matched by header text; keep the headers and order from the sheet. Each row is one animation; a static asset is a one-frame animation.

| Column | Accepted values | Blank means |
| --- | --- | --- |
| Animation ID | `asset_state`, lowercase with underscores (`player_walk`) | Row skipped |
| Asset ID | Groups an object's animations (`player_character`) | Row skipped |
| File Name | PNG name; `none` for an invisible object such as a trigger zone | Placeholder box until the art exists |
| Frame Width / Height (px) | Size of one frame, whole pixels | Row skipped |
| Frames | 1 or more | 1 |
| Frames per Row | Frames in each row of a grid | All frames in one row |
| FPS | Playback speed | Static: always frame 1 |
| Loops | `Yes` (idle, walk) or `No` (jump, land: stops on the last frame) | Yes if it has an FPS |
| Pivot | `bottom-centre` (characters, props), `top-left` (tiles, backgrounds), `centre` (floating objects, effects) | bottom-centre |
| Faces | `right` (or `left`) for art with a direction; mirrored when the object faces the other way | Never mirrored |
| Layer | `Background`, `Middle` (player and things it touches), `Foreground`, `UI` | Middle |
| Notes | Anything; not read by the game | |

Invalid or missing values never stop the game. Each problem is logged at startup with its sheet row, for example `game_data.csv: row 3 (player_walk): FPS "fast" is not a positive number`, and a default is used or the row is skipped. A PNG smaller than its frames need is also logged.

### Placeholders

An animation listed in Game Data without a PNG draws a translucent box at its Frame Width × Frame Height and pivot, so positions do not change when the art arrives. Objects that already had a temporary drawing keep it at the sheet's size; for example, the player keeps the student drawing until `player_character` PNGs exist. Rows with File Name `none` are never drawn and are not reported as missing.

### Player

The player uses Asset ID `player_character` and the states `idle`, `walk`, `jump` (rising) and `fall`. Name the animations `player_idle`, `player_walk`, `player_jump` and `player_fall`, or anything ending in `_<state>` with that Asset ID. A missing `fall` uses `jump`, and any other missing state uses `idle`. Draw the art facing right with Faces = `right`; it is mirrored when the player moves left. With no `player_character` rows, the original drawing is used at the collision-box size.

Sprite size does not change collision. The player's collision box stays 30 × 44 pixels, and its bottom-centre is the sprite's pivot.

### How sprites are scaled

Every sprite is scaled uniformly, never stretched, with nearest-neighbour filtering so pixels stay sharp. Artists export PNGs at 100%: the game draws each PNG pixel as `pixelScale` (2) world pixels, and the level camera's zoom (2) makes each art pixel 4×4 on screen. A sprite can also be best-fitted inside a rectangle; it then grows as large as fits without changing shape, rounded down to whole pixels. The camera and player are snapped to whole pixels while scrolling, and the window is scaled so pixel art stays crisp at any window size. The settings are `pixelScale` in `game/src/engine/assets/asset_manager.h` and the camera zoom in `game/src/levels/level.cpp`; how to add sprites is in [game/src/engine/assets/README.md](../game/src/engine/assets/README.md).

### Updating assets

1. Edit the Game Data sheet (and the art in the Assets folder).
2. Export the sheet: File → Download → Comma-separated values (.csv).
3. Replace `game/resources/data/game_data.csv` with the download (rename it to `game_data.csv`) and copy new or changed PNGs into `art/static/` or `art/animated/`.
4. Build and run. CMake copies `game/resources/` on every build; check the console for Game Data warnings.
5. Commit the CSV and PNGs together.

### Using it in code

```cpp
const auto& assets = resources.Assets();
const AnimationData* walk = assets.Find("player_walk");                // by Animation ID
const AnimationData* same = assets.Find("player_character", "walk");   // by Asset ID + state
assets.Draw(*walk, animator.Frame(*walk), feetPosition, Facing::Left);  // pivot, flip, placeholder
```

Use an `Animator` (`engine/assets/animator.h`) per object for its current state and playback time, and a `RenderQueue` to draw in layer order. Levels queue their sprites in `SubmitSprites`; see [LEVEL_AUTHORING.md](LEVEL_AUTHORING.md#layout).

## Sounds and music

Audio works the same way as Game Data sprites: an **Audio** tab in the Game Data workbook, exported to `game/resources/data/audio_data.csv`, with WAV sound effects in `game/resources/audio/sfx/` and OGG music in `game/resources/audio/music/`. See [game/src/engine/audio/README.md](../game/src/engine/audio/README.md).

## Runtime files

`game/resources/` holds runtime fonts, exported images, defaults and audio. `game/CMakeLists.txt` stages this directory on every build of the game target using `cmake -E copy_directory`. Desktop builds copy it beside the executable. With `PLATFORM=Web`, it goes in `${CMAKE_BINARY_DIR}/resources` for Emscripten's `--preload-file resources` link option. Source files such as PSDs stay in the root `assets/` folder and are not copied.

For desktop distribution, include the executable and its adjacent `resources/` directory. Include the license for any third-party asset added to the game.

The resource copy overwrites matching files but does not remove old exports. Use a fresh build directory or remove the specific staged file when testing a deleted asset.
