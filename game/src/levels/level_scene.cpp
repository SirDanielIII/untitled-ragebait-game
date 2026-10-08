#include "level_scene.h"
#include "../core/layout.h"

#include <algorithm>

namespace rage
{
LevelScene::LevelScene(const LevelDefinition& definition, const GameConfig& config) : definition(definition)
{
    camera.offset = {worldViewport.width / 2, worldViewport.y + worldViewport.height / 2};
    camera.zoom = 1.5f;
    Reset(config);
}

void LevelScene::Reset(const GameConfig& config)
{
    player.Reset(definition.spawn);
    dialogue.writer.SetSpeed(config.charactersPerSecond);
    dialogue.writer.Replace(definition.dialogue);
    UpdateCamera();
}

void LevelScene::Update(float dt, const InputFrame& input, const GameConfig& config)
{
    if (input.resetPlayer) player.Reset(definition.spawn);
    dialogue.writer.SetSpeed(config.charactersPerSecond);
    if (input.replayDialogue) dialogue.writer.Reset();
    if (input.completeDialogue) dialogue.writer.Complete();
    dialogue.writer.Update(dt);
    player.Update(dt, input, config, definition.platforms, definition.world);
    UpdateCamera();
    // Add actual level triggers, hazards and narrator scheduling here when designed.
}

void LevelScene::UpdateCamera()
{
    const auto position = player.Position();
    const auto world = definition.world;
    auto follow = [](float target, float start, float length, float halfView)
    {
        if (length <= halfView * 2) return start + length / 2;
        return std::clamp(target, start + halfView, start + length - halfView);
    };
    camera.target = {follow(position.x + 15, world.x, world.width, worldViewport.width / (2 * camera.zoom)),
                     follow(position.y + 22, world.y, world.height, worldViewport.height / (2 * camera.zoom))};
}

void LevelScene::Draw(Font font) const
{
    BeginScissorMode(static_cast<int>(worldViewport.x), static_cast<int>(worldViewport.y),
                     static_cast<int>(worldViewport.width), static_cast<int>(worldViewport.height));
    DrawRectangleRec(worldViewport, Color{36, 44, 60, 255});
    BeginMode2D(camera);
    for (int x = 0; x < static_cast<int>(definition.world.width); x += 100)
        DrawLine(x, 0, x, static_cast<int>(definition.world.height), Color{46, 55, 73, 255});
    for (int y = 0; y < static_cast<int>(definition.world.height); y += 100)
        DrawLine(0, y, static_cast<int>(definition.world.width), y, Color{46, 55, 73, 255});
    if (definition.id == LevelId::Final)
    {
        DrawRectangle(1200, 300, 280, 350, Color{60, 66, 85, 255});
        DrawRectangleLines(1200, 300, 280, 350, definition.accent);
        DrawTextEx(font, "LECTURE HALL", {1218, 322}, 24, 1, RAYWHITE);
        DrawRectangle(1300, 530, 70, 120, Color{90, 96, 115, 255});
        DrawCircleLines(1100, 545, 30, definition.accent);
        DrawTextEx(font, "Entity: future boss", {1000, 485}, 18, 1, definition.accent);
    }
    for (const auto platform : definition.platforms)
    {
        DrawRectangleRec(platform, Color{62, 73, 88, 255});
        DrawRectangle(static_cast<int>(platform.x), static_cast<int>(platform.y), static_cast<int>(platform.width), 4, definition.accent);
    }
    DrawTextEx(font, "A", {definition.spawn.x, definition.spawn.y - 30}, 20, 1, definition.accent);
    player.Draw();
    EndMode2D();
    EndScissorMode();
    DrawTextEx(font, definition.title, {40, 26}, 36, 1, definition.accent);
    DrawTextEx(font, "A/D or arrows: move   Space/W/Up: jump   Esc/P: pause   R: reset", {40, 88}, 26, 1, RAYWHITE);
    dialogue.Draw(font, {40, 714, 1520, 162}, "The Entity", 24);
    DrawTextEx(font, "Placeholder scene - no hazards, lives, completion or boss mechanics yet", {40, 678}, 22, 1, Color{173, 185, 200, 255});
}
}
