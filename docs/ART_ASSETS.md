# Assets

## Fonts

The font roles are defined in `game/src/engine/resources/asset_paths.h`: Silkscreen Regular for ordinary text, Silkscreen Bold for headings, and Jersey10-Regular for typewriter dialogue, level-select descriptions and native settings keycaps. Place the three TTFs (`Silkscreen-Regular.ttf`, `Silkscreen-Bold.ttf`, `Jersey10-Regular.ttf`) in `game/resources/fonts/`. CMake stages runtime resources on every build. Missing or unreadable fonts log a warning and fall back to Raylib's default.

Font bytes are read once. Bitmap glyph atlases are cached by role and physical raster size, use nearest-neighbour filtering and pixel-aligned drawing, and have a per-face soft cache budget of 48 sizes / 16 MiB. Layout measurements use resolution-independent logical advances. The world and UI render directly to the native framebuffer, using an aspect-fitted 1600×900 logical viewport. Sprite artwork remains point-filtered and keeps its existing Game Data mapping, pivot, frame, facing and layer.

Use the shared text functions for UI and world labels:

```cpp
DrawTextFit(resources, "SETTINGS", {70, 64, 500, 50}, 44, WHITE, FontRole::Heading);
DrawParagraph(resources, description, {70, 140, 500, 120}, 20, WHITE, FontRole::Typewriter);
resources.DrawText("FPS", {1410, 18}, 20, 1, WHITE);
```

Positions and sizes are logical coordinates. Use `resources.MeasureText` with the same font role and spacing as drawing. For UI clipping, use `resources.BeginClip(bounds)` and `EndScissorMode()`. The font loader's `UiCodepoints()` list includes printable ASCII, Latin-1 and common punctuation; extend it when adding other languages.

## Exported artwork

Export runtime PNGs under `game/resources/art/`. Desktop builds copy them to the executable's `resources/art/` directory; Web builds preload them with the other runtime files. Menu images are listed in Game Data (`data/game_data.csv`); legacy non-menu paths are defined in `asset_paths.h`. Images load at startup.

Menu art uses the same Game Data AssetManager as world sprites. The C++ menus reference Asset IDs, not filenames. Use one static animation per slot (`Animation ID` = `Asset ID` + `_idle`, Frames = 1, Pivot = centre, Layer = UI, no FPS or Faces). Use a folder-qualified filename relative to `art/`; set Frame Width and Frame Height to the actual source PNG dimensions (not the logical slot). Update the sheet whenever you rename an image or change its dimensions. The runtime CSV includes these rows; copy them into the master Google Sheet before the next export.

At 3840×2160 the 1600×900 logical canvas scales by 2.4. Use the following 4K-ready source sizes; images are contained in their logical slots, with point filtering. No world-sprite scaling rules have changed.

| Asset ID | File Name | Logical slot | Recommended PNG |
| --- | --- | --- | --- |
| `menu_main` | `menus/main.png` | 1600×900 | 3840×2160 |
| `menu_settings` | `menus/settings.png` | 1600×900 | 3840×2160 |
| `menu_level_select` | `menus/stage_select.png` | 1600×900 | 3840×2160 |
| `menu_pause` | `menus/pause.png` | 1600×900 | 3840×2160 |
| `menu_pause_sprite` | `menus/pause_sprite.png` | 680×510 | 1920×1440 |
| `menu_preview_level_0` | `menus/previews/level_0.png` | 576×324 | 1920×1080 |
| `menu_preview_level_1` | `menus/previews/level_1.png` | 576×324 | 1920×1080 |
| `menu_preview_level_2` | `menus/previews/level_2.png` | 576×324 | 1920×1080 |
| `menu_preview_level_3` | `menus/previews/level_3.png` | 576×324 | 1920×1080 |

Legacy optional non-menu slots (unchanged):

| PNG under `game/resources/art/` | Use | Placement |
| --- | --- | --- |
| `characters/student.png` | Student | Aspect preserved, bottom aligned |
| `characters/entity.png` | Entity | Aspect preserved, bottom aligned |
| `world/platform.png` | Platform surface | Stretched to each platform rectangle |
| `levels/level_0.png` | House backdrop | World bounds |
| `levels/level_1.png` | Streets backdrop | World bounds |
| `levels/level_2.png` | Stairwell backdrop | World bounds |
| `levels/level_3.png` | Lecture-hall backdrop | World bounds |
| `ui/dialogue.png` | Dialogue panel | Panel bounds |
| `ui/button.png` | Filled button | Button bounds |

Missing menu backgrounds use the opaque solid `theme::background` color; other missing textures use the shape drawings in `engine/graphics/placeholder_art.cpp`. `art::DrawMenuArt` returns whether a CSV-defined menu image was drawn; `Resources::DrawArt` remains for legacy non-menu art. Character PNGs should be tightly framed; transparent padding affects the displayed size. For the main menu, export a 16:9 PNG (3840×2160) as `menus/main.png`. It fills the logical viewport behind the title, buttons and description, without any extra illustrated placeholders. The top and right side are reserved for the background artwork.

The pause illustration is omitted entirely when missing. The level selector shows an empty dark preview rectangle until its corresponding PNG is present; export previews at 1920×1080 (16:9), and the transparent pause illustration at 1920×1440 (4:3). These are Game Data assets, independent of world backdrops and sprite assignments. The level selector keeps the preview at a fixed position across all four levels; descriptions are authored in `LevelPreviewDescription()` in `ui/menus/menus.cpp`, not in the CSV. The packaged test PNGs can be replaced with final art at the same dimensions, or at other dimensions with matching sheet rows.

Keep labels and button text out of background images. Menus draw text, focus outlines and controls separately. Widget drawing is in `ui/`; colors are in `ui/theme.h`.

Legacy non-menu fixed slots still work. New assets should use Game Data (next section).

Artwork does not define collision geometry. Edit platforms and hazard bounds in level/gameplay code. Animation, parallax, tile maps and nine-slice panels can be added to the rendering code as those assets are developed.

## Game Data sprites and animations

Sprite metadata and animation frames are described by the **CISC 320 Game Data** Google Sheet. The level-specific `level_<n>_sprites.cpp` files bind object IDs to Asset IDs; menu slots reference their IDs directly. For the steps to add a sprite (making the PNG, filling in the sheet, placing it in a level, animating it, troubleshooting), see [game/src/engine/assets/README.md](../game/src/engine/assets/README.md). The game reads an exported copy at startup, loads each PNG once, and uses the appropriate stand-in or missing-art fallback for each slot. Changing a size, frame count, FPS, pivot or layer in the sheet, or replacing a PNG, takes effect on the next launch without code changes. The sheet is the master copy; never edit the CSV by hand.

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

The player uses Asset ID `player_character` and the states `idle`, `walk`, `jump` (rising), `fall`, `crouch` and `crouch_walk`. Name the animations `player_idle`, `player_walk`, `player_jump` and `player_fall`, or anything ending in `_<state>` with that Asset ID. A missing `fall` uses `jump`; `crouch_walk` can use `crouch`. If crouch art is unavailable, standing art is compressed to the crouch height. Other missing states fall back to idle or the first available animation. Draw the art facing right with Faces = `right`; it is mirrored when the player moves left. With no `player_character` rows, the original drawing is used at the collision-box size.

Sprite size does not change collision. The packaged standing collision box is 30 × 44 world pixels; crouching reduces its height. Collision tuning is independent of art. The player sprite is anchored at the visual feet (bottom-centre).

### How sprites are scaled

Game Data world sprites use nearest-neighbour filtering and uniform scaling. The default `pixelScale` is 2 world pixels per source pixel; with camera zoom 2 that is 4 logical canvas pixels, before native framebuffer scaling. `DrawFitted` rounds to whole logical screen pixels with a minimum of one, so oversized source art can exceed a small target. `art::DrawMenuArt` instead permits fractional downscaling and centers artwork in its logical slot. The camera and sprite anchors use logical pixel snapping; fractional framebuffer scales may produce uneven physical pixel widths. Legacy `Resources::DrawArt` can explicitly stretch backgrounds or panels. The settings are `pixelScale` in `game/src/engine/assets/asset_manager.h` and the camera zoom in `game/src/levels/level.cpp`; how to add sprites is in [game/src/engine/assets/README.md](../game/src/engine/assets/README.md).

### Updating assets

1. Edit the Game Data sheet (and the art in the Assets folder).
2. Export the sheet: File → Download → Comma-separated values (.csv).
3. Replace `game/resources/data/game_data.csv` with the download (rename it to `game_data.csv`) and copy new or changed PNGs into `art/static/`, `art/animated/`, or the folder-qualified path under `art/` (e.g. `art/menus/previews/`).
4. Build and run. CMake copies `game/resources/` on every build; check the console for Game Data warnings.
5. Commit the CSV and PNGs together.

### Using it in code

```cpp
const auto& assets = resources.Assets();
const AnimationData* walk = assets.Find("player_walk");                // by Animation ID
const AnimationData* same = assets.Find("player_character", "walk");   // by Asset ID + state
if (walk) assets.Draw(*walk, animator.Frame(*walk), feetPosition, Facing::Left);  // pivot, flip, placeholder
```

Use an `Animator` (`engine/assets/animator.h`) per object for its current state and playback time, and a `RenderQueue` to draw in layer order. Levels queue their sprites in `SubmitSprites`; see [LEVEL_AUTHORING.md](LEVEL_AUTHORING.md#layout).

## Sounds and music

Audio works the same way as Game Data sprites: an **Audio** tab in the Game Data workbook, exported to `game/resources/data/audio_data.csv`, with WAV sound effects in `game/resources/audio/sfx/` and OGG music in `game/resources/audio/music/`. See [game/src/engine/audio/README.md](../game/src/engine/audio/README.md).

## Runtime files

`game/resources/` holds runtime fonts, exported images, defaults and audio. `game/CMakeLists.txt` stages this directory on every build of the game target using `cmake -E copy_directory`. Desktop builds copy it beside the executable. With `PLATFORM=Web`, it goes in `${CMAKE_BINARY_DIR}/resources` for Emscripten's `--preload-file resources` link option. Source files such as PSDs stay in the root `assets/` folder and are not copied.

For desktop distribution, include the executable and its adjacent `resources/` directory. Include the license for any third-party asset added to the game.

The resource copy overwrites matching files but does not remove old exports. Use a fresh build directory or remove the specific staged file when testing a deleted asset.
