#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace rage {
    enum class DialogueRevealMethod { None, Natural, Enter, Instant };

    struct EventNotification {
        std::string topic;
        std::uint64_t sourceId = 0; // Optional identifier for a particular object or message.
        DialogueRevealMethod revealMethod = DialogueRevealMethod::None;
        std::string objectId; // Readable collision source, also available after removal.
    };

    struct Event {
        std::string id;
        // Predicates read gameplay state. An omitted condition is always satisfied.
        std::function<bool()> condition = {};
        std::function<void()> execute;
        std::vector<std::string> dependencies = {};
        // When supplied, a matching notification is also required.
        std::function<bool(const EventNotification &)> matchesNotification = {};
        bool repeatable = false;
    };

    class EventManager {
    public:
        EventManager() = default;
        EventManager(const EventManager &) = delete;
        EventManager &operator=(const EventManager &) = delete;

        // Register prerequisites first. Unique IDs and backward-only dependencies
        // rule out self-references and cycles. Invalid definitions throw std::invalid_argument.
        void RegisterEvent(Event event);

        // Runs in registration order. Notifications are available to every event for
        // one update, with at most one execution per event. Callback posts wait for the next update.
        void Update();
        void Notify(EventNotification notification);
        void Reset();

        // A repeatable event stays completed after its first execution until reset.
        bool IsCompleted(std::string_view eventId) const;

    private:
        struct RegisteredEvent {
            Event event;
            bool completed = false;
            bool armed = true;
        };

        const RegisteredEvent *Find(std::string_view id) const;

        std::vector<RegisteredEvent> events;
        std::vector<EventNotification> pendingNotifications;
        bool updating = false;
        bool resetDuringUpdate = false;
    };
}
