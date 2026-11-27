# Asset system (Game Data)

Every sprite and animation in the game is described in the **CISC 320 Game Data** Google Sheet instead of in C++. When the game starts, it reads an exported copy of that sheet, loads each PNG once, and draws a placeholder for anything that has no art yet.

- **Changing art** (a new PNG, a different size, frame count, speed, pivot or layer) needs **no code**.
- **Putting a new object in a level** needs one line in that level's file, because the sheet says *what* something looks like, not *where* it goes.

Contents: [How to add a sprite](#how-to-add-a-sprite) · [Folders](#folders) · [1. Make the art](#1-make-the-art) · [2. Describe it in Game Data](#2-describe-it-in-game-data) · [3. Get it into the game](#3-get-it-into-the-game) · [4. Player art](#4-player-art) · [5. Place objects in a level](#5-place-objects-in-a-level) · [6. Animated objects](#6-animated-objects) · [How sprites are scaled](#how-sprites-are-scaled) · [Changing existing art](#changing-existing-art) · [Placeholders and invisible objects](#placeholders-and-invisible-objects) · [Troubleshooting](#troubleshooting) · [Code reference](#code-reference) · [Tests](#tests) · [Committing](#committing)

## How to add a sprite

The short version. Each step links to the details below.

1. **Make a PNG** at 100%, with a transparent background, cropped tightly. Animation frames go side by side in one PNG. Save it in `game/resources/art/static/` (one frame) or `game/resources/art/animated/` (several frames). [Details](#1-make-the-art)
2. **Add a row to the Game Data sheet** (Assets tab): Animation ID `<asset>_<state>`, Asset ID, File Name, the size of one frame, and the number of frames. [All columns](#2-describe-it-in-game-data)
   ```
   school_desk_idle,school_desk,school_desk.png,32,32,1,,,,bottom-centre,,Middle,
   ```
3. **Export the CSV:** File → Download → Comma-separated values, rename it `game_data.csv`, and replace `game/resources/data/game_data.csv`. [Details](#3-get-it-into-the-game)
4. **Put it in the game:**
   - **The player:** Asset ID `player_character` with `player_idle`, `player_walk`, `player_jump`, `player_fall`. No code. [Details](#4-player-art)
   - **Anything in a level:** add one line to the level's `.objects` list in `game/src/levels/level_<n>/level_<n>.cpp`. The last argument is the Asset ID:
     ```cpp
     SolidPlatform("desk-1", {600, 586, 64, 64}, "school_desk"),   // the player can stand on it
     ArtObject("poster-1", {800, 400, 64, 96}, "poster"),          // art only, the player walks past
     ```
     The box is `{x, y, width, height}`: x/y is the top-left (y grows downward), and width/height is the PNG size × 2. [Details](#5-place-objects-in-a-level)
5. **Build, then run.** If it doesn't show, check the terminal for `Game Data:` warnings. [Troubleshooting](#troubleshooting)

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

Each level's code is in `game/src/levels/level_<number>/level_<number>.cpp`. Pick the way that fits the thing you're adding:

| The thing... | Use | Drawing code needed |
|---|---|---|
| sits there, collides or is a hazard (platform, furniture, trash can) | an entry in `.objects` with an `.assetId` | none |
| is moved or checked by the level's own script (Level 1's trash can and hydrant) | an `ArtObject` in `.objects`, moved onto the script's hitbox if it moves | none if it stays still; one `SetPosition` line if it moves |
| is pure decoration with no object (a poster, a sign) | a `SubmitSprites` override | a few lines |

Don't draw art with `DrawRectangle`/`DrawCircle` in `DrawWorld`: shapes there can't be swapped for sprites. `DrawWorld` is for text, road markings and temporary stand-ins.

### Objects: no drawing code

Add the object to the level's `.objects` list. Its Asset ID is the **Asset ID** column of the sheet, and it plays `<assetId>_<state>` (it starts in `idle`; with no matching state it uses the asset's first row). Every level uses the same shortcuts from `objects/game_object.h`, one line per object, with the box as `{x, y, width, height}`:

```cpp
SolidPlatform("step-1", {350, 550, 180, 24}),                  // stand on it; Asset ID "platform"
SolidPlatform("desk-1", {600, 586, 64, 64}, "school_desk"),   // stand on it, with its own art
SolidPlatform("wire-1", {2940, 800, 220, 16}, ""),            // collision only, no sprite
ArtObject("poster-1", {800, 400, 64, 96}, "poster"),          // art only, no collision
```

For anything else (hazards, triggers, moving or falling objects) write the full form:

```cpp
{.id = "spikes-1", .type = ObjectType::Hazard, .collision = CollisionType::Hazard,
 .position = {500, 600}, .size = {60, 50}, .assetId = "spikes"},
```

The box is in world pixels: x/y is its top-left, and a 48×32 PNG is 96×64 at 2×. With Pivot `bottom-centre` the sprite stands on the bottom-centre of the box. In the full form, `.collision` is `Solid` (stand on it), `Hazard` (hurts), `Trigger` or `None` (walk past it). The base `Level::SubmitSprites` draws every object, so nothing else is needed.

### Things the level script moves

Some levels handle things with their own code instead of the object system: Level 1 checks the trash can and hydrant hits itself, in `CheckHazards`. Give each one an `ArtObject` on the same rectangle as its hitbox:

```cpp
ArtObject("trash-can", trashCan, "trash_can"),   // in .objects: no collision, no placeholder
```

If the script also **moves** it, copy the script's rectangle onto the object every frame, at the end of `OnUpdate` and `OnReset`:

```cpp
if (auto *car = Objects().Get("car-1")) car->SetPosition({carBounds.x, carBounds.y});
```

Art-only objects use `.drawPlaceholder = false`, so until the row exists nothing is drawn and the old `DrawWorld` shape stays visible. Once the row exists the sprite draws on top of it; then delete that shape from `DrawWorld`.

### Decorations: a `SubmitSprites` override

Near the bottom of the level's file is its class (`Level1` and so on). Add a `SubmitSprites` function inside it, next to `OnReset` and `OnUpdate`, and declare it in the level's `.h` with `override`:

```cpp
class Level1 final : public Level {
    // ... existing code ...
private:
    // ... OnReset, OnUpdate ...

    // Called every frame while drawing. Queue this level's sprites here.
    void SubmitSprites(RenderQueue &queue, const Resources &resources) const override {
        Level::SubmitSprites(queue, resources);              // keep this: it draws the level's game objects
        const auto &assets = resources.Assets();
        if (const auto *desk = assets.Find("school_desk"))
            queue.Submit(assets, *desk, 0, SnapToPixels({600, 650}));   // frame 0, pivot at x 600, y 650
        if (const auto *sign = assets.Find("exit_sign"))
            queue.Submit(assets, *sign, 0, SnapToPixels({2400, 560}));
    }
};
```

- `assets.Find("school_desk")` must match the **Animation ID** exactly. If the ID isn't in the sheet it returns `nullptr` and the `if` skips it, so nothing breaks.
- The sprite goes on the **Layer** from the sheet automatically.
- **Always call `Level::SubmitSprites(queue, resources)` first.** The base version draws the level's game objects; skipping it makes them invisible.
- Wrap positions in `SnapToPixels(...)` so sprites stay crisp while the camera moves. It only changes where the sprite is drawn, not its gameplay position.
- Build and run to see it.

### Fitting a sprite into a rectangle

To make a sprite as large as possible inside a rectangle (a sign on a wall, an icon in a slot) use `SubmitFitted`:

```cpp
if (const auto *sign = assets.Find("exit_sign"))
    queue.SubmitFitted(assets, *sign, 0, {2380, 500, 60, 60}, Facing::None, Camera().zoom);
```

The sprite keeps its shape (it is never stretched), its size is rounded down so every art pixel stays a whole number of screen pixels, and its pivot is anchored to the same point of the rectangle: bottom-centre on the rectangle's bottom edge, top-left in its corner, centre in its middle. A missing PNG draws its placeholder in the same area. Pass `Camera().zoom` in levels; use `1` for drawing straight onto the 1600×900 canvas.

### Choosing coordinates

Positions are **world pixels**, the same units as the level's `.world`, `.spawn` and `.objects` at the top of the same file (Level 0 shown):

```cpp
.world = {0, 0, 1500, 700},                         // level is 1500 wide, 700 tall
.spawn = {100, 606},                                // where the player starts
.objects = {
    SolidPlatform("floor", {0, 650, 1500, 50}),     // the floor; its top is y = 650
    SolidPlatform("step-1", {350, 550, 180, 24}),   // a step from x 350 to 530, top at y = 550
```

- `x` grows to the right from 0. `y` grows **downward** from 0 at the top.
- For an object, `{x, y}` is the top-left of its box. To stand it on the floor, use `y = 650 − height`.
- In `SubmitSprites`, the position is where the **pivot** goes. For `bottom-centre`, use the top `y` of whatever it stands on: `{600, 650}` stands on Level 0's floor. For `top-left` it is the image's top-left corner; for `centre`, its middle.
- Each level has its own size: Level 1's floor is at y 1480, not 650.
- Expect some trial and error: change the numbers, build, look.

### Making it solid

`SolidPlatform` objects are solid. `ArtObject`s and `SubmitSprites` sprites are only pictures, so the player walks through them. Hazards and triggers use the full form with `CollisionType::Hazard` or `CollisionType::Trigger`.

## 6. Animated objects

**Objects animate on their own.** An object plays `<assetId>_<state>` with the row's FPS and Loops, starting in `idle`. To switch animations, set its state from the level's code; the new animation starts from its first frame:

```cpp
if (auto *anvil = Objects().Get("anvil-1")) anvil->SetState("fall");   // plays the anvil_fall row
```

The rest of this section is only for sprites a level draws itself in `SubmitSprites`. Those need their own `Animator` to track state and playback time, one per sprite, as a member of the level class:

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

## How sprites are scaled

Sprites are always scaled **uniformly** (the same amount across and down), so art is never stretched or squashed, and always with **nearest-neighbour** filtering, so pixels stay hard-edged instead of blurring.

| Setting | Where | Effect |
| --- | --- | --- |
| `pixelScale` (2) | `engine/assets/asset_manager.h` | Every Game Data sprite is drawn this many times its PNG size. Artists export at 100%. |
| Camera zoom (2) | `Level::Level` in `levels/level.cpp` | Levels are magnified on screen. With `pixelScale` 2, each art pixel is 4×4 canvas pixels. |
| Best fit (optional) | `queue.SubmitFitted` / `assets.DrawFitted` | Fits a sprite inside a rectangle without stretching, rounded to whole pixels (see above). |

Keep `pixelScale × camera zoom` a whole number (with zoom 2, any whole `pixelScale` works). Otherwise art pixels come out uneven, some columns wider than others.

Two more things keep pixel art clean, with nothing for you to do:

- **Pixel snapping.** The camera and the player are drawn at whole canvas pixels (`Level::SnapToPixels`), so edges don't flicker as the camera scrolls. Physics still uses exact positions. Use `SnapToPixels` for your own sprites too.
- **Window scaling.** The game draws on a fixed 1600×900 canvas, then copies it to the window (`ChoosePresentation` in `engine/graphics/layout.h`). At exactly 1×, 2× and so on it uses nearest-neighbour. At in-between sizes, such as a maximized window or a laptop at 125% display scaling, it enlarges with nearest-neighbour to the next whole multiple, then shrinks smoothly to fit, so pixels stay crisp. Below 1× it shrinks smoothly.

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
- **Level still draws a stand-in:** set `.drawPlaceholder = false` on the object. With no row, nothing is drawn for it; with a row, the sprite appears.
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
| Some art pixels are wider than others | `pixelScale × camera zoom` is not a whole number. See [How sprites are scaled](#how-sprites-are-scaled). |
| Level objects disappeared after adding `SubmitSprites` | The override doesn't call `Level::SubmitSprites(queue, resources)` first. |
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
queue.Submit(assets, *a, frame, SnapToPixels({x, y}), Facing::Right); // draw on its layer (inside SubmitSprites)
queue.SubmitFitted(assets, *a, frame, {x, y, w, h}, Facing::None, Camera().zoom); // best fit, no stretching
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
