#pragma once

#include "levels/level.h"
#include "objects/bird.h"
#include <array>

namespace rage {

    // Asset IDs for Level 2's things, one organized list: level_2_sprites.cpp.
    std::vector<SpriteAssignment> Level2Sprites();

    class Level2 final : public Level {
    public:
        Level2();

    private:
        void OnReset() override;
        void OnUpdate(float dt, const InputFrame& input) override;
        void SubmitSprites(RenderQueue& queue, const Resources& resources) const override;

        std::vector<Event> CreateEvents();

        std::array<Bird, 10> birds;
    };

}