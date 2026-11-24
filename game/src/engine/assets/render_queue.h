// RenderQueue: draws things in layer order, whatever order the code asks for them.
//
// Instead of drawing straight away, code "submits" draw jobs tagged with a layer.
// Flush() then runs every Background job, then Middle, then Foreground, then UI.
// That way a level can submit a foreground bush before the player and the bush still
// ends up in front.
//
// Level::Draw makes one queue per frame. Levels add their sprites in SubmitSprites():
//   queue.Submit(assets, *desk, 0, {600, 650});   // a Game Data sprite, on its sheet layer
//   queue.Submit(Layer::Foreground, [] { DrawRectangle(0, 0, 50, 50, RED); }); // any drawing
//
// Jobs run later, at Flush(). A lambda must not capture by reference ([&]) a local
// variable that is gone by then. Capture such values by copy instead.
#pragma once

#include "engine/assets/asset_manager.h"

#include <array>
#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

namespace rage {
    class RenderQueue {
    public:
        // Adds any drawing code to a layer. Jobs in the same layer run in the order added.
        void Submit(Layer layer, std::function<void()> draw) {
            const auto index = static_cast<std::size_t>(layer);
            if (index < layers.size() && draw) layers[index].push_back(std::move(draw));
        }

        // Adds a Game Data sprite on the layer from its sheet row. Arguments are the same as
        // AssetManager::Draw. `assets` and `animation` are kept by reference, which is safe
        // when they come from resources.Assets(): those live for the whole game.
        void Submit(const AssetManager &assets, const AnimationData &animation, int frame, Vector2 position,
                    Facing facing = Facing::None, PlaceholderDrawer placeholder = {}) {
            Submit(animation.layer, [&assets, &animation, frame, position, facing, placeholder = std::move(placeholder)] {
                assets.Draw(animation, frame, position, facing, placeholder);
            });
        }

        // Like Submit, but best-fits the sprite inside `target` without stretching (see
        // AssetManager::DrawFitted). In a level, pass Camera().zoom as `pixelsPerUnit`.
        void SubmitFitted(const AssetManager &assets, const AnimationData &animation, int frame, Rectangle target,
                          Facing facing = Facing::None, float pixelsPerUnit = 1, PlaceholderDrawer placeholder = {}) {
            Submit(animation.layer, [&assets, &animation, frame, target, facing, pixelsPerUnit,
                                     placeholder = std::move(placeholder)] {
                assets.DrawFitted(animation, frame, target, facing, pixelsPerUnit, placeholder);
            });
        }

        // Runs every job, Background first and UI last, then empties the queue.
        void Flush() {
            for (auto &layer: layers) {
                for (const auto &draw: layer) draw();
                layer.clear();
            }
        }

        // Number of jobs waiting to be drawn.
        std::size_t Size() const {
            std::size_t total = 0;
            for (const auto &layer: layers) total += layer.size();
            return total;
        }

    private:
        // One list of jobs per layer, indexed by the Layer enum (Background = 0 ... Ui = 3).
        std::array<std::vector<std::function<void()>>, static_cast<std::size_t>(Layer::Count)> layers;
    };
}
