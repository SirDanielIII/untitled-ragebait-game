#include "levels/level.h"
#include "engine/graphics/layout.h"
#include "engine/graphics/placeholder_art.h"
#include "ui/text/text.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace rage {
    Level::Level(const LevelDefinition &definition) : definition(definition) {
        camera.offset = {worldViewport.width / 2, worldViewport.y + worldViewport.height / 2};
        camera.zoom = 1.5f;
    }

    void Level::Reset(const GameConfig &config) {
        player.Reset(definition.spawn);
        elapsedSeconds = 0;
        preferredTextSpeed = config.charactersPerSecond;
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
        preferredTextSpeed = config.charactersPerSecond;
        ApplyDialogueSpeed();
        if (input.replayDialogue && currentCue.allowReplay) {
            dialogue.writer.Reset();
            dialogueDelay = preferredTextSpeed == 0 || currentCue.speedMultiplier == 0 ? 0 : currentCue.startDelay;
        }
        if (input.completeDialogue && currentCue.allowSkip) {
            dialogue.writer.Complete();
            dialogueDelay = 0;
        }
        const float revealTime = std::max(0.0f, dt - dialogueDelay);
        dialogueDelay = std::max(0.0f, dialogueDelay - dt);
        dialogue.writer.Update(revealTime);
        player.Update(dt, input, config, definition.platforms, definition.world);
        OnUpdate(dt, input);
        UpdateCamera();
    }

    void Level::ApplyDialogueSpeed() {
        dialogue.writer.SetSpeed(preferredTextSpeed * currentCue.speedMultiplier);
        if (preferredTextSpeed == 0 || currentCue.speedMultiplier == 0) dialogueDelay = 0;
    }

    void Level::Say(DialogueCue cue) {
        currentCue = std::move(cue);
        currentCue.speedMultiplier = std::isfinite(currentCue.speedMultiplier) ? std::clamp(currentCue.speedMultiplier, 0.0f, 10.0f) : 1.0f;
        currentCue.startDelay = std::isfinite(currentCue.startDelay) ? std::max(0.0f, currentCue.startDelay) : 0.0f;
        currentCue.fontSize = std::isfinite(currentCue.fontSize) ? std::clamp(currentCue.fontSize, 12.0f, 48.0f) : 24.0f;
        dialogueDelay = currentCue.startDelay;
        ApplyDialogueSpeed();
        dialogue.writer.Replace(currentCue.text);
    }

    void Level::UpdateCamera() {
        const auto position = player.Position();
        const auto world = definition.world;
        auto follow = [](float target, float start, float length, float halfView) {
            if (length <= halfView * 2) return start + length / 2;
            return std::clamp(target, start + halfView, start + length - halfView);
        };
        camera.target = {
            follow(position.x + 15, world.x, world.width, worldViewport.width / (2 * camera.zoom)),
            follow(position.y + 22, world.y, world.height, worldViewport.height / (2 * camera.zoom))
        };
    }

    void Level::Draw(const Resources &resources) const {
        resources.BeginClip(worldViewport);
        DrawRectangleRec(worldViewport, Color{36, 44, 60, 255});
        BeginMode2D(camera);
        art::LevelBackdrop(resources, definition.backdrop, definition.world, definition.accent);
        DrawWorld(resources);
        for (const auto platform: definition.platforms) art::Platform(resources, platform, definition.accent);
        art::Student(resources, player.Bounds());
        EndMode2D();
        EndScissorMode();
        DrawTextFit(resources, definition.title, {40, 26, 1380, 44}, 36, definition.accent);
        DrawTextFit(resources, "A/D: move   Space: jump   Esc: pause   R: restart", {40, 88, 1480, 30}, 24, RAYWHITE);
        dialogue.Draw(resources, {40, 714, 1520, 162}, currentCue.speaker.c_str(), currentCue.fontSize,
                      currentCue.allowSkip, currentCue.allowReplay);
        DrawTextFit(resources, definition.description, {40, 678, 1500, 28}, 22, Color{173, 165, 190, 255});
    }
}
