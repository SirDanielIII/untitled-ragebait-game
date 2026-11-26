#pragma once

namespace rage {
    // Fake countdown timer, distracts player, creates a false sense of urgency 

    class FakeTimer {
        public:
            void Reset(float startSeconds = 60.0f);

            void Update(float dt, bool paused = false);

            void ForceGlitch();
            
            void Draw(int x, int y, int fontSize = 40) const;

            void DebugSetDriftSpeed(float pixelsPerSecond) { driftSpeed = pixelsPerSecond; 
            }
        
        private:
            enum class Glitch {None, SpeedUp, SlowDown, Freeze, WrongTime};

            static float RandFloat(float lo, float hi);
            static const char* formatTime(float seconds);
            void StartRandomGlitch();


            float displayTime = 60.0f;

            // Glitch
            Glitch glitch = Glitch::None;
            float glitchTime = 0.0f;
            float nextGlitch = 4.0f;
            float fakeValue = 60.0f;
            float fakeTimer = 0.0f;

            // Drift
            float driftDirX = 0.0f;
            float driftDirY = 1.0f;
            float driftDistance = 0.0f;
            float driftSpeed = 5.0f;
            float wobbleClock = 0.0f;
            float flashClock = 0.0f;
    };
}