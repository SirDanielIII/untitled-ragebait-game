#pragma once

#include "levels/level.h"
#include "levels/level_1/fake_timer.h"
#include "objects/bird.h"

#include <array>
#include <cstdint>
#include <vector>

namespace rage {
    class Level1 final : public Level {
    public:
        Level1();

    private:
        struct Vehicle {
            Rectangle bounds;
            float speed;
            Color color;
        };

        struct FallingStar {
            float x;
            float triggerX;
            float y = 0;
            float warningSeconds = 0;
            bool warned = false;
            bool falling = false;
            bool finished = false;
        };

        struct FlyingPig {
            Rectangle bounds;
            int route;
            float waveAmplitude;
            float waveRate;
            float wavePhase;
            float speed = 165;
            float steerSeconds = 0;
            float fartSeconds = 0;
            std::uint64_t id = 0;
        };

        struct GasPuff {
            Vector2 center;
            float age = 0;
            std::uint64_t id = 0;
        };

        void OnReset() override;
        void OnUpdate(float dt, const InputFrame& input) override;
        void DrawPileup() const;
        void DrawWorld(const Resources& resources) const override;
        void SubmitSprites(RenderQueue& queue, const Resources& resources) const override;
        void DrawHud(const Resources& resources) const override;
        std::vector<Event> CreateEvents();
        void Respawn(const char* notification);
        void ResetTraffic();
        void CheckHazards(float dt);
        void ResetStars();
        void UpdatePigs(float dt);
        void UpdateCampus();

        std::array<Vehicle, 5> traffic = {}; // The last car waits in the tunnel between speeding runs.
        float speederDelay = 0.0f;
        std::vector<FlyingPig> pigs;
        std::vector<GasPuff> gasPuffs;
        std::array<Bird, 3> birds;
        std::uint64_t nextHazardId = 1;
        float pigSpawnSeconds = 0;
        float birdCooldown = 0;
        bool crashImpact = false;
        std::array<FallingStar, 9> stars = {};
        FakeTimer timer;

        Vector2 checkpoint = {};
        float collisionCooldown = 0.0f;
        float pileupSeconds = 0.0f;
        float slideProgress = 0.0f;
        float chickenX = 0.0f;
        float anvilLandedAt = 0.0f;
        int collisionCount = 0;
        bool roadCheckpointReached = false;
        bool skyCheckpointReached = false;
        bool pileupStarted = false;
        bool pileupSettled = false;
        bool sliding = false;
        bool slideFinished = false;
        bool campusReached = false;
    };
}
