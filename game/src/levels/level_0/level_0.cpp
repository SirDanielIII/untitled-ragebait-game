#include "levels/level.h"

namespace rage {
    // Edit this file for this stage: layout, narration, events and custom behavior.
    const LevelDefinition &Level0Definition() {
        static const LevelDefinition definition{
            .id = LevelId::Level0,
            .title = "Level 0",
            .description = "Leave the house through the front door.",
            .plannedContent = "House hallucinations: falling lights, flooded rooms, false doors and a fireplace monster.",
            .intro = {
                .text = "Good morning. You barely slept, and class has already started.\nI would check the front door. Ignore whatever the furniture is doing.",
                .speaker = "The Entity",
                .speedMultiplier = 1.0f,
                .startDelay = 0.0f,
                .fontSize = 24.0f,
                .allowSkip = true,
                .allowReplay = true
            },
            .accent = {108, 181, 170, 255},
            .backdrop = ArtId::Level0,
            .world = {0, 0, 1500, 700},
            .spawn = {100, 606},
            .platforms = {
                {0, 650, 1500, 50}, // Floor
                {350, 550, 180, 24},
                {630, 470, 180, 24},
                {920, 550, 180, 24},
            },
        };
        return definition;
    }

    namespace {
        class Level0 final : public Level {
        public:
            Level0() : Level(Level0Definition()) {
                Events().RegisterEvent({
                    .id = "hallway-cue",
                    .condition = [this] { return GetPlayer().Position().x >= 350.0f && Dialogue().IsComplete(); },
                    .execute = [this] { hallwayCue = Say({.text = "Wait here a moment. Let your eyes adjust."}); }
                });
                Events().RegisterEvent({
                    .id = "hallway-cue-revealed",
                    .execute = [this] { followUpTime = ElapsedSeconds() + 2.0f; },
                    .dependencies = {"hallway-cue"},
                    .matchesNotification = [this](const EventNotification &notice) {
                        return notice.topic == dialogueEvents::revealCompleted && notice.sourceId == hallwayCue;
                    }
                });
                Events().RegisterEvent({
                    .id = "hallway-follow-up",
                    .condition = [this] { return ElapsedSeconds() >= followUpTime && Dialogue().IsComplete(); },
                    .execute = [this] { Say({.text = "That should do it. The front door is still ahead."}); },
                    .dependencies = {"hallway-cue-revealed"}
                });
            }

        private:
            void OnReset() override {
                hallwayCue = 0;
                followUpTime = 0.0f;
            }

            DialogueMessageId hallwayCue = 0;
            float followUpTime = 0.0f;
        };
    }

    std::unique_ptr<Level> MakeLevel0() { return std::make_unique<Level0>(); }
}
