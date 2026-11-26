#include "levels/level.h"
#include "engine/graphics/layout.h"
#include "engine/graphics/placeholder_art.h"
#include "ui/text/text.h"
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
        Restart();
    }

    void Level::Restart() {
        events.Reset();
        player.Reset(definition.spawn);
        respawnPoint = definition.spawn;
        levelComplete = false;
        objects.ResetAll();
        playerAnimator = {};
        playerAnimator.SetState("idle");
        soundRequests.clear();
        trackingPlayerSounds = false; // A restart is not a landing.
        playerFacing = Facing::Right;
        elapsedSeconds = 0;
        currentCue.id = 0;
        Say(definition.intro);
        OnReset();
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
        // Objects move first, so the player stands on their updated
        // bounds this frame. Hazards and triggers are checked after
        // the player moves, and before events.Update(), so the
        // notifications they post are visible to events this update.
        objects.UpdateAll(dt);
        player.Update(dt, input, config, objects.SolidBounds(), definition.world,
                      objects.SolidBounds(CollisionType::OneWay));
        UpdatePlayerAnimation(dt);
        UpdatePlayerSounds(dt);
        CheckHazardsAndTriggers();
        OnUpdate(dt, input);
        events.Update();
        UpdateCamera();
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
        events.Notify({topic, ObjectNotificationId(object.Id())});
    }

    void Level::OnTrigger(GameObject &object) {
        // Id conventions, so levels need no code for the common cases.
        if (object.Id().rfind("checkpoint-", 0) == 0) {
            respawnPoint = object.Position();
        } else if (object.Id().rfind("exit-", 0) == 0) {
            levelComplete = true;
        }
    }

    void Level::CheckHazardsAndTriggers() {
        // Actions that finished this frame (e.g. an anvil landing)
        // post once, then the flag is consumed.
        for (const auto &object : objects.Objects()) {
            if (object->TakeActionCompleted()) {
                NotifyObject(*object, objectEvents::actionCompleted);
            }
        }
        const Rectangle playerBounds = player.Bounds();
        // A hazard respawns the player at the last checkpoint (or the
        // spawn). Objects are not reset, so a reached checkpoint
        // survives the death.
        for (auto *hazard : objects.Overlapping(playerBounds, CollisionType::Hazard)) {
            NotifyObject(*hazard, objectEvents::hazardHit);
            hazard->OnTrigger();
            RequestSound(playerSounds::death);
            player.Reset(respawnPoint);
            trackingPlayerSounds = false; // Appearing at the checkpoint is not a landing.
            return;
        }
        // Triggers fire on entry only: the object's triggered flag is
        // the "player is inside" state, cleared when they leave a
        // repeatable trigger. One-shot triggers deactivate in
        // OnTrigger, so they never fire twice.
        for (auto &object : objects.Objects()) {
            if (!object->IsActive() || object->Collision() != CollisionType::Trigger) {
                continue;
            }
            const bool overlapping = CheckCollisionRecs(playerBounds, object->CollisionBounds());
            if (overlapping && !object->IsTriggered()) {
                object->OnTrigger();
                NotifyObject(*object, objectEvents::triggerEntered);
                OnTrigger(*object);
            } else if (!overlapping && object->IsTriggered() && object->IsActive()) {
                object->ClearTrigger();
            }
        }
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
        if (!player.Grounded()) playerAnimator.SetState(velocity.y < 0 ? "jump" : "fall");
        else playerAnimator.SetState(velocity.x != 0 ? "walk" : "idle");
        playerAnimator.Update(dt);
    }

    void Level::SubmitPlayer(RenderQueue &queue, const Resources &resources) const {
        const auto &assets = resources.Assets();
        const auto &state = playerAnimator.State();
        // Missing states fall back so a half-finished set of animations still shows something.
        const AnimationData *animation = assets.Find(playerAssetId, state);
        if (!animation && state == "fall") animation = assets.Find(playerAssetId, "jump");
        if (!animation) animation = assets.Find(playerAssetId, "idle");
        if (!animation) animation = assets.Find(playerAssetId, "");
        const Rectangle body = player.Bounds();
        const float duck = std::clamp(PlayerDuckBlend(), 0.0f, 1.0f);
        if (duck > 0.0f) {
            const Vector2 feet = SnapToPixels({body.x + body.width / 2, body.y + body.height});
            // Compress the existing sprite around its feet, not its centre. No new art required.
            // The same interpolation that lowers the pig hitbox drives the pose.
            queue.Submit(animation ? animation->layer : Layer::Middle,
                         [this, &resources, &assets, animation, body, feet, duck] {
                rlPushMatrix();
                rlTranslatef(feet.x, feet.y, 0.0f);
                rlScalef(1.0f + 0.10f * duck, 1.0f - 0.72f * duck, 1.0f);
                rlTranslatef(-feet.x, -feet.y, 0.0f);
                if (animation) {
                    assets.Draw(*animation, playerAnimator.Frame(*animation), feet, playerFacing,
                                [&resources](Rectangle b, bool) { art::Student(resources, b); });
                } else {
                    art::Student(resources, body);
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
        const auto position = player.Position();
        const auto world = definition.world;
        auto follow = [](float target, float start, float length, float halfView) {
            if (length <= halfView * 2) return start + length / 2;
            return std::clamp(target, start + halfView, start + length - halfView);
        };
        camera.target = SnapToPixels({
            follow(position.x + 15, world.x, world.width, worldViewport.width / (2 * camera.zoom)),
            follow(position.y + 22, world.y, world.height, worldViewport.height / (2 * camera.zoom))
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
        SubmitPlayer(queue, resources);
        queue.Flush();
        // Drawn after the queue flushes, still inside the camera
        // transform, so the bar sits in world space above the player
        // and is not painted over by the backdrop and sprites.
        DrawStaminaBar(resources, config);
        EndMode2D();
        EndScissorMode();
        DrawTextFit(resources, definition.title, {40, 26, 1380, 44}, 36, definition.accent);
        DrawTextFit(resources, "A/D: move   Space: jump   W/UP: climb   Esc: pause   R: restart", {40, 88, 1480, 30}, 24, RAYWHITE);
        dialogue.Draw(resources, {40, 714, 1520, 162}, currentCue.speaker.c_str(), currentCue.fontSize,
                      currentCue.allowSkip, currentCue.allowReplay);
        DrawTextFit(resources, definition.description, {40, 678, 1500, 28}, 22, Color{173, 165, 190, 255});
    }

    void Level::DrawStaminaBar(const Resources &resources, const GameConfig &config) const {
        // Stamina bar above the player: green above 60%, yellow above
        // 30%, red below that, on a dark background.
        (void)resources;
        const Rectangle playerBounds = player.Bounds();
        const float barWidth = 40.0f;
        const float barHeight = 6.0f;
        const float barX = playerBounds.x + (playerBounds.width - barWidth) / 2.0f;
        const float barY = playerBounds.y - 12.0f;
        const float staminaPercent = player.StaminaPercent(config.maxStamina);
        Color staminaColor = GREEN;
        if (staminaPercent < 0.3f) {
            staminaColor = RED;
        } else if (staminaPercent < 0.6f) {
            staminaColor = YELLOW;
        }
        DrawRectangle(static_cast<int>(barX), static_cast<int>(barY), static_cast<int>(barWidth),
                      static_cast<int>(barHeight), DARKGRAY);
        DrawRectangle(static_cast<int>(barX), static_cast<int>(barY),
                      static_cast<int>(barWidth * staminaPercent), static_cast<int>(barHeight), staminaColor);
    }
}
