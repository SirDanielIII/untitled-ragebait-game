#include "levels/level_0/level_0.h"

namespace rage {
    std::vector<Event> Level0::CreateEvents() {
        return {
            // Standing still arms the attendance trap once the taunt has been revealed.
            {
                .id = "idle-taunt",
                .condition = [this] { return stillSeconds >= 10 && CanSay(); },
                .execute = [this] {
                    idleCue = Say({
                        .text = "Why have you been standing still for 10 seconds? I'll touch you.",
                        .speedMultiplier = 1.35f,
                        .effects = {.shakePixels = 2.5f, .shakeHz = 9, .holdSeconds = 3, .fadeSeconds = 1}
                    });
                }
            },
            {
                .id = "attendance-trap-armed",
                .execute = [] {}, // Its completion arms the movement event below.
                .dependencies = {"idle-taunt"},
                .matchesNotification = [this](const EventNotification &notice) {
                    return notice.topic == dialogueEvents::revealCompleted && notice.sourceId == idleCue;
                }
            },
            {
                .id = "back-to-bed",
                .condition = [this] { return movedThisFrame; },
                .execute = [this] {
                    MutablePlayer().Reset(Definition().spawn);
                    previousPosition = Definition().spawn;
                    Say({.text = "Present! In bed. I've corrected the attendance sheet.",
                         .effects = {.holdSeconds = 2, .fadeSeconds = 1.5f}}, DialoguePolicy::Interrupt);
                },
                .dependencies = {"attendance-trap-armed"}
            },

            // Entering the hallway starts a cue and a delayed follow-up.
            {
                .id = "hallway-cue",
                .condition = [this] { return GetPlayer().Position().x >= 350.0f && CanSay(); },
                .execute = [this] { hallwayCue = Say({.text = "Wait here a moment. Let your eyes adjust."}); }
            },
            {
                .id = "hallway-cue-revealed",
                .execute = [this] { followUpTime = ElapsedSeconds() + 2.0f; },
                .dependencies = {"hallway-cue"},
                .matchesNotification = [this](const EventNotification &notice) {
                    return notice.topic == dialogueEvents::revealCompleted && notice.sourceId == hallwayCue;
                }
            },
            {
                .id = "hallway-follow-up",
                .condition = [this] { return ElapsedSeconds() >= followUpTime && CanSay(); },
                .execute = [this] {
                    Say({.text = "That should do it. The front door is still ahead.",
                         .effects = {.holdSeconds = 3, .fadeSeconds = 1}});
                },
                .dependencies = {"hallway-cue-revealed"}
            },
        };
    }
}
