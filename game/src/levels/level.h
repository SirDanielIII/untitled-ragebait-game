#pragma once

#include "raylib.h"
#include "engine/resources/asset_paths.h"
#include <vector>
#include <string>
#include "player/player.h"
#include "ui/dialogue/dialogue.h"
#include "engine/resources/resources.h"
#include <memory>

namespace rage
{
// Explicit values keep existing saved settings compatible.
enum class LevelId { Level0 = 0, Level1 = 1, Level2 = 2, Level3 = 3 };
struct DialogueCue
{
    std::string text;
    std::string speaker = "The Entity";
    float speedMultiplier = 1.0f; // Multiplies the player's preference; 0 = instant.
    float startDelay = 0.0f;
    float fontSize = 24.0f;
    bool allowSkip = true;
    bool allowReplay = true;
};
// Content is data; behavior belongs in the corresponding Level subclass.
struct LevelDefinition
{
    LevelId id;
    const char* title;
    const char* description;
    const char* plannedContent;
    DialogueCue intro;
    Color accent;
    ArtId backdrop;
    Rectangle world;
    Vector2 spawn;
    std::vector<Rectangle> platforms;
};

class Level
{
public:
    explicit Level(const LevelDefinition& definition);
    virtual ~Level() = default;
    void Reset(const GameConfig& config);
    void Update(float dt, const InputFrame& input, const GameConfig& config);
    void Draw(const Resources& resources) const;
    const LevelDefinition& Definition() const { return definition; }
    const Player& GetPlayer() const { return player; }
    const Typewriter& Dialogue() const { return dialogue.writer; }
    const Camera2D& Camera() const { return camera; }
    float ElapsedSeconds() const { return elapsedSeconds; }

protected:
    void Say(DialogueCue cue);
    virtual void OnReset() {}
    virtual void OnUpdate(float, const InputFrame&) {}
    virtual void DrawWorld(const Resources&) const {}

private:
    void UpdateCamera();
    void ApplyDialogueSpeed();
    const LevelDefinition& definition;
    Player player;
    DialogueBox dialogue;
    Camera2D camera = {};
    DialogueCue currentCue;
    float dialogueDelay = 0;
    float preferredTextSpeed = 40;
    float elapsedSeconds = 0;
};

// Level list and factories are implemented in main.cpp, beside the level handler.
const std::vector<LevelDefinition>& LevelCatalog();
const LevelDefinition& FindLevel(LevelId id);
// Constructs and resets a level; Game::Apply replaces the active level.
std::unique_ptr<Level> CreateLevel(LevelId id, const GameConfig& config);

// Each definition/factory pair lives in its own level file.
const LevelDefinition& Level0Definition();
const LevelDefinition& Level1Definition();
const LevelDefinition& Level2Definition();
const LevelDefinition& Level3Definition();
std::unique_ptr<Level> MakeLevel0();
std::unique_ptr<Level> MakeLevel1();
std::unique_ptr<Level> MakeLevel2();
std::unique_ptr<Level> MakeLevel3();
}
