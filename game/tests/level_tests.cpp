#include "levels/level.h"
#include <iostream>
#include <stdexcept>

namespace rage
{
namespace
{
void Check(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}
class ScriptedTestLevel final : public Level
{
public:
    explicit ScriptedTestLevel(const LevelDefinition& definition) : Level(definition) {}
    using Level::Say;
    int resets = 0;
    int updates = 0;
private:
    void OnReset() override { ++resets; updates = 0; }
    void OnUpdate(float, const InputFrame&) override { ++updates; }
};

void TestLevelScripts()
{
    GameConfig config;
    config.charactersPerSecond = 8;
    auto definition = Level0Definition();
    definition.intro = {.text = "abcdefgh", .speedMultiplier = 0.5f, .startDelay = 0.25f};
    ScriptedTestLevel scene(definition);
    scene.Reset(config);
    Check(scene.resets == 1 && scene.Dialogue().VisibleBytes() == 0, "Level reset hook or intro failed");
    scene.Update(0.25f, {}, config);
    Check(scene.Dialogue().VisibleBytes() == 0, "Intro ignored its delay");
    scene.Update(0.5f, {}, config);
    Check(scene.Dialogue().VisibleBytes() == 2, "Level typewriter speed did not multiply user speed");
    Check(scene.updates == 2, "Level update hook was not called once per update");
    InputFrame skip;
    skip.completeDialogue = true;
    scene.Update(0, skip, config);
    Check(scene.Dialogue().IsComplete(), "Skip did not complete the intro");
    scene.Say({.text = "A new message from level code.", .speedMultiplier = 2});
    scene.Update(0.25f, {}, config);
    Check(scene.Dialogue().VisibleBytes() == 4, "Replacement dialogue lost its style");
    InputFrame replay;
    replay.replayDialogue = true;
    scene.Update(0, replay, config);
    Check(scene.Dialogue().VisibleBytes() == 0, "Replay failed to restart text");
    scene.Say({.text = "Locked message", .allowSkip = false, .allowReplay = false});
    scene.Update(0, skip, config);
    Check(scene.Dialogue().VisibleBytes() == 0, "Disabled skip was ignored");
    scene.Update(0.25f, {}, config);
    scene.Update(0, replay, config);
    Check(scene.Dialogue().VisibleBytes() == 2, "Disabled replay was ignored");
    InputFrame restart;
    restart.resetPlayer = true;
    scene.Update(0, restart, config);
    Check(scene.resets == 2 && scene.updates == 0 && scene.ElapsedSeconds() == 0, "R did not reset the whole level");
    config.charactersPerSecond = 0;
    scene.Reset(config);
    Check(scene.Dialogue().IsComplete(), "Instant user preference failed to override the intro delay");
    for (const auto& level : LevelCatalog())
    {
        auto first = CreateLevel(level.id, config);
        auto second = CreateLevel(level.id, config);
        first->Update(0.05f, {}, config);
        Check(first->ElapsedSeconds() > 0 && second->ElapsedSeconds() == 0, "Two scenes shared mutable level state");
    }
}
}
void TestLevelAuthoring()
{
    TestLevelScripts();
    std::cout << "PASS: level reset/update hooks, isolated state and typewriter preferences\n";
}
}
