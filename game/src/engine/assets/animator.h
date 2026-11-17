// Animator: remembers what one object is doing ("idle", "walk") and for how long.
//
// Give each animated object its own Animator. Each frame, call SetState() with what the
// object is doing and Update(dt) with the frame time. When drawing, Frame() asks Game
// Data which frame that much time corresponds to.
//
// It stores time rather than a frame number on purpose. Changing FPS or the number of
// frames in the sheet then changes the animation with no code changes.
#pragma once

#include "engine/assets/game_data.h"

#include <cmath>
#include <string>
#include <string_view>

namespace rage {
    class Animator {
    public:
        // Switches to a state and starts it from its first frame. Calling it again with
        // the state that is already playing does nothing, so it is safe to call every frame.
        void SetState(std::string_view next) {
            if (state == next) return;
            state = next;
            seconds = 0;
        }

        // Plays the current state again from the start (e.g. a second jump).
        void Restart() { seconds = 0; }

        // Advances playback by the frame time in seconds. Negative or invalid times are ignored.
        void Update(float dt) {
            if (std::isfinite(dt) && dt > 0) seconds += dt;
        }

        const std::string &State() const { return state; }
        double Seconds() const { return seconds; }

        // Which frame of `animation` to draw now. Pass the animation found for State().
        int Frame(const AnimationData &animation) const { return FrameAt(animation, seconds); }

        // True when a non-looping animation has reached its last frame.
        bool Finished(const AnimationData &animation) const { return IsFinished(animation, seconds); }

    private:
        std::string state;
        double seconds = 0; // double, not float, so long sessions keep precise timing.
    };
}
