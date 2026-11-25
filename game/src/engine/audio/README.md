# Audio system (Audio Data)

Every sound effect and music track is described in the **Audio** tab of the **CISC 320 Game Data** workbook, not in C++. When the game starts, it reads an exported copy of that tab, loads each sound effect once, and streams music when it plays. Anything missing is silent and logged, never a crash.

- **Adding or changing audio** (a new file, volume, looping, variants) needs **no code**.
- **Triggering a new sound** from gameplay needs one line in a level. Player sounds and music are already hooked up.

Contents: [Folders](#folders) · [1. Make the audio](#1-make-the-audio) · [2. Describe it in Audio Data](#2-describe-it-in-audio-data) · [3. Get it into the game](#3-get-it-into-the-game) · [4. What plays automatically](#4-what-plays-automatically) · [5. Playing sounds from a level](#5-playing-sounds-from-a-level) · [Volume and pause](#volume-and-pause) · [Troubleshooting](#troubleshooting) · [Code reference](#code-reference) · [Tests](#tests)

## Folders

| Path | What goes there |
| --- | --- |
| `game/resources/data/audio_data.csv` | The Audio tab, exported as CSV |
| `game/resources/audio/sfx/` | Sound effects, UI and voice: **WAV** |
| `game/resources/audio/music/` | Music: **OGG** (streamed, so files stay small) |
| `game/src/engine/audio/` | The code (this folder) |

The game reads a copy of `game/resources/` placed next to the executable at build time. **After changing anything in `game/resources/`, click Build before running.**

## 1. Make the audio

- **Sound effects:** export as **WAV** (16-bit, 44.1 or 48 kHz). WAV plays instantly with no decoding delay. Trim silence at the start, or the sound will feel late.
- **Music:** export as **OGG**. Make loops seamless: the end should flow straight into the start.
- **Variants:** for sounds that repeat a lot (footsteps, jumps), record a few takes and number them: `jump_1.wav`, `jump_2.wav`, `jump_3.wav`. The game picks one at random each time, so it doesn't sound robotic.
- File names are lowercase with underscores.

## 2. Describe it in Audio Data

Add one row per sound to the **Audio** tab:

| Column | What to enter | If left blank |
| --- | --- | --- |
| **Sound ID** | What code asks for, lowercase with underscores (`player_jump`). Must be unique. | Row ignored |
| **File Name** | The file's name (`jump.wav`). With variants, the name *without* the number. | Silent until the file exists |
| **Category** | `SFX`, `UI`, `Music` or `Voice`. Music uses the Music slider; the rest use the Sound Effects slider. | SFX |
| **Volume** | 0 to 1, for balancing sounds against each other | 1 |
| **Loops** | `Yes` to repeat, `No` to play once | Music: Yes. Others: No |
| **Variants** | How many numbered files there are (`3` → `jump_1.wav` … `jump_3.wav`). Not for music. | 1 |
| **Notes** | Anything. The game ignores it. | |

Example rows:

```
Sound ID,File Name,Category,Volume,Loops,Variants,Notes
ui_click,click.wav,UI,1,,,Menu button click
player_jump,jump.wav,SFX,0.8,,3,jump_1.wav to jump_3.wav
level_1_theme,level_1.ogg,Music,0.7,Yes,,Streets theme
level1_slide,,SFX,1,,,Planned: no file yet
```

A row with no File Name is a placeholder: it plays nothing until the file is added, and code can already request it.

## 3. Get it into the game

1. In Google Sheets, on the **Audio** tab: **File → Download → Comma-separated values (.csv)**. Only the tab you are on is exported.
2. Rename the download to `audio_data.csv` and replace `game/resources/data/audio_data.csv`.
3. Put WAVs in `audio/sfx/` and OGGs in `audio/music/`.
4. **Build**, run, and read the console. Problems are listed with their sheet row, for example
   `audio_data.csv: row 4 (player_jump): Volume "loud" is not a number; using 1`.
   A healthy start includes a line like `Audio Data: 10 sounds, 6 files loaded, 4 silent`.

The Google Sheet is the master copy. For real changes, edit the sheet and export again instead of editing the CSV by hand.

## 4. What plays automatically

These Sound IDs are already wired up. Add their files and they just work:

| Sound ID | When |
| --- | --- |
| `ui_click` | A menu button is pressed |
| `player_jump` | The player leaves the ground moving up (not when walking off a ledge) |
| `player_land` | The player lands |
| `player_hurt` | The player loses health but survives |
| `player_death` | The player dies, or touches a hazard and respawns |
| `menu_theme` | Music on the main menu, level select and settings |
| `level_0_theme` … `level_3_theme` | Music in each level (keeps playing, quieter, while paused) |

Respawning or being moved by a script (checkpoints, Level 1's slide) never counts as a jump or landing.

## 5. Playing sounds from a level

Gameplay code never calls the audio system directly. A level **requests** a sound and the game plays it at the end of the frame. That keeps levels testable without speakers.

**A scripted moment** (in an event, `OnUpdate` or `OnTrigger`):

```cpp
RequestSound("level1_slide");
```

For example, inside the event that reacts to `"level1.slide_started"`. Unknown IDs or IDs with no file yet are silent and logged once.

**Different music for a level:** override `MusicTrack` in the level's class. Return `""` for silence:

```cpp
std::string MusicTrack() const override {
    return campusReached ? "campus_theme" : "level_1_theme";
}
```

The game checks this every frame and **crossfades** whenever the answer changes, so music can follow the story.

## Volume and pause

- The final volume is **Master × (Music or Sound Effects slider) × the row's Volume**. Mute silences everything; the volumes are kept.
- The sliders are in **Settings → Audio** and saved in preferences (`audio.music_volume`, `audio.sfx_volume`).
- **Pausing** stops sound effects that are playing and lowers the music to 35%. Menu clicks still play.
- Each sound effect can overlap itself up to 4 times, so rapid repeats don't cut each other off.

## Troubleshooting

| What you see | Likely cause |
| --- | --- |
| Nothing changed after editing audio or the CSV | You didn't build. The game reads the copy made at build time. |
| `... was not found in resources/audio/sfx` | The file name or folder is wrong. With variants, the files must be numbered (`jump_1.wav`). |
| `no sound "x" in data/audio_data.csv` | The ID in `RequestSound("x")` doesn't match the sheet. |
| `... has no File Name yet` | A planned row. Add the file name when the audio exists. |
| `... is not in the Music category` | A music track's Category isn't `Music`. |
| No sound at all | Check Settings → Audio (enabled, sliders up). `Audio: no audio device` in the console means the computer has no output. |
| Music cuts out | It isn't OGG/WAV, or the file is damaged. Check the console. |

## Code reference

| File | What it does |
| --- | --- |
| `audio_data.h/.cpp` | Reads and checks the CSV. `SoundData` is one row; `AudioData` holds all rows. No raylib, so it is testable without speakers. |
| `audio_manager.h/.cpp` | `AudioManager`: loads each file once (with extra voices), plays by ID, streams and crossfades music, applies volumes and pause. |
| `../data/csv.h/.cpp` | CSV parsing and validation helpers shared with Game Data. |

How it fits together:

```
Level::Update ── RequestSound / player state ──► soundRequests
GameWindow::Frame:  audio.SetVolumes(...) → audio.SetPaused(game.IsLevelPaused())
                    → audio.PlayMusic(game.MusicTrack()) → audio.Play(each request) → audio.Update(dt)
```

`Resources` owns the one `AudioManager` (`resources.Audio()`) and unloads it before the audio device closes.

## Tests

The tests are in `game/tests/audio_tests.cpp`. `--self-test` checks CSV import and validation, variants, volumes, settings, and the level's sound requests (jump, land, teleport, scripted, music IDs) without speakers. `--smoke-test` plays generated tones through the real audio device: effects, voices, variants, music, pause and missing files. It is skipped cleanly on a computer with no audio device.
