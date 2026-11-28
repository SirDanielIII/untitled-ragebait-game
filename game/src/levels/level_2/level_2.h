#pragma once

#include "levels/level.h"
#include "objects/bird.h"
#include <array>

namespace rage {

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