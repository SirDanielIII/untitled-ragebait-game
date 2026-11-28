#include "levels/level.h"
#include "engine/graphics/layout.h"
#include "engine/graphics/placeholder_art.h"
#include "ui/text/text.h"
#include "ui/hud/gameplay_hud.h"
#include "rlgl.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace rage {
    Level::Level(const LevelDefinition &definition) : definition(definition) {
        camera.offset = {worldViewport.width / 2, worldViewport.y + worldViewport.height / 2};
        camera.zoom = 2.0f;
        // Objects are initialised when the level loads, from its
        // definitions. Leaving a level destroys the Level (and this
        // manager with it), so no object can outlive its level.
        for (const auto &objectDefinition : definition.objects) {
            objects.Create(objectDefinition);
        }
        respawnPoint = definition.spawn;
    }

    void Level::Reset(const GameConfig &config) {
        preferredTextSpeed = config.charactersPerSecond;
        player.Reset(definition.spawn, config);
        Restart();
    }

    void Level::Restart() {
        events.Reset();
        player.RecordDamageSource({});
        player.Reset(definition.spawn);
        jumpParticles.Clear();
        respawnPoint = definition.spawn;
        levelComplete = false;
        objects.ResetAll();
        collisions.Reset();
        scriptCollisions.Reset();
        scriptBounds.clear();
        scriptShapes.clear();
        scriptContacts.clear();
        previousScriptContacts.clear();
        playerAnimator = {};
        playerAnimator.SetState("idle");
        soundRequests.clear();
        trackingPlayerSounds = false; // A restart is not a landing.
        playerFacing = Facing::Right;
        elapsedSeconds = 0;
        currentCue.id = 0;
        Say(definition.intro);
        OnReset();
        RefreshColliders();
        UpdateCamera();
    }

    void Level::Update(float dt, const InputFrame &input, const GameConfig &config) {
        if (input.resetPlayer) {
            Reset(config);
            return;
        }
        if (!std::isfinite(dt) || dt < 0) return;
        elapsedSeconds += dt;
        requestedThisFrame = false;
        preferredTextSpeed = config.charactersPerSecond;
        const bool instant = ApplyDialogueSpeed();
        auto revealMethod = instant ? DialogueRevealMethod::Instant : DialogueRevealMethod::Natural;
        if (input.replayDialogue && currentCue.allowReplay) {
            dialogue.writer.Reset();
            dialogueDelay = instant ? 0 : currentCue.startDelay;
        }
        if (input.completeDialogue && currentCue.allowSkip) {
            if (!dialogue.writer.IsComplete()) revealMethod = DialogueRevealMethod::Enter;
            dialogue.writer.Complete();
            dialogueDelay = 0;
        }
        const float revealTime = std::max(0.0f, dt - dialogueDelay);
        dialogueDelay = std::max(0.0f, dialogueDelay - dt);
        dialogue.writer.Update(revealTime);
        PublishDialogueMilestones(revealMethod);
        InputFrame playerInput = input;
        playerInput.climb = input.climb && CanClimb();
        // Objects and actors advance on the same clock, including long frames.
        scriptPlayerFrom = player.Bounds();
        float remaining = dt;
        do {
            const float step = std::min(remaining, 1.0f / 120.0f);
            objects.UpdateAll(step);
            RefreshColliders();
            player.Update(step, playerInput, config, collisions);
            if (CheckHazardsAndTriggers()) break;
            remaining -= step;
        } while (remaining > 0);
        jumpParticles.Update(dt);
        if (const auto feet = player.TakeCrouchJump()) jumpParticles.Emit(*feet);
        UpdatePlayerAnimation(dt);
        UpdatePlayerSounds(dt);
        scriptShapes.clear();
        scriptContacts.clear();
        OnUpdate(dt, input);
        scriptBounds.clear();
        for (const auto &shape : scriptShapes) scriptBounds[shape.id] = shape;
        for (const auto &event : scriptCollisions.TrackContacts("player", scriptContacts)) {
            const auto &c = event.contact;
            events.Notify({event.phase == ContactPhase::Enter ? objectEvents::contactEntered : objectEvents::contactExited,
                           ObjectNotificationId(c.objectId), DialogueRevealMethod::None, c.objectId});
        }
        previousScriptContacts = scriptContacts;
        scriptCollisions.SetColliders(scriptShapes);
        events.Update();
        UpdateCamera();
    }

    bool Level::ScriptContact(const std::string &id, Rectangle bounds, CollisionType type, bool enabled) {
        auto previous = bounds;
        if (const auto old = scriptBounds.find(id); old != scriptBounds.end() && old->second.type != CollisionType::None)
            previous = old->second.bounds;
        scriptShapes.push_back({id, bounds, previous, enabled ? type : CollisionType::None});
        if (!enabled || !CollisionSystem::TouchesDuringMove(scriptPlayerFrom, player.Bounds(), previous, bounds)) return false;
        scriptContacts.push_back({id, type});
        const bool entered = std::ranges::none_of(previousScriptContacts, [&](const auto &c) { return c.objectId == id; });
        if (entered && type == CollisionType::Hazard) {
            player.RecordDamageSource(id);
            events.Notify({objectEvents::hazardHit, ObjectNotificationId(id), DialogueRevealMethod::None, id});
        }
        return entered;
    }

    bool Level::ActivateObject(const std::string &id) {
        if (!objects.Activate(id)) {
            return false;
        }
        if (const GameObject *object = objects.Get(id)) {
            NotifyObject(*object, objectEvents::activated);
        }
        return true;
    }

    bool Level::DeactivateObject(const std::string &id) {
        if (!objects.Deactivate(id)) {
            return false;
        }
        if (const GameObject *object = objects.Get(id)) {
            NotifyObject(*object, objectEvents::activated);
        }
        return true;
    }

    bool Level::StartObjectFall(const std::string &id) {
        return objects.StartFall(id);
    }

    void Level::NotifyObject(const GameObject &object, const char *topic) {
        events.Notify({topic, ObjectNotificationId(object.Id()), DialogueRevealMethod::None, object.Id()});
    }

    void Level::OnTrigger(GameObject &object) {
        // Id conventions, so levels need no code for the common cases.
        if (object.Id().rfind("checkpoint-", 0) == 0) {
            respawnPoint = object.Position();
        } else if (object.Id().rfind("exit-", 0) == 0) {
            levelComplete = true;
        }
    }

    void Level::RefreshColliders() {
        auto scene = objects.Colliders();
        const auto w = definition.world;
        const auto add = [&](const char *id, Rectangle bounds, CollisionType type) {
            scene.push_back({id, bounds, bounds, type});
        };
        if (definition.sideBoundaries) {
            add("boundary-left", {w.x - 100, w.y - 100, 100, w.height + 1000}, CollisionType::Solid);
            add("boundary-right", {w.x + w.width, w.y - 100, 100, w.height + 1000}, CollisionType::Solid);
        }
        if (definition.ceilingBoundary) add("boundary-top", {w.x, w.y - 100, w.width, 100}, CollisionType::Solid);
        if (definition.bottomKillZone) add("world-fall", {w.x - 100, w.y + w.height + 100, w.width + 200, 1000}, CollisionType::Hazard);
        collisions.SetColliders(std::move(scene));
    }

    bool Level::CheckHazardsAndTriggers() {
        for (const auto &object : objects.Objects()) {
            if (object->TakeActionCompleted()) NotifyObject(*object, objectEvents::actionCompleted);
        }
        const auto contactEvents = collisions.TrackContacts("player", player.LastCollision().contacts);
        for (const auto &event : contactEvents) {
            const auto &contact = event.contact;
            const bool entered = event.phase == ContactPhase::Enter;
            events.Notify({entered ? objectEvents::contactEntered : objectEvents::contactExited,
                ObjectNotificationId(contact.objectId), DialogueRevealMethod::None, contact.objectId});
            auto *object = objects.Get(contact.objectId);
            if (!entered) {
                if (object && object->IsActive()) object->ClearTrigger();
                continue;
            }
            if (object && !object->IsActive()) continue;
            if (contact.type == CollisionType::Trigger && object) {
                object->OnTrigger();
                NotifyObject(*object, objectEvents::triggerEntered);
                OnTrigger(*object);
            }
            if (contact.type == CollisionType::Hazard) {
                player.RecordDamageSource(contact.objectId);
                events.Notify({objectEvents::hazardHit, ObjectNotificationId(contact.objectId),
                               DialogueRevealMethod::None, contact.objectId});
                if (object) object->OnTrigger();
                if (!object || object->Lethal()) player.Kill();
                else player.TakeDamage(object->Damage());
                RequestSound(player.IsDead() ? playerSounds::death : playerSounds::hurt);
                lastHealth = player.GetHealth();
                if (player.IsDead()) {
                    player.Reset(respawnPoint);
                    scriptPlayerFrom = player.Bounds();
                    trackingPlayerSounds = false;
                    return true;
                }
            }
        }
        if (player.LastCollision().crushed) {
            const auto &contacts = player.LastCollision().contacts;
            player.RecordDamageSource(contacts.empty() ? "crush" : contacts.front().objectId);
            player.Kill();
            RequestSound(playerSounds::death);
            player.Reset(respawnPoint);
            scriptPlayerFrom = player.Bounds();
            trackingPlayerSounds = false;
            return true;
        }
        return false;
    }

    void Level::SubmitSprites(RenderQueue &queue, const Resources &resources) const {
        // Objects draw through the AssetManager: a Game Data row, or
        // the platform placeholder while the art is not in the sheet.
        // Collision bounds are independent of whichever is drawn.
        const Color accent = definition.accent;
        objects.SubmitSprites(queue, resources.Assets(),
                              [&resources, accent](Rectangle bounds, bool) {
                                  art::Platform(resources, bounds, accent);
                              });
    }

    bool Level::ApplyDialogueSpeed() {
        dialogue.writer.SetSpeed(preferredTextSpeed * currentCue.speedMultiplier);
        const bool instant = preferredTextSpeed == 0 || currentCue.speedMultiplier == 0;
        if (instant) dialogueDelay = 0;
        return instant;
    }

    bool Level::CanSay() const {
        return currentCue.id == 0 || (!requestedThisFrame && revealCompleted && dialogue.writer.IsComplete()
            && (!dialogue.writer.FadesOut() || dialogue.writer.Opacity() == 0));
    }

    DialogueMessageId Level::Say(DialogueCue cue, DialoguePolicy policy) {
        if (policy == DialoguePolicy::WhenAvailable && !CanSay()) return 0;
        cue.id = currentCue.id + 1;
        currentCue = std::move(cue);
        requestedThisFrame = true;
        currentCue.speedMultiplier = std::isfinite(currentCue.speedMultiplier) ? std::clamp(currentCue.speedMultiplier, 0.0f, 10.0f) : 1.0f;
        currentCue.startDelay = std::isfinite(currentCue.startDelay) ? std::max(0.0f, currentCue.startDelay) : 0.0f;
        currentCue.fontSize = std::isfinite(currentCue.fontSize) ? std::clamp(currentCue.fontSize, 12.0f, 48.0f) : 24.0f;
        dialogueDelay = currentCue.startDelay;
        const bool instant = ApplyDialogueSpeed();
        dialogue.writer.SetEffects(currentCue.effects);
        dialogue.writer.Replace(currentCue.text);
        revealStarted = false;
        revealCompleted = false;
        PublishDialogueMilestones(instant ? DialogueRevealMethod::Instant : DialogueRevealMethod::Natural);
        return currentCue.id;
    }

    void Level::PublishDialogueMilestones(DialogueRevealMethod method) {
        if (currentCue.id == 0 || dialogueDelay > 0 || revealCompleted) return;
        // Replay resets presentation only. Each Say emits milestones once per message ID.
        if (!revealStarted) {
            events.Notify({dialogueEvents::revealStarted, currentCue.id});
            revealStarted = true;
        }
        if (dialogue.writer.IsComplete()) {
            events.Notify({dialogueEvents::revealCompleted, currentCue.id, method});
            revealCompleted = true;
        }
    }

    void Level::RequestSound(std::string soundId) {
        if (!soundId.empty()) soundRequests.push_back(std::move(soundId));
    }

    std::vector<std::string> Level::TakeSoundRequests() {
        std::vector<std::string> requests;
        requests.swap(soundRequests);
        return requests;
    }

    std::string Level::MusicTrack() const {
        return "level_" + std::to_string(static_cast<int>(definition.id)) + "_theme";
    }

    void Level::UpdatePlayerSounds(float dt) {
        const bool grounded = player.Grounded();
        const int health = player.GetHealth();
        const Vector2 position = player.Position();
        const Vector2 velocity = player.Velocity();
        const float speed = std::max(std::abs(velocity.x), std::abs(velocity.y));
        // Moved farther than its speed allows (plus room for collision push-outs): the player
        // was put somewhere by a respawn or a script (checkpoints, Level 1's slide). That is
        // not a jump or a landing, so this frame only records the new state.
        const float reach = std::max(speed, lastPlayerSpeed) * dt + 24.0f;
        const bool teleported = std::abs(position.x - lastPlayerPosition.x) > reach
                                || std::abs(position.y - lastPlayerPosition.y) > reach;
        if (trackingPlayerSounds && !teleported) {
            // Leaving the ground while moving up is a jump; walking off a ledge is not.
            if (wasGrounded && !grounded && player.Velocity().y < 0) RequestSound(playerSounds::jump);
            else if (!wasGrounded && grounded) RequestSound(playerSounds::land);
            if (health < lastHealth) RequestSound(player.IsDead() ? playerSounds::death : playerSounds::hurt);
        }
        trackingPlayerSounds = true;
        wasGrounded = grounded;
        lastHealth = health;
        lastPlayerPosition = position;
        lastPlayerSpeed = speed;
    }

    void Level::UpdatePlayerAnimation(float dt) {
        const auto velocity = player.Velocity();
        if (velocity.x < 0) playerFacing = Facing::Left;
        else if (velocity.x > 0) playerFacing = Facing::Right;
        if (player.Crouching()) playerAnimator.SetState(velocity.x != 0 ? "crouch_walk" : "crouch");
        else if (!player.Grounded()) playerAnimator.SetState(velocity.y < 0 ? "jump" : "fall");
        else playerAnimator.SetState(velocity.x != 0 ? "walk" : "idle");
        playerAnimator.Update(dt);
    }

    void Level::SubmitPlayer(RenderQueue &queue, const Resources &resources) const {
        const auto &assets = resources.Assets();
        const auto &state = playerAnimator.State();
        // Missing states fall back so a half-finished set of animations still shows something.
        const AnimationData *animation = assets.Find(playerAssetId, state);
        if (!animation && state == "crouch_walk") animation = assets.Find(playerAssetId, "crouch");
        const bool hasCrouchAnimation = player.Crouching() && animation;
        if (!animation && state == "fall") animation = assets.Find(playerAssetId, "jump");
        if (!animation) animation = assets.Find(playerAssetId, "idle");
        if (!animation) animation = assets.Find(playerAssetId, "");
        const Rectangle body = player.VisualBounds();
        if (player.Crouching() && !hasCrouchAnimation) {
            const Vector2 feet = SnapToPixels({body.x + body.width / 2, body.y + body.height});
            const Rectangle standing = player.VisualStandingBounds();
            const float heightScale = body.height / standing.height;
            // Until crouch art is supplied, compress standing art to match the hitbox.
            queue.Submit(animation ? animation->layer : Layer::Middle,
                         [this, &resources, &assets, animation, standing, feet, heightScale] {
                rlPushMatrix();
                rlTranslatef(feet.x, feet.y, 0.0f);
                rlScalef(1.0f, heightScale, 1.0f);
                rlTranslatef(-feet.x, -feet.y, 0.0f);
                if (animation) {
                    assets.Draw(*animation, playerAnimator.Frame(*animation), feet, playerFacing,
                                [&resources](Rectangle b, bool) { art::Student(resources, b); });
                } else {
                    art::Student(resources, standing);
                }
                rlPopMatrix();
            });
            return;
        }
        if (!animation) {
            // No Game Data rows for the player yet: keep the original drawing at the collision size.
            queue.Submit(Layer::Middle, [&resources, body] { art::Student(resources, body); });
            return;
        }
        const Vector2 feet = SnapToPixels({body.x + body.width / 2, body.y + body.height});
        queue.Submit(assets, *animation, playerAnimator.Frame(*animation), feet, playerFacing,
                     [&resources](Rectangle bounds, bool) { art::Student(resources, bounds); });
    }

    Vector2 Level::SnapToPixels(Vector2 point) const {
        return {std::round(point.x * camera.zoom) / camera.zoom, std::round(point.y * camera.zoom) / camera.zoom};
    }

    void Level::UpdateCamera() {
        const auto body = player.VisualStandingBounds();
        const auto world = definition.world;
        auto follow = [](float target, float start, float length, float halfView) {
            if (length <= halfView * 2) return start + length / 2;
            return std::clamp(target, start + halfView, start + length - halfView);
        };
        camera.target = SnapToPixels({
            follow(body.x + body.width / 2, world.x, world.width, worldViewport.width / (2 * camera.zoom)),
            follow(body.y + body.height / 2, world.y, world.height, worldViewport.height / (2 * camera.zoom))
        });
    }

    void Level::Draw(const Resources &resources, const GameConfig &config) const {
        resources.BeginClip(worldViewport);
        DrawRectangleRec(worldViewport, Color{36, 44, 60, 255});
        BeginMode2D(camera);
        RenderQueue queue;
        queue.Submit(Layer::Background, [&] {
            art::LevelBackdrop(resources, definition.backdrop, definition.world, definition.accent);
        });
        queue.Submit(Layer::Middle, [&] {
            DrawWorld(resources);
        });
        SubmitSprites(queue, resources);
        jumpParticles.SubmitSprites(queue, camera.zoom);
        SubmitPlayer(queue, resources);
        queue.Flush();
        if (config.showHitboxes) {
            collisions.DrawDebug();
            scriptCollisions.DrawDebug();
            DrawRectangleLinesEx(player.Bounds(), 1, MAGENTA);
        }
        EndMode2D();
        EndScissorMode();
        DrawPlayerCard(resources, player, config);
        DrawHud(resources);
        dialogue.Draw(resources, {40, 714, 1520, 162}, currentCue.speaker.c_str(), currentCue.fontSize,
                      currentCue.allowSkip, currentCue.allowReplay);
        DrawTextFit(resources, definition.description, {40, 678, 1500, 28}, 22, Color{173, 165, 190, 255});
    }

}
