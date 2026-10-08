#pragma once

#include "level_catalog.h"
#include "../entities/player.h"
#include "../dialogue/dialogue_box.h"

namespace rage
{
class LevelScene
{
public:
    explicit LevelScene(const LevelDefinition& definition, const GameConfig& config);
    void Reset(const GameConfig& config);
    void Update(float dt, const InputFrame& input, const GameConfig& config);
    void Draw(Font font) const;
    const LevelDefinition& Definition() const { return definition; }
    const Player& Student() const { return player; }
    const Typewriter& Dialogue() const { return dialogue.writer; }
    const Camera2D& Camera() const { return camera; }

private:
    void UpdateCamera();
    const LevelDefinition& definition;
    Player player;
    DialogueBox dialogue;
    Camera2D camera = {};
};
}
