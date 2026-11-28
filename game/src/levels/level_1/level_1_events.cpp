#include "levels/level_1/level_1.h"

#include <cmath>

namespace rage {
    std::vector<Event> Level1::CreateEvents() {
        return {
            {
                .id = "sidewalk-tip",
                .condition = [this] { return GetPlayer().Position().x >= 165.0f && GetPlayer().Position().x < 590.0f && CanSay(); },
                .execute = [this] {
                    Say({.text = "Those trash cans are practically a welcome mat. Step right into them.",
                         .effects = {.holdSeconds = 2.0f, .fadeSeconds = 1.0f}});
                }
            },
            {
                .id = "road-tip",
                .condition = [this] { return GetPlayer().Position().x >= 585.0f && GetPlayer().Position().x < 2700.0f && CanSay(); },
                .execute = [this] {
                    Say({.text = "Don't worry about the traffic. Cars always stop for students.\n"
                                 "Well, almost always.",
                         .effects = {.holdSeconds = 2.5f, .fadeSeconds = 1.0f}});
                }
            },
            {
                .id = "chicken-pileup",
                .condition = [this] { return pileupStarted && GetPlayer().Position().x < 3200.0f && CanSay(); },
                .execute = [this] {
                    Say({.text = "A chicken crossing the road? How original.\n"
                                 "Looks like the drivers weren't ready for that joke.",
                         .effects = {.holdSeconds = 2.0f, .fadeSeconds = 1.0f}});
                }
            },
            {
                .id = "road-checkpoint",
                .condition = [this] { return pileupSettled && GetPlayer().Position().x >= 2700.0f && !roadCheckpointReached; },
                .execute = [this] {
                    roadCheckpointReached = true;
                    checkpoint = {2700.0f, 1436.0f};
                    Say({.text = "A car pile-up makes a staircase now. Very convenient.\n"
                                 "Checkpoint saved. Try not to look relieved.",
                         .effects = {.holdSeconds = 3.0f, .fadeSeconds = 1.0f}}, DialoguePolicy::Interrupt);
                }
            },
            {
                .id = "sky-checkpoint",
                .condition = [this] {
                    return GetPlayer().Position().x >= 3740.0f && GetPlayer().Position().y <= 850.0f
                        && !skyCheckpointReached;
                },
                .execute = [this] {
                    skyCheckpointReached = true;
                    checkpoint = {3720.0f, 784.0f};
                    Say({.text = "Welcome to the sky. Jump onto clouds when the wires flash yellow.\n"
                                 "Another checkpoint. You're getting suspiciously good at this.",
                         .effects = {.holdSeconds = 3.0f, .fadeSeconds = 1.0f}}, DialoguePolicy::Interrupt);
                }
            },
            {
                .id = "cloud-checkpoint",
                .condition = [this] {
                    const auto body = GetPlayer().Bounds();
                    const auto cloud = Objects().Get("cloud-1")->Bounds();
                    return GetPlayer().Grounded() && std::abs(body.y + body.height - cloud.y) < 0.1f
                        && body.x < cloud.x + cloud.width && body.x + body.width > cloud.x;
                },
                .execute = [this] {
                    const auto cloud = Objects().Get("cloud-1")->Position();
                    // Respawns stand up, even when this checkpoint was reached crouching.
                    checkpoint = {cloud.x, cloud.y - GetPlayer().StandingBounds().height};
                    Say({.text = "Checkpoint saved. At least the clouds aren't plugged in.",
                         .effects = {.holdSeconds = 2.0f, .fadeSeconds = 1.0f}}, DialoguePolicy::Interrupt);
                }
            },
            {
                .id = "pig-warning",
                .condition = [this] { return GetPlayer().Position().x >= 4710.0f && GetPlayer().Position().x < 6610.0f && CanSay(); },
                .execute = [this] {
                    Say({.text = "Hold S or Shift to duck low pigs. Those green farts are lethal.\n"
                                 "Boxed in? Wait on a cloud and let the herd pass.",
                         .effects = {.holdSeconds = 3.0f, .fadeSeconds = 1.0f}});
                }
            },
            {
                .id = "slide-announcement",
                .execute = [this] {
                    Say({.text = "A rainbow slide to campus? Sure, that's how commuting works.",
                         .effects = {.holdSeconds = 2.0f, .fadeSeconds = 1.0f}}, DialoguePolicy::Interrupt);
                },
                .matchesNotification = [](const EventNotification& n) {
                    return n.topic == "level1.slide_started";
                }
            },
            {
                .id = "campus-arrival",
                .condition = [this] {
                    return slideFinished && GetPlayer().Position().x >= 6975.0f && !campusReached;
                },
                .execute = [this] {
                    campusReached = true;
                    Say({.text = "You actually made it to campus. Unbelievable.\n"
                                 "Now find the lecture hall before I think of something else.",
                         .effects = {.holdSeconds = 5.0f, .fadeSeconds = 1.5f}}, DialoguePolicy::Interrupt);
                }
            },
            {
                .id = "anvil-hit",
                .execute = [this] {
                    Restart();
                    RequestSound(playerSounds::death);
                    Say({.text = "Back to square one. Next time, let the anvil go first.",
                         .effects = {.holdSeconds = 2.0f, .fadeSeconds = 1.0f}}, DialoguePolicy::Interrupt);
                },
                .matchesNotification = [](const EventNotification& n) {
                    return n.topic == "level1.anvil_hit";
                }
            },
            {
                .id = "obstacle-hit",
                .execute = [this] {
                    Say({.text = "Outplayed by sidewalk furniture. Excellent start.",
                         .effects = {.holdSeconds = 1.4f, .fadeSeconds = 0.9f}}, DialoguePolicy::Interrupt);
                },
                .matchesNotification = [](const EventNotification& n) {
                    return n.topic == "level1.obstacle_hit";
                },
                .repeatable = true
            },
            {
                .id = "traffic-hit",
                .execute = [this] {
                    Say({.text = "The car insists it had the right of way. Again?",
                         .effects = {.holdSeconds = 1.4f, .fadeSeconds = 0.9f}}, DialoguePolicy::Interrupt);
                },
                .matchesNotification = [](const EventNotification& n) {
                    return n.topic == "level1.traffic_hit";
                },
                .repeatable = true
            },
            {
                .id = "sky-hit",
                .execute = [this] {
                    Say({.text = "Apparently the sky also has a traffic problem.",
                         .effects = {.holdSeconds = 1.4f, .fadeSeconds = 0.9f}}, DialoguePolicy::Interrupt);
                },
                .matchesNotification = [](const EventNotification& n) {
                    return n.topic == "level1.star_hit"
                        || n.topic == "level1.pig_hit" || n.topic == "level1.pig_gas_hit" || n.topic == "level1.fall_hit";
                },
                .repeatable = true
            },
            {
                .id = "wire-hit",
                .execute = [this] {
                    Say({.text = "They said the phone lines were live. You took it literally.",
                         .effects = {.holdSeconds = 1.4f, .fadeSeconds = 0.9f}}, DialoguePolicy::Interrupt);
                },
                .matchesNotification = [](const EventNotification& n) {
                    return n.topic == "level1.wire_hit";
                },
                .repeatable = true
            }
        };
    }
}
