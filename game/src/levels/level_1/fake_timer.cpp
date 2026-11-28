#include "levels/level_1/fake_timer.h"
#include "engine/graphics/layout.h"
#include "engine/resources/resources.h"
#include "ui/hud/gameplay_hud.h"
#include "ui/theme.h"
#include <algorithm>
#include <cmath>


namespace rage {
    namespace {
        constexpr float entranceHold = 1.2f;
        constexpr float entranceTravel = 0.65f;
        constexpr float entranceDuration = entranceHold + entranceTravel;
        constexpr float driftSpeed = 5.0f;
    }

    float FakeTimer::RandFloat(float lo, float hi) {
        return lo + (hi - lo) * (static_cast<float>(GetRandomValue(0, 10000))/10000.0f);
    }

    const char* FakeTimer::FormatTime(float seconds) {
        const bool negative = seconds < 0.0f;
        const int total = static_cast<int>(std::fabs(seconds));
        return TextFormat("%s%02d:%02d", negative ? "-" : "", total / 60, total % 60);
    }


    void FakeTimer::Reset(float startSeconds, bool showEntrance) {
        displayTime = startSeconds;
        fakeValue = startSeconds;
        fakeTimer = 0.0f;
        glitch = Glitch::None;
        glitchTime = 0.0f;
        nextGlitch = RandFloat(3.0f, 6.0f);
        elapsedSeconds = 0.0f;
        entranceSeconds = showEntrance ? 0.0f : entranceDuration;
        driftDistance = 0.0f;


        // random drift direction
        const float angle = RandFloat(0.0f, 2.0f * PI);
        driftDirX = std::cos(angle);
        driftDirY = std::sin(angle);
    }

    void FakeTimer::StartRandomGlitch() {
        switch (GetRandomValue(0, 4)) {
            case 0:
            case 1:
                glitch = Glitch::SpeedUp;
                break;
            case 2:
                glitch = Glitch::SlowDown;
                break;
            case 3:
                glitch = Glitch::Freeze;
                break;
            case 4:
                glitch = Glitch::WrongTime;
                break;
        }
        glitchTime = RandFloat(2.0f, 4.0f);
    }
    void FakeTimer::ForceGlitch() {
        glitch = Glitch::None;
        StartRandomGlitch();
    }

    void FakeTimer::Update(float dt, bool paused) {
        if (paused || !std::isfinite(dt) || dt < 0)
            return;

        elapsedSeconds += dt;
        const float previousEntrance = entranceSeconds;
        entranceSeconds = std::min(entranceDuration, entranceSeconds + dt);


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

        // Only start drifting once it has reached its corner.
        driftDistance += std::max(0.0f, dt - (entranceSeconds - previousEntrance)) * driftSpeed;
    }

    void FakeTimer::Draw(const Resources &resources) const {
        const float shown = (glitch == Glitch::WrongTime) ? fakeValue : displayTime;
        const char* text = FormatTime(shown);
        const float travel = std::clamp((entranceSeconds - entranceHold) / entranceTravel, 0.0f, 1.0f);
        const float eased = travel * travel * (3 - 2 * travel);
        const bool docked = entranceSeconds >= entranceDuration;
        // Two short flashes on the timer itself, with no full-screen strobe.
        const bool entranceFlash = entranceSeconds < 0.14f
            || (entranceSeconds >= 0.5f && entranceSeconds < 0.64f);
        const bool urgentFlash = shown < 10.0f && std::fmod(elapsedSeconds, 0.5f) < 0.25f;
        const Color accent = entranceFlash ? theme::paper : urgentFlash ? theme::danger : theme::gold;
        const Color fill = entranceFlash ? theme::danger : theme::panel;

        // Quantize the moving overlay to a two-pixel grid.
        const auto snap = [](float value) { return std::round(value / 2) * 2; };
        Rectangle panel{0, 0, snap(std::lerp(440.0f, 248.0f, eased)), snap(std::lerp(168.0f, 100.0f, eased))};
        panel.x = snap(std::lerp(canvasWidth / 2.0f, canvasWidth - 160.0f, eased) - panel.width / 2);
        panel.y = snap(std::lerp(canvasHeight / 2.0f, 98.0f, eased) - panel.height / 2);
        if (docked) {
            // Keep the original drift and wobble, bounded near the top-right anchor.
            const float wobble = 6.0f * std::min(driftDistance / 6, 1.0f);
            panel.x += snap(std::clamp(driftDirX * driftDistance, -24.0f, 12.0f)
                + std::sin(elapsedSeconds * 1.3f) * wobble);
            panel.y += snap(std::clamp(driftDirY * driftDistance, 0.0f, 24.0f)
                + std::cos(elapsedSeconds * 0.9f) * wobble);
        }
        DrawHudPanel(panel, accent, fill);
        const char *label = entranceSeconds < entranceHold ? "LATE FOR CLASS!" : "TIME LEFT";
        const float labelSize = 20;
        const auto labelWidth = resources.MeasureText(label, labelSize, 1, FontRole::Heading).x;
        resources.DrawText(label, {snap(panel.x + (panel.width - labelWidth) / 2), panel.y + 14}, labelSize, 1, accent, FontRole::Heading);

        float fontSize = std::round(std::lerp(80.0f, 40.0f, eased) / 4) * 4;
        // Negative minutes and long runs still fit inside the corner panel.
        while (fontSize > 12 && resources.MeasureText(text, fontSize).x > panel.width - 24) fontSize -= 4;
        const auto textSize = resources.MeasureText(text, fontSize);
        const Vector2 position{snap(panel.x + (panel.width - textSize.x) / 2),
                               snap(panel.y + 40 + (panel.height - 48 - fontSize) / 2)};
        resources.DrawText(text, {position.x + 4, position.y + 4}, fontSize, 1, theme::background);
        resources.DrawText(text, position, fontSize, 1, urgentFlash && !entranceFlash ? theme::danger : theme::paper);
    }
}
