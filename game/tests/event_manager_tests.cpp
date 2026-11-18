#include "levels/event_manager.h"
#include "test_support.h"

#include <iostream>

namespace rage {
    namespace {
        using test::Check;

        void CheckRejected(EventManager &events, const Event &event) {
            try {
                events.RegisterEvent(event);
            } catch (const std::invalid_argument &) {
                return;
            }
            throw std::runtime_error("Invalid event registration succeeded: " + event.id);
        }

        void TestRegistration() {
            EventManager events;
            const auto noAction = [] {};
            CheckRejected(events, {.id = "", .execute = noAction});
            CheckRejected(events, {.id = "no-callback", .execute = {}});
            CheckRejected(events, {.id = "self", .execute = noAction, .dependencies = {"self"}});
            // Neither half of a cycle can be registered first.
            CheckRejected(events, {.id = "first", .execute = noAction, .dependencies = {"second"}});
            CheckRejected(events, {.id = "second", .execute = noAction, .dependencies = {"first"}});
            events.RegisterEvent({.id = "first", .execute = noAction});
            CheckRejected(events, {.id = "first", .execute = noAction});
        }

        void TestDependencies() {
            EventManager events;
            bool left = false;
            bool right = true;
            int runs = 0;
            events.RegisterEvent({.id = "left", .condition = [&] { return left; }, .execute = [] {}});
            events.RegisterEvent({.id = "right", .condition = [&] { return right; }, .execute = [] {}});
            events.RegisterEvent({
                .id = "both",
                .execute = [&] {
                    Check(!events.IsCompleted("both"), "Event completed before its callback finished");
                    ++runs;
                },
                .dependencies = {"left", "right"}
            });
            events.Update();
            Check(events.IsCompleted("right") && runs == 0, "Event ran with only its second prerequisite complete");
            events.Reset();
            left = true;
            right = false;
            events.Update();
            Check(events.IsCompleted("left") && !events.IsCompleted("right") && runs == 0,
                  "Event ran with only its first prerequisite complete");
            right = true;
            events.Update();
            events.Update();
            Check(events.IsCompleted("both") && runs == 1, "One-time event did not execute exactly once");
            events.Reset();
            Check(!events.IsCompleted("both"), "Reset retained completion state");
            events.Update();
            Check(runs == 2, "Reset did not rearm the dependency chain");
        }

        void TestRepeatableEvents() {
            EventManager events;
            bool inside = true;
            int entries = 0;
            events.RegisterEvent({
                .id = "enter-region", .condition = [&] { return inside; }, .execute = [&] { ++entries; }, .repeatable = true
            });
            events.Update();
            events.Update();
            Check(entries == 1, "Held condition retriggered the event");
            inside = false;
            events.Update();
            Check(events.IsCompleted("enter-region"), "Rearming lost repeatable event completion");
            inside = true;
            events.Update();
            Check(entries == 2, "Condition did not rearm after going false");
        }

        void TestNotifications() {
            EventManager events;
            bool unlocked = false;
            bool enabled = true;
            int interactions = 0;
            int responses = 0;
            events.RegisterEvent({.id = "unlock", .condition = [&] { return unlocked; }, .execute = [] {}});
            events.RegisterEvent({
                .id = "interact",
                .condition = [&] { return enabled; },
                .execute = [&] { ++interactions; events.Notify({"response"}); },
                .dependencies = {"unlock"},
                .matchesNotification = [](const EventNotification &notice) {
                    return notice.topic == "interact" && notice.sourceId == 42;
                },
                .repeatable = true
            });
            events.RegisterEvent({
                .id = "respond", .execute = [&] { ++responses; }, .dependencies = {"interact"},
                .matchesNotification = [](const EventNotification &notice) { return notice.topic == "response"; },
                .repeatable = true
            });
            events.Notify({"interact", 42});
            events.Update();
            Check(interactions == 0, "A notification bypassed a prerequisite");
            unlocked = true;
            events.Notify({"interact", 99});
            events.Update();
            Check(interactions == 0, "A stale or wrong-source notification activated an event");
            enabled = false;
            events.Notify({"interact", 42});
            events.Update();
            Check(interactions == 0, "A notification bypassed the trigger condition");
            enabled = true;
            events.Notify({"interact", 42});
            events.Notify({"interact", 42});
            Check(interactions == 0, "A notification ran a callback before update");
            events.Update();
            Check(interactions == 1 && responses == 0, "Notifications did not coalesce or defer callback posts");
            events.Update();
            Check(interactions == 1 && responses == 1, "Callback notification failed or an old notification was reused");
            events.Notify({"interact", 42});
            events.Update();
            Check(interactions == 2, "A new notification did not reactivate the event");
            events.Notify({"interact", 42});
            events.Reset();
            events.Update();
            Check(interactions == 2 && responses == 1, "Reset retained pending notifications");
        }

        void TestResetDuringCallback() {
            EventManager events;
            bool restart = true;
            int runs = 0;
            events.RegisterEvent({
                .id = "restart", .execute = [&] { if (restart) events.Reset(); }
            });
            events.RegisterEvent({.id = "later", .execute = [&] { ++runs; }});
            events.Update();
            Check(!events.IsCompleted("restart") && runs == 0, "Reset did not stop the old frame");
            restart = false;
            events.Update();
            Check(events.IsCompleted("restart") && runs == 1, "Update failed after an event-triggered reset");
        }
    }

    void TestEventManager() {
        TestRegistration();
        TestDependencies();
        TestRepeatableEvents();
        TestNotifications();
        TestResetDuringCallback();
        std::cout << "PASS: event validation, dependencies, repeat triggers, notifications and reset\n";
    }
}
