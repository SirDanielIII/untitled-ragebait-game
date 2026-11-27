#include "levels/level_1/fake_timer.h"
#include "levels/level_1/level_1.h"
#include <array>
#include <cstdint>
#include "engine/graphics/layout.h"
#include <algorithm>
#include <cmath>


namespace rage {
    float FakeTimer::RandFloat(float lo, float hi) {
        return lo + (hi - lo) * (static_cast<float>(GetRandomValue(0, 10000))/10000.0f);
    }

    const char* FakeTimer::formatTime(float seconds) {
        const bool negative = seconds < 0.0f;
        const int total= static_cast<int>(std::fabs(seconds));
        return TextFormat("%s%02d:%02d", negative ? "-" : "", total / 60, total%60);
    }


    void FakeTimer::Reset(float startSeconds) {
        displayTime = startSeconds;
        fakeValue = startSeconds;
        fakeTimer = 0.0f;
        glitch = Glitch::None;
        glitchTime = 0.0f;
        nextGlitch = RandFloat(3.0f, 6.0f);
        wobbleClock = 0.0f;
        flashClock = 0.0f;
        driftDistance = 0.0f;


        // random drift direction
        const float angle = RandFloat(0.0f, 2.0f * PI);
        driftDirX = std::cos(angle);
        driftDirY = std::sin(angle);
    }

    void FakeTimer::StartRandomGlitch() {
        switch(GetRandomValue(0,4)) {
            case 0:
                glitch = Glitch::SpeedUp;
                glitchTime = RandFloat(2.0f, 4.0f);
                break;
            case 1:
                glitch = Glitch::SpeedUp;
                glitchTime = RandFloat(2.0f, 4.0f);
                break;
            case 2:
                glitch = Glitch::SlowDown;
                glitchTime = RandFloat(2.0f, 4.0f);
                break;
            case 3:
                glitch = Glitch::Freeze;
                glitchTime = RandFloat(2.0f, 4.0f);
                break;
            case 4:
                glitch = Glitch::WrongTime;
                glitchTime = RandFloat(2.0f, 4.0f);
                break;
            
            default:
                // just jump forwards or backwards, does not go back to previous time
                displayTime += RandFloat(-10.0f, 10.0f);
                nextGlitch = RandFloat(2.0f, 6.0f);
                break;
        }
    }
    void FakeTimer::ForceGlitch() {
        glitch = Glitch::None;
        StartRandomGlitch();
    }

    void FakeTimer::Update(float dt, bool paused) {
        if (paused) 
            return;

        wobbleClock += dt;
        flashClock += dt;


        if (glitch != Glitch::None) {
            glitchTime -= dt;
            if (glitchTime <= 0.0f) {
                glitch = Glitch::None;
                nextGlitch = RandFloat(2.0f, 6.0f);
            }
        } else {
            nextGlitch -= dt;
            if (nextGlitch <=0.0f) 
                StartRandomGlitch();
        }

        float scale = 1.0f;
        switch(glitch) {
            case Glitch::SpeedUp: scale = 2.5f;
                break;
            case Glitch::SlowDown: scale = 0.25f;
                break;
            case Glitch::Freeze: scale = 0.0f;
                break;
            default: break;
        }
        displayTime -= dt * scale;


        if (glitch == Glitch::WrongTime) {
            fakeTimer -=dt;
            if (fakeTimer <= 0.0f) {
                fakeValue = displayTime + RandFloat(-30.0f, 30.0f);
                fakeTimer = 0.15f;
            }
        }

        driftDistance += dt * driftSpeed;
    }

        void FakeTimer::Draw(int x, int y, int fontSize) const {
        const float shown = (glitch == Glitch::WrongTime) ? fakeValue : displayTime;
        const char* text = formatTime(shown);
        const int textW = MeasureText(text, fontSize);

        // (x, y) is the top-centre anchor of the text.
        float posX = static_cast<float>(x) - static_cast<float>(textW) / 2.0f;
        float posY = static_cast<float>(y);

        // Drift + a little wobble.
        posX += driftDirX * driftDistance + std::sin(wobbleClock * 1.3f) * 6.0f;
        posY += driftDirY * driftDistance + std::cos(wobbleClock * 0.9f) * 6.0f;

        // Keep it on screen.
        const float margin = 10.0f;
        const float maxX = std::max(margin, static_cast<float>(canvasWidth - textW) - margin);
        const float maxY = std::max(margin, static_cast<float>(canvasHeight - fontSize) - margin);
        posX = std::clamp(posX, margin, maxX);
        posY = std::clamp(posY, margin, maxY);

        // Flash red under 10 seconds.
        Color color = WHITE;
        if (shown < 10.0f && std::fmod(flashClock, 0.5f) < 0.25f)
            color = RED;

        DrawText(text, static_cast<int>(posX), static_cast<int>(posY), fontSize, color);
    }
}