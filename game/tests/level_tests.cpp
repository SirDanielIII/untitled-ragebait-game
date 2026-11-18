#include "test_support.h"
#include <iostream>

namespace rage
{
namespace
{
using test::Check;
using test::ClickAction;

class ScriptedTestLevel final : public Level
{
public:
    explicit ScriptedTestLevel(const LevelDefinition& definition) : Level(definition) {}
    using Level::Events;
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
    const InputFrame skip{.completeDialogue = true};
    scene.Update(0, skip, config);
    Check(scene.Dialogue().IsComplete(), "Skip did not complete the intro");
    scene.Say({.text = "A new message from level code.", .speedMultiplier = 2});
    scene.Update(0.25f, {}, config);
    Check(scene.Dialogue().VisibleBytes() == 4, "Replacement dialogue lost its style");
    const InputFrame replay{.replayDialogue = true};
    scene.Update(0, replay, config);
    Check(scene.Dialogue().VisibleBytes() == 0, "Replay failed to restart text");
    scene.Say({.text = "Locked message", .allowSkip = false, .allowReplay = false});
    scene.Update(0, skip, config);
    Check(scene.Dialogue().VisibleBytes() == 0, "Disabled skip was ignored");
    scene.Update(0.25f, {}, config);
    scene.Update(0, replay, config);
    Check(scene.Dialogue().VisibleBytes() == 2, "Disabled replay was ignored");
    scene.Update(0, {.resetPlayer = true}, config);
    Check(scene.resets == 2 && scene.updates == 0 && scene.ElapsedSeconds() == 0, "R did not reset the whole level");
    config.charactersPerSecond = 0;
    scene.Reset(config);
    Check(scene.Dialogue().IsComplete(), "Instant user preference failed to override the intro delay");
}

void TestDialogueEvents()
{
    GameConfig config;
    config.charactersPerSecond = 8;
    ScriptedTestLevel scene(Level0Definition());
    DialogueMessageId watched = 0;
    int starts = 0;
    int completions = 0;
    scene.Events().RegisterEvent({
        .id = "started",
        .execute = [&] { ++starts; },
        .matchesNotification = [&](const EventNotification& notice) {
            return notice.topic == dialogueEvents::revealStarted && notice.sourceId == watched;
        },
        .repeatable = true
    });
    scene.Events().RegisterEvent({
        .id = "completed",
        .execute = [&] { ++completions; },
        .matchesNotification = [&](const EventNotification& notice) {
            return notice.topic == dialogueEvents::revealCompleted && notice.sourceId == watched;
        },
        .repeatable = true
    });

    scene.Reset(config);
    watched = scene.Say({.text = "abcd", .startDelay = 0.5f});
    scene.Update(0.25f, {}, config);
    Check(starts == 0 && completions == 0, "Dialogue started during its delay");
    scene.Update(0.375f, {}, config);
    Check(starts == 1 && completions == 0 && scene.Dialogue().VisibleText() == "a", "Dialogue start milestone failed");
    scene.Update(0.375f, {}, config);
    Check(completions == 1, "Natural reveal did not publish completion");
    scene.Update(0.5f, {.replayDialogue = true}, config);
    scene.Update(0, {.completeDialogue = true}, config);
    Check(starts == 1 && completions == 1 && scene.CurrentDialogueId() == watched, "Replay published new milestones");

    watched = scene.Say({.text = "abcd", .startDelay = 1.0f});
    const auto replacement = scene.Say({.text = "abcd", .startDelay = 1.0f});
    scene.Update(0, {.completeDialogue = true}, config);
    Check(replacement != watched && starts == 1 && completions == 1, "Identical replacement text published milestones for the wrong message");
    watched = scene.Say({.text = "abcd", .speedMultiplier = 0, .startDelay = 1.0f});
    scene.Update(0, {}, config);
    Check(starts == 2 && completions == 2, "Instant reveal did not publish both milestones");
}

void TestHallwaySequence()
{
    GameConfig config;
    config.charactersPerSecond = 1;
    Game game(config);
    const auto click = [&](ActionKind action, int value = 0) { game.Update(0, ClickAction(game, action, value)); };
    const auto advance = [&](int frames, InputFrame input = {}) {
        for (int frame = 0; frame < frames; ++frame) game.Update(0.1f, input);
    };
    const auto cue = [&] {
        Check(game.ActiveLevel() != nullptr, "Hallway test has no active level");
        return game.ActiveLevel()->CurrentDialogueId();
    };
    const InputFrame skip{.completeDialogue = true};
    click(ActionKind::OpenLevels);
    click(ActionKind::StartLevel, static_cast<int>(LevelId::Level0));
    const auto intro = cue();
    advance(12, {.move = 1});
    Check(cue() == intro, "Hallway cue interrupted the introduction");
    game.Update(0, skip);
    const auto hallway = cue();
    Check(hallway != intro, "The hallway cue did not start after entering the area and revealing the intro");
    advance(25);
    Check(cue() == hallway && !game.ActiveLevel()->Dialogue().IsComplete(), "Follow-up ran before the hallway cue finished");

    game.Update(0, skip);
    advance(8);
    Check(cue() == hallway, "Follow-up ignored its delay");
    game.Update(0, {.pause = true});
    advance(30);
    Check(cue() == hallway, "Follow-up ran while paused");
    click(ActionKind::Resume);
    game.Update(0, {.replayDialogue = true});
    game.Update(0, skip);
    advance(14);
    Check(cue() != hallway, "Follow-up did not run on time after resume and replay");

    game.Update(0, {.resetPlayer = true});
    const auto restartedIntro = cue();
    game.Update(0, skip);
    Check(cue() == restartedIntro, "Restart retained the previous sequence or the hallway cue ignored its location");
    advance(12, {.move = 1});
    Check(cue() != restartedIntro, "Hallway trigger did not rearm on restart");
    game.Update(0, skip); // Leave with a follow-up pending.
    game.Update(0, {.pause = true});
    click(ActionKind::OpenLevels);
    Check(!game.ActiveLevel(), "Leaving the hallway retained the old level");
    click(ActionKind::StartLevel, static_cast<int>(LevelId::Level0));
    const auto reloadedIntro = cue();
    game.Update(0, skip);
    advance(25);
    Check(cue() == reloadedIntro, "Reloaded level inherited a pending hallway follow-up");
}
}
void TestLevelAuthoring()
{
    TestLevelScripts();
    TestDialogueEvents();
    TestHallwaySequence();
    std::cout << "PASS: level hooks, dialogue milestones and the hallway event sequence\n";
}
}
