#pragma once

namespace rage {
    class Resources;

    // Fake countdown timer: creates urgency, then glitches while docked in the HUD.
    class FakeTimer {
    public:
        void Reset(float startSeconds = 60.0f, bool showEntrance = true);
        void Update(float dt, bool paused = false);
        void ForceGlitch();
        void Draw(const Resources &resources) const;

    private:
        enum class Glitch { None, SpeedUp, SlowDown, Freeze, WrongTime };

        static float RandFloat(float lo, float hi);
        static const char *FormatTime(float seconds);
        void StartRandomGlitch();

        float displayTime = 60.0f;
        Glitch glitch = Glitch::None;
        float glitchTime = 0.0f;
        float nextGlitch = 4.0f;
        float fakeValue = 60.0f;
        float fakeTimer = 0.0f;

        float elapsedSeconds = 0.0f;
        float entranceSeconds = 0.0f;
        float driftDirX = 0.0f;
        float driftDirY = 1.0f;
        float driftDistance = 0.0f;
    };
}
