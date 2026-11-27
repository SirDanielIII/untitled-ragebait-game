#include "levels/level_3/level_3.h"

namespace rage {
    std::vector<Event> Level3::CreateEvents() {
        return {
            {

                .id = "first-anvil",
                .condition = [this] { return AnvilWarningShowing() && CanSay(); },
                .execute = [this] {
                    Say({.text = "Is that... an anvil? In a school?\n"
                                 "Weird.",
                         .effects = {.holdSeconds = 2.5f, .fadeSeconds = 1.0f}});
                }
            },
            {
                .id = "chair-tip",
                .condition = [this] { return StandingOnChair() && CanSay(); },
                .execute = [this] {
                    Say({.text = "Classroom chairs. Famously stable.\n"
                                 "Nobody has ever fallen off one. Ever.",
                         .effects = {.holdSeconds = 2.0f, .fadeSeconds = 1.0f}});
                }
            },
            {
                // Interrupts, because the book's countdown is short and the warning matters.
                .id = "book-tip",
                .condition = [this] { return AnyBookCounting(); },
                .execute = [this] {
                    Say({.text = "Oh, those are library books. Very thin. Very overdue.\n"
                                 "I don't think they were built to hold a person.",
                         .effects = {.holdSeconds = 2.0f, .fadeSeconds = 1.0f}}, DialoguePolicy::Interrupt);
                }
            },
            {
                .id = "book-falls",
                .condition = [this] { return AnyBookFallen(); },
                .execute = [this] {
                    Say({.text = "See? Books can't hold you up.\n"
                                 "Maybe that's a sign. The sign says \"go home.\"",
                         .effects = {.holdSeconds = 2.0f, .fadeSeconds = 1.0f}}, DialoguePolicy::Interrupt);
                }
            },
            {
                // Waits for the Entity to finish talking
                .id = "first-death",
                .condition = [this] { return deaths >= 1 && CanSay(); },
                .execute = [this] {
                    Say({.text = "Ouch. That looked like it hurt.\n"
                                 "Your bed doesn't hurt. Just saying.",
                         .effects = {.holdSeconds = 2.0f, .fadeSeconds = 1.0f}});
                }
            },
            {

                .id = "books-reset",
                .execute = [this] { ResetBooks(); ResetFallingObjects(); ++deaths; },
                .matchesNotification = [](const EventNotification &n) { return n.topic == objectEvents::hazardHit; },
                .repeatable = true
            },
        };
    }
}
