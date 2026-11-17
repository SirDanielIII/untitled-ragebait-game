// AssetManager: loading PNGs named in Game Data, and drawing frames at their pivots.
// See asset_manager.h for how to use it.
#include "engine/assets/asset_manager.h"

#include <algorithm>
#include <vector>

namespace rage {
    namespace {
        // The places to look for a row's PNG, in order.
        // "desk.png" with 1 frame:   art/static/desk.png, then art/animated/desk.png.
        // "walk.png" with 6 frames:  art/animated/walk.png, then art/static/walk.png.
        // "tiles/street.png":        art/tiles/street.png only.
        std::vector<std::filesystem::path> Candidates(const std::filesystem::path &directory,
                                                      const AnimationData &animation) {
            std::string name = animation.fileName;
            std::replace(name.begin(), name.end(), '\\', '/'); // Accept Windows-style folders too.
            // A File Name with a folder is taken as relative to art/.
            if (name.find('/') != std::string::npos) return {directory / "art" / name};
            const auto preferred = animation.frames > 1 ? animatedArtFolder : staticArtFolder;
            const auto other = animation.frames > 1 ? staticArtFolder : animatedArtFolder;
            return {directory / preferred / name, directory / other / name};
        }
    }

    AssetManager::~AssetManager() { Unload(); }

    void AssetManager::Unload() {
        for (const auto &[name, texture]: textures) if (IsTextureValid(texture)) UnloadTexture(texture);
        textures.clear();
        data.Clear();
    }

    void AssetManager::Load(const std::filesystem::path &resourceDirectory) {
        Unload(); // Loading twice must not leak the first set of textures.
        data.LoadFile(resourceDirectory / gameDataPath);
        for (const auto &problem: data.Problems()) TraceLog(LOG_WARNING, "%s", problem.c_str());
        // Textures live on the graphics card, which only exists once a window is open.
        // Window-free tests still get the rows, just no textures.
        const bool canLoadTextures = IsWindowReady();
        int placeholders = 0;
        for (const auto *animation: data.All()) {
            if (animation->invisible) continue;
            if (animation->fileName.empty()) {
                ++placeholders;
                TraceLog(LOG_INFO, "Game Data: %s has no File Name yet; drawing a placeholder", animation->animationId.c_str());
                continue;
            }
            if (!canLoadTextures) continue;
            // Each file is loaded once, even when several rows share it (e.g. a tile set).
            if (!textures.contains(animation->fileName)) {
                Texture2D texture = {};
                for (const auto &path: Candidates(resourceDirectory, *animation)) {
                    if (!FileExists(path.string().c_str())) continue;
                    texture = LoadTexture(path.string().c_str());
                    break;
                }
                if (!IsTextureValid(texture)) {
                    ++placeholders;
                    TraceLog(LOG_WARNING, "Game Data: %s for %s was not found in resources/%s or resources/%s; drawing a placeholder",
                             animation->fileName.c_str(), animation->animationId.c_str(), animatedArtFolder, staticArtFolder);
                    continue;
                }
                textures.emplace(animation->fileName, texture);
            }
            // Catch PNGs that are smaller than the sheet says, e.g. a strip exported with
            // fewer frames than the Frames column. Drawing still works; frames past the
            // edge come out empty, so warn about it.
            const Texture2D &texture = textures.at(animation->fileName);
            const int neededWidth = animation->Columns() * animation->frameWidth;
            const int neededHeight = animation->Rows() * animation->frameHeight;
            if (texture.width < neededWidth || texture.height < neededHeight) {
                TraceLog(LOG_WARNING, "Game Data: %s is %dx%d but %s needs at least %dx%d (%d frames of %dx%d)",
                         animation->fileName.c_str(), texture.width, texture.height, animation->animationId.c_str(),
                         neededWidth, neededHeight, animation->frames, animation->frameWidth, animation->frameHeight);
            }
        }
        TraceLog(LOG_INFO, "Game Data: %d animations, %d textures, %d placeholders",
                 static_cast<int>(data.Size()), static_cast<int>(textures.size()), placeholders);
    }

    const Texture2D *AssetManager::TextureFor(const AnimationData &animation) const {
        if (animation.invisible || animation.fileName.empty()) return nullptr;
        const auto found = textures.find(animation.fileName);
        return found != textures.end() && IsTextureValid(found->second) ? &found->second : nullptr;
    }

    bool AssetManager::HasTexture(const AnimationData &animation) const { return TextureFor(animation) != nullptr; }

    // Moves the frame so that its pivot point, not its corner, lands on `position`.
    Rectangle AssetManager::Bounds(const AnimationData &animation, Vector2 position, float scale) {
        const float width = animation.frameWidth * scale;
        const float height = animation.frameHeight * scale;
        float pivotX = 0;
        float pivotY = 0;
        PivotOffset(animation.pivot, width, height, pivotX, pivotY);
        return {position.x - pivotX, position.y - pivotY, width, height};
    }

    DrawResult AssetManager::Draw(const AnimationData &animation, int frame, Vector2 position, Facing facing,
                                  const PlaceholderDrawer &placeholder, float scale, Color tint) const {
        if (animation.invisible) return DrawResult::Invisible;
        // Where on screen (or in the world) the frame goes, and whether to mirror it.
        const Rectangle bounds = Bounds(animation, position, scale);
        const bool flipped = ShouldFlip(animation.faces, facing);
        const Texture2D *texture = TextureFor(animation);
        if (!texture) {
            if (placeholder) placeholder(bounds, flipped);
            else DrawPlaceholderBox(bounds, flipped, animation.faces != Facing::None);
            return DrawResult::Placeholder;
        }
        // Cut this frame out of the strip or grid and stretch it over `bounds`.
        const FrameRect frameRect = FrameSource(animation, frame);
        Rectangle source{frameRect.x, frameRect.y, frameRect.width, frameRect.height};
        if (flipped) source.width = -source.width; // Raylib mirrors a negative source width.
        DrawTexturePro(*texture, source, bounds, {0, 0}, 0, tint);
        return DrawResult::Texture;
    }

    void AssetManager::DrawPlaceholderBox(Rectangle bounds, bool flipped, bool directional) {
        const Color fill{214, 64, 196, 70};
        const Color line{214, 64, 196, 220};
        DrawRectangleRec(bounds, fill);
        DrawRectangleLinesEx(bounds, 2, line);
        DrawLineEx({bounds.x, bounds.y}, {bounds.x + bounds.width, bounds.y + bounds.height}, 1, line);
        DrawLineEx({bounds.x + bounds.width, bounds.y}, {bounds.x, bounds.y + bounds.height}, 1, line);
        if (!directional) return;
        // A notch on the side the object faces, so flipping is visible before art exists.
        const float notch = std::min(bounds.width, bounds.height) * 0.2f;
        const float x = flipped ? bounds.x : bounds.x + bounds.width - notch;
        DrawRectangleRec({x, bounds.y + bounds.height / 2 - notch / 2, notch, notch}, line);
    }
}
