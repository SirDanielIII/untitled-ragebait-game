#include "levels/event_manager.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace rage {
    const EventManager::RegisteredEvent *EventManager::Find(std::string_view id) const {
        for (const auto &entry: events) {
            if (entry.event.id == id) return &entry;
        }
        return nullptr;
    }

    void EventManager::RegisterEvent(Event event) {
        if (updating) throw std::logic_error("Cannot register events during an event update");
        if (event.id.empty() || !event.execute || Find(event.id))
            throw std::invalid_argument("Invalid event ID or callback: " + event.id);
        for (const auto &dependency: event.dependencies) {
            if (!Find(dependency))
                throw std::invalid_argument("Event '" + event.id + "' has an unregistered dependency: " + dependency);
        }
        events.push_back({std::move(event)});
    }

    void EventManager::Update() {
        if (updating) throw std::logic_error("Cannot run a nested event update");
        updating = true;
        resetDuringUpdate = false;
        struct EndUpdate {
            bool &updating;
            ~EndUpdate() { updating = false; }
        } endUpdate{updating};

        std::vector<EventNotification> notifications;
        notifications.swap(pendingNotifications);
        for (auto &entry: events) {
            const auto &event = entry.event;
            if (entry.completed && !event.repeatable) continue;
            if (!std::ranges::all_of(event.dependencies, [this](const auto &id) { return IsCompleted(id); }))
                continue;

            if (event.matchesNotification && !std::ranges::any_of(notifications, event.matchesNotification))
                continue;
            if (event.condition && !event.condition()) {
                entry.armed = true;
                continue;
            }
            // Polling events rearm when their condition goes false. Each new
            // notification batch can activate a notification-driven event.
            if (!event.matchesNotification && !entry.armed) continue;
            event.execute();
            // A callback can restart the owning level. Stop processing the old frame.
            if (resetDuringUpdate) break;
            entry.completed = true;
            entry.armed = false;
        }
    }

    void EventManager::Notify(EventNotification notification) {
        pendingNotifications.push_back(std::move(notification));
    }

    void EventManager::Reset() {
        pendingNotifications.clear();
        for (auto &entry: events) {
            entry.completed = false;
            entry.armed = true;
        }
        resetDuringUpdate = updating;
    }

    bool EventManager::IsCompleted(std::string_view eventId) const {
        const auto *entry = Find(eventId);
        return entry && entry->completed;
    }
}
