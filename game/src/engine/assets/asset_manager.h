// AssetManager: loads the PNGs listed in Game Data and draws their frames.
//
// There is one AssetManager for the whole game. Resources owns it, so anything that has
// `resources` can reach it with resources.Assets(). It is loaded once at startup and
// cleaned up automatically when the game closes.
//
// Typical use:
//   const auto &assets = resources.Assets();
//   if (const auto *walk = assets.Find("player_walk"))
//       assets.Draw(*walk, animator.Frame(*walk), feet, Facing::Left);
//
// It only draws. Gameplay code keeps its own positions and collision rectangles, so a
// missing or resized PNG never changes how the game plays.
#pragma once

#include "engine/assets/game_data.h"
#include "raylib.h"

#include <cstddef>
#include <filesystem>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace rage {
    // Where things live, relative to the game/resources/ folder.
    inline constexpr const char *gameDataPath = "data/game_data.csv";
    inline constexpr const char *staticArtFolder = "art/static";     // One-frame PNGs.
    inline constexpr const char *animatedArtFolder = "art/animated"; // Strips and grids.

    // Every Game Data sprite is drawn this many times its png size.
    // Artists export at 100%; a 16x32 png is drawn 32x64 in the world.
    inline constexpr float pixelScale = 2.0f;

    // What Draw() actually did. Mostly useful for tests.
    enum class DrawResult { Texture, Placeholder, Invisible };

    // A function that draws a stand-in when a PNG does not exist yet. It receives the
    // rectangle the art would cover, and whether the art would have been mirrored.
    // Example: the player passes one that draws the old student drawing in that rectangle.
    using PlaceholderDrawer = std::function<void(Rectangle bounds, bool flipped)>;

    class AssetManager {
    public:
        AssetManager() = default;

        ~AssetManager(); // Unloads every texture.

        // Copying is disabled: two copies would both try to unload the same textures.
        AssetManager(const AssetManager &) = delete;

        AssetManager &operator=(const AssetManager &) = delete;

        // Reads data/game_data.csv, then loads each PNG it names (each file only once).
        // Problems, such as bad values or missing files, are written to the console and
        // never stop the game. PNGs are only loaded when a raylib window exists.
        void Load(const std::filesystem::path &resourceDirectory);

        // Unloads all textures and forgets all rows.
        void Unload();

        const GameData &Data() const { return data; }

        // Look up an animation by its Animation ID, e.g. Find("player_walk").
        // Returns nullptr when the ID is not in Game Data, so check before using it.
        const AnimationData *Find(std::string_view animationId) const { return data.Find(animationId); }

        // Look up by object and state, e.g. Find("player_character", "walk").
        const AnimationData *Find(std::string_view assetId, std::string_view state) const {
            return data.Find(assetId, state);
        }

        // False when the PNG is missing, not loaded yet, or the File Name is blank or "none".
        bool HasTexture(const AnimationData &animation) const;

        std::size_t TextureCount() const { return textures.size(); }

        // The rectangle a frame covers when its pivot is placed at `position`.
        // Example: a 48x80 bottom-centre frame at (100, 650) covers x 52-148, y 490-650.
        static Rectangle Bounds(const AnimationData &animation, Vector2 position, float scale = pixelScale);

        // Draws frame `frame` with its pivot at `position`, in world or canvas coordinates.
        //   facing:      which way the object faces now. The image is mirrored when this is
        //                the opposite of the row's Faces value.
        //   placeholder: what to draw if the PNG is missing. Leave empty for a plain box.
        //   scale, tint: optional size multiplier (default pixelscale) and colour tint (WHITE = unchanged).
        DrawResult Draw(const AnimationData &animation, int frame, Vector2 position, Facing facing = Facing::None,
                        const PlaceholderDrawer &placeholder = {}, float scale = pixelScale, Color tint = WHITE) const;

        // The default stand-in for missing art: a translucent box with a cross.
        // `directional` adds a notch on the side the object faces.
        static void DrawPlaceholderBox(Rectangle bounds, bool flipped, bool directional = false);

    private:
        // The loaded texture for a row, or nullptr if there is none.
        const Texture2D *TextureFor(const AnimationData &animation) const;

        GameData data;
        // Loaded textures by File Name. Rows that share a PNG share one texture.
        std::unordered_map<std::string, Texture2D> textures;
    };
}
