# Asset system (Game Data)

Every sprite and animation in the game is described in the **CISC 320 Game Data** Google Sheet instead of in C++. When the game starts, it reads an exported copy of that sheet, loads each PNG once, and draws a placeholder for anything that has no art yet.

- **Changing art** (a new PNG, a different size, frame count, speed, pivot or layer) needs **no code**.
- **Putting a new object in a level** needs a few lines in that level's file, because the sheet says *what* something looks like, not *where* it goes.

Contents: [Folders](#folders) · [1. Make the art](#1-make-the-art) · [2. Describe it in Game Data](#2-describe-it-in-game-data) · [3. Get it into the game](#3-get-it-into-the-game) · [4. Player art](#4-player-art) · [5. Place objects in a level](#5-place-objects-in-a-level) · [6. Animated objects](#6-animated-objects) · [Changing existing art](#changing-existing-art) · [Placeholders and invisible objects](#placeholders-and-invisible-objects) · [Troubleshooting](#troubleshooting) · [Code reference](#code-reference) · [Tests](#tests) · [Committing](#committing)

## Folders

| Path | What goes there |
| --- | --- |
| `game/resources/data/game_data.csv` | The Game Data sheet, exported as CSV |
| `game/resources/art/static/` | One-frame PNGs (props, signs, backgrounds) |
| `game/resources/art/animated/` | Animation strips and grids (Frames > 1) |
| `game/src/engine/assets/` | The code (this folder) |

The game reads a copy of `game/resources/` that is placed next to the executable when you build. **After changing anything in `game/resources/`, click Build before running.**

## 1. Make the art

Use any program that can save a PNG: Aseprite, LibreSprite, Piskel, Krita, GIMP, Photoshop, Procreate, and so on. The game only reads PNG files, never a program's own project format (`.aseprite`, `.psd`, `.kra`…), so always export a PNG.

Every PNG must follow these rules:

1. **PNG with a transparent background.** Otherwise a coloured box shows around the sprite.
2. **Crop tightly.** The canvas size is what you will enter as Frame Width and Frame Height. Empty space at the bottom makes ground objects look like they float.
3. **Animations go in one image, frames side by side**, left to right in play order, all the same size, with **no gaps or borders** between them. Four 32×32 frames make one 128×32 PNG.
4. **Grids** (for example a tile set) fill each row left to right, then the next row down. Every cell is one frame of the same size.
5. **Draw characters facing right** (or consistently left). The game mirrors them when they turn around, so never draw a separate left-facing version.
6. **Size:** export at **100%**. The game draws every PNG pixel as `pixelScale` (2) game pixels, set in `asset_manager.h`, and the level camera then zooms 2×, so each art pixel ends up 4×4 on screen (the same as Stardew Valley). Enter the PNG's real size as Frame Width and Height. If you ever do scale pixel art, use a whole number (200%, 300%) and **nearest-neighbour** scaling; other modes blur it.

Where to find this in common programs:

| Program | Single image | Animation strip |
| --- | --- | --- |
| Aseprite / LibreSprite | File → Export As (Resize 100%) | File → Export Sprite Sheet: Sheet Type Horizontal Strip; Border Padding, Spacing and Inner Padding all 0 |
| Piskel | Export → PNG | Export → PNG → spritesheet, 1 row |
| Krita / GIMP / Photoshop | Export as PNG | Lay the frames out side by side on one canvas, then export as PNG |

Save the PNG straight into `game/resources/art/static/` (one frame) or `game/resources/art/animated/` (several frames). File names should be lowercase with underscores, e.g. `school_desk.png`.

## 2. Describe it in Game Data

Add one row per animation to the **Game Data** sheet. A static image is one row with one frame. A character with idle, walk and jump has three rows.

| Column | What to enter | If left blank |
| --- | --- | --- |
| **Animation ID** | What code asks for: `asset_state`, lowercase with underscores (`player_walk`). For a static image it can match the Asset ID. Must be unique. | Row ignored |
| **Asset ID** | The object it belongs to (`player_character`). All of an object's animations share it. | Row ignored |
| **File Name** | The PNG's file name (`player_walk.png`). Write `none` for an object that is never drawn. | Placeholder box until the art exists |
| **Frame Width (px)** / **Frame Height (px)** | Size of **one** frame (not the whole strip), in whole pixels | Row ignored |
| **Frames** | Number of frames in the PNG | 1 |
| **Frames per Row** | Only for grids: how many frames in each row | All frames in one row |
| **FPS** | Frames per second for animations | Not animated: always shows frame 1 |
| **Loops** | `Yes` repeats (idle, walk). `No` plays once and stays on the last frame (jump, land). | Yes, if it has an FPS |
| **Pivot** | The point placed at the object's position. `bottom-centre`: characters and anything on the ground. `top-left`: tiles and backgrounds. `centre`: floating things and effects. | bottom-centre |
| **Faces** | `right` or `left`, the way the drawing faces. Leave blank for art with no direction. | Never mirrored |
| **Layer** | `Background` (behind everything), `Middle` (the player and what it touches), `Foreground` (in front of the player), `UI` (on top) | Middle |
| **Notes** | Anything. The game ignores it. | |

Example rows:

```
Animation ID,Asset ID,File Name,Frame Width (px),Frame Height (px),Frames,Frames per Row,FPS,Loops,Pivot,Faces,Layer,Notes
school_desk,school_desk,school_desk.png,32,32,1,,,,bottom-centre,,Middle,static prop
player_walk,player_character,player_walk.png,16,32,6,,12,Yes,bottom-centre,right,Middle,looping animation
player_jump,player_character,player_jump.png,16,32,3,,10,No,bottom-centre,right,Middle,plays once
street_tiles,street_tiles,street_tiles.png,16,16,8,4,,,top-left,,Background,4x2 grid of tiles
anvil_fall,anvil,,32,32,4,,8,No,centre,,Foreground,art not made yet: placeholder
exit_trigger,trigger,none,64,64,1,,,,top-left,,Middle,invisible
```

Keep the header row exactly as it is in the sheet, and don't rename or reorder columns without telling the team.

## 3. Get it into the game

1. In Google Sheets: **File → Download → Comma-separated values (.csv)**.
2. Rename the download to `game_data.csv` and replace `game/resources/data/game_data.csv`.
3. Make sure the PNGs are in `art/static/` or `art/animated/`.
4. **Build**, then run the game.
5. Read the console (the terminal the game was started from). Every problem is listed there, e.g.
   `game_data.csv: row 4 (player_walk): FPS "fast" is not a positive number; showing the first frame only`.
   A healthy start ends with a summary like `Game Data: 6 animations, 4 textures, 1 placeholders`.

The Google Sheet is the master copy. For real changes, edit the sheet and export again instead of editing the CSV by hand.

## 4. Player art

The player needs no code. Give its rows the Asset ID **`player_character`** and Animation IDs ending in its states:

| State | Plays when | Example ID |
| --- | --- | --- |
| `idle` | Standing still on the ground | `player_idle` |
| `walk` | Moving on the ground | `player_walk` |
| `jump` | Rising | `player_jump` |
| `fall` | Falling | `player_fall` |

- Missing states fall back: `fall` uses `jump`, and anything else uses `idle`. A single `player_idle` row is enough to see the art.
- With Faces `right`, the sprite is mirrored when the player moves left.
- The sprite's bottom-centre sits at the bottom-centre of the player's 30×44 collision box. Art size never changes collision.
- With no `player_character` rows at all, the original temporary drawing is used.

## 5. Place objects in a level

Each level's code is in `game/src/levels/level_<number>/level_<number>.cpp`. Near the bottom is the level's class (`Level1` and so on). Add a `SubmitSprites` function inside it, next to `OnReset` and `OnUpdate`:

```cpp
class Level1 final : public Level {
    // ... existing code ...
private:
    // ... OnReset, OnUpdate ...

    // Called every frame while drawing. Queue this level's sprites here.
    void SubmitSprites(RenderQueue &queue, const Resources &resources) const override {
        const auto &assets = resources.Assets();
        if (const auto *desk = assets.Find("school_desk"))
            queue.Submit(assets, *desk, 0, {600, 650});      // frame 0, pivot at x 600, y 650
        if (const auto *sign = assets.Find("exit_sign"))
            queue.Submit(assets, *sign, 0, {2400, 560});
    }
};
```

- `assets.Find("school_desk")` must match the **Animation ID** exactly. If the ID isn't in the sheet it returns `nullptr` and the `if` skips it, so nothing breaks.
- The sprite goes on the **Layer** from the sheet automatically.
- Build and run to see it.

### Choosing coordinates

Positions are **world pixels**, the same units as the level's `.platforms`, `.spawn` and `.world` at the top of the same file:

```cpp
.world = {0, 0, 2600, 700},        // level is 2600 wide, 700 tall
.spawn = {100, 606},               // where the player starts
.platforms = {
    {0, 650, 2600, 50},            // {x, y, width, height}: the floor; its top is y = 650
    {400, 545, 250, 24},           // a platform from x 400 to 650, top at y = 545
```

- `x` grows to the right from 0. `y` grows **downward** from 0 at the top.
- The position is where the **pivot** goes. For `bottom-centre`, use the top `y` of whatever it stands on: `{600, 650}` stands on Level 1's floor, and `{525, 545}` stands on the middle of the second platform.
- For `top-left`, the position is the image's top-left corner. For `centre`, it is the middle of the image.
- Expect some trial and error: change the numbers, build, look.

### Making it solid

A sprite is only a picture, so the player walks through it. To make something solid, also add a rectangle in the same place to `.platforms`. Hazards and triggers need their own gameplay code in `OnUpdate`. Keep their rectangles in the level code, not in the art.

## 6. Animated objects

An animated object needs an `Animator` to track its state and playback time. Add one per object as a member of the level class:

```cpp
class Level1 final : public Level {
    // ...
private:
    Animator anvilAnimator;                 // what the anvil is doing, and for how long
    Vector2 anvilPosition{1200, 300};
    bool anvilDropped = false;

    void OnReset() override {
        anvilAnimator = {};                 // restart clears everything
        anvilDropped = false;
    }

    void OnUpdate(float dt, const InputFrame &input) override {
        if (!anvilDropped && GetPlayer().Position().x > 1100) {
            anvilDropped = true;
            anvilAnimator.SetState("fall"); // finds the anvil's "..._fall" animation
        }
        anvilAnimator.Update(dt);           // advance time every frame
    }

    void SubmitSprites(RenderQueue &queue, const Resources &resources) const override {
        if (!anvilDropped) return;
        const auto &assets = resources.Assets();
        if (const auto *anvil = assets.Find("anvil", anvilAnimator.State()))
            queue.Submit(assets, *anvil, anvilAnimator.Frame(*anvil), anvilPosition);
    }
};
```

- `SetState("fall")` restarts the animation only when the state changes, so it is safe to call every frame.
- `assets.Find("anvil", "fall")` finds the row with Asset ID `anvil` whose Animation ID ends in `_fall`.
- `anvilAnimator.Frame(*anvil)` picks the frame from the sheet's FPS and Loops. `anvilAnimator.Finished(*anvil)` tells you when a non-looping animation is done.
- **Facing:** pass a fifth argument to mirror art, e.g. `queue.Submit(assets, *anvil, frame, position, Facing::Left)`. The image is mirrored when that is the opposite of the row's Faces value.

## Changing existing art

No code is needed for any of these. Change it, export the CSV if the sheet changed, then build and run:

- **New or better PNG:** replace the file, keeping the same name. If its size changed, update Frame Width and Frame Height.
- **More or fewer frames:** export the new strip and change Frames.
- **Faster or slower:** change FPS.
- **Play once instead of looping:** change Loops.
- **Draw in front of or behind the player:** change Layer.
- **Line up differently:** change Pivot.

Placement in levels stays the same because positions refer to the pivot, not the image size.

## Placeholders and invisible objects

- **Art not made yet:** add the row with the planned size and leave File Name blank (or name a PNG that doesn't exist yet). The game draws a translucent magenta box of that size at the pivot, so programmers can place and test objects before the art exists. When the PNG arrives, it replaces the box on the next build with no code changes.
- **Never drawn:** File Name `none`, for example trigger zones. These rows can still be looked up and placed, but nothing is drawn and they are never reported as missing.

## Troubleshooting

| What you see | Likely cause |
| --- | --- |
| Nothing changed after editing art or the CSV | You didn't build. The game reads the copy made at build time. |
| A magenta box instead of your art | PNG not found. Check the console for `... was not found in resources/art/...`, and check the File Name spelling and folder. |
| Nothing at all where the sprite should be | The ID in `assets.Find("...")` doesn't match the sheet, the row was skipped (see console), or File Name is `none`. |
| `row N ...` warnings at startup | A bad value in that sheet row. The message says which column and what was used instead. |
| `... is 64x32 but ... needs at least 128x32` | The PNG has fewer frames than the Frames column, or Frame Width is wrong. |
| Sprite floats above the ground | Empty space at the bottom of the PNG; crop it. Or the Pivot isn't `bottom-centre`. |
| Sprite is the wrong size | Frame Width and Height must match the PNG exported at 100%. The overall size is set by `pixelScale` in `asset_manager.h`. |
| Sprite is blurry | The PNG was resized in the art program. Export at 100%, or use nearest-neighbour scaling if you must resize. |
| Animation doesn't move | FPS is blank or 0, or Frames is 1. |
| Animation stops on the last frame | Loops is `No`. |
| Character faces the wrong way | Faces is wrong, or blank (never mirrored). |
| `column H header is "bottom-centre" but should be "FPS"` | A header in the sheet is misnamed. The game still reads the column by position, but please fix the sheet. |

## Code reference

| File | What it does |
| --- | --- |
| `game_data.h/.cpp` | Reads and checks the CSV. `AnimationData` is one row; `GameData` holds all rows with lookups; also the frame, timing, pivot and flip maths. No raylib, so it is testable without a window. |
| `asset_manager.h/.cpp` | `AssetManager` loads each PNG once, unloads them on exit, and draws a frame at its pivot (mirrored if needed) or a placeholder. |
| `animator.h` | `Animator`: one object's current state and playback time. |
| `render_queue.h` | `RenderQueue`: collects draw jobs and runs them Background → Middle → Foreground → UI. |

The calls you will use:

```cpp
const auto &assets = resources.Assets();                    // available anywhere you have Resources
const AnimationData *a = assets.Find("player_walk");        // by Animation ID (nullptr if missing)
const AnimationData *b = assets.Find("player_character", "walk"); // by Asset ID + state
queue.Submit(assets, *a, frame, {x, y}, Facing::Right);     // draw on its layer (inside SubmitSprites)
queue.Submit(Layer::Foreground, [] { /* any raylib drawing */ });

Animator animator;
animator.SetState("walk");                                  // in OnUpdate
animator.Update(dt);
int frame = animator.Frame(*a);                             // when drawing
bool done = animator.Finished(*a);                          // for non-looping animations
```

How it fits together: `Resources` owns the one `AssetManager` and loads it at startup. `Level::Draw` creates a `RenderQueue` each frame, queues the backdrop, platforms, the level's `SubmitSprites`, then the player, and flushes it. The player's animation is chosen in `Level::UpdatePlayerAnimation` and drawn in `Level::SubmitPlayer` (`levels/level.cpp`).

**One rule for queued lambdas:** jobs run later, at `Flush()`, so a lambda must not capture a temporary local variable by reference (`[&]`). Capture it by copy instead. `queue.Submit(assets, *sprite, ...)` is always safe.

## Tests

The tests are in `game/tests/asset_tests.cpp`. `--self-test` checks CSV import, validation, lookups, frame maths, layer order and player states without a window. `--smoke-test` loads real PNGs and checks the drawn pixels for pivots, flipping, grids, placeholders and layers.

```powershell
ctest --test-dir build --output-on-failure
& './build/untitled-ragebait-game/untitled-ragebait-game.exe' --smoke-test
```

## Committing

Commit the CSV and its PNGs together, so the sheet and the art always match. Don't commit `build/` (it's ignored automatically) or program project files such as `.aseprite` and `.psd`. Source artwork goes in the root `assets/` folder, which Git also ignores.
