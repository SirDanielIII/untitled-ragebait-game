#pragma once
#include <array>
#include "levels/level.h"

namespace rage {
    class Level3 final : public Level {
    public:
        Level3();

    private:
        std::vector<Event> CreateEvents();
        void OnReset() override;
        void OnUpdate(float dt, const InputFrame &input) override;
        void DrawWorld(const Resources &resources) const override;
        void SubmitSprites(RenderQueue &queue, const Resources &resources) const override;
        float BookShake(const std::string &id) const;
        void UpdateBooks(float dt);
        void ResetBooks();
        void UpdateFallingObjects(float dt);
        void ResetFallingObjects();

        bool AnvilWarningShowing() const;
        bool AnyBookCounting() const;
        bool AnyBookFallen() const;
        bool StandingOnChair() const;

        Vector2 entityPosition = {};
        std::array<float, 3> bookTimers = {};
        float fallTimer = 0.0f;
        int fallIndex = 0;
        std::array<float, 3> fallAge = {-1.0f, -1.0f, -1.0f};
        int deaths = 0; 
    };
}
