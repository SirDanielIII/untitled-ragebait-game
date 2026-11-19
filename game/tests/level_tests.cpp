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
    scene.Say({.text = "Locked message", .allowSkip = false, .allowReplay = false}, DialoguePolicy::Interrupt);
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
    auto expectedMethod = DialogueRevealMethod::Natural;
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
            return notice.topic == dialogueEvents::revealCompleted && notice.sourceId == watched
                && notice.revealMethod == expectedMethod;
        },
        .repeatable = true
    });

    scene.Reset(config);
    const auto intro = scene.CurrentDialogueId();
    Check(scene.Say({.text = "Busy"}) == 0 && scene.CurrentDialogueId() == intro, "Unrelated request replaced intro");
    watched = scene.Say({.text = "abcd", .startDelay = 0.5f}, DialoguePolicy::Interrupt);
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
    const auto replacement = scene.Say({.text = "abcd", .startDelay = 1.0f}, DialoguePolicy::Interrupt);
    scene.Update(0, {.completeDialogue = true}, config);
    Check(replacement != watched && starts == 1 && completions == 1, "Identical replacement text published milestones for the wrong message");
    expectedMethod = DialogueRevealMethod::Instant;
    watched = scene.Say({.text = "abcd", .speedMultiplier = 0, .startDelay = 1.0f});
    Check(scene.Say({.text = "Too soon"}) == 0, "An instant cue was overwritten within the same frame");
    scene.Update(0, {}, config);
    Check(starts == 2 && completions == 2, "Instant reveal did not publish both milestones");

    expectedMethod = DialogueRevealMethod::Enter;
    watched = scene.Say({.text = "Read me", .startDelay = 1});
    scene.Update(0, {.completeDialogue = true}, config);
    Check(starts == 3 && completions == 3, "Enter during the delay did not publish manual completion");

    watched = scene.Say({.text = "Pending", .speedMultiplier = 0});
    expectedMethod = DialogueRevealMethod::Instant;
    scene.Reset(config);
    scene.Update(0, {}, config);
    Check(scene.CurrentDialogueId() == intro && starts == 3 && completions == 3,
          "Reset retained dialogue IDs or undelivered notifications");
}

void TestDialogueEffects()
{
    GameConfig config;
    config.charactersPerSecond = 4;
    ScriptedTestLevel scene(Level0Definition());
    scene.Reset(config);
    const auto id = scene.Say({.text = "abcd",
        .effects = {.shakePixels = 3, .shakeHz = 1, .holdSeconds = 1, .fadeSeconds = 2}}, DialoguePolicy::Interrupt);
    scene.Update(0.25f, {}, config);
    Check(test::Near(scene.Dialogue().TextOffset().x, 3), "Authored shake amplitude or rate was ignored");
    scene.Update(1.75f, {}, config);
    Check(scene.Dialogue().IsComplete() && scene.Dialogue().Opacity() == 1, "Hold did not begin at reveal completion");
    scene.Update(1, {}, config);
    Check(test::Near(scene.Dialogue().Opacity(), 0.5f) && scene.Say({.text = "Busy"}) == 0,
          "The fade was interrupted or did not use the authored duration");
    scene.Update(1, {}, config);
    Check(scene.Dialogue().Opacity() == 0, "Text remained visible after fading");
    scene.Update(0, {.replayDialogue = true}, config);
    Check(scene.CurrentDialogueId() == id && scene.Dialogue().Opacity() == 1 && scene.Dialogue().VisibleBytes() == 0,
          "T did not restore the faded cue");
    scene.Update(4, {}, config);
    Check(scene.Dialogue().Opacity() == 0 && scene.Say({.text = "Next"}) != 0,
          "Replayed fade depends on frame partition or never released the dialogue slot");
}

void TestIdleTrap()
{
    GameConfig config;
    config.charactersPerSecond = 8;
    Game game(config);
    game.Update(0, ClickAction(game, ActionKind::StartLevel));
    const auto& level = *game.ActiveLevel();
    const auto intro = level.CurrentDialogueId();
    game.Update(9, {.completeDialogue = true});
    game.Update(0.1f, {.move = 1});
    game.Update(1, {});   // let the player finish sliding
    game.Update(9, {});
    game.Update(0, {.pause = true});
    game.Update(20, {});
    game.Update(0, {.pause = true});
    game.Update(0.75f, {});
    Check(level.CurrentDialogueId() == intro, "Movement or pause counted towards the idle threshold");
    game.Update(0.25f, {});
    const auto taunt = level.CurrentDialogueId();
    Check(taunt != intro && !level.Dialogue().IsComplete(), "Ten idle seconds did not start the taunt");
    const float spawn = level.Definition().spawn.x;
    game.Update(0.1f, {.move = 1});
    Check(level.GetPlayer().Position().x > spawn && level.CurrentDialogueId() == taunt,
          "Movement sprung the trap before its taunt was revealed");
    game.Update(0, {.completeDialogue = true});
    game.Update(0.1f, {.move = 1});
    const auto punchline = level.CurrentDialogueId();
    Check(level.GetPlayer().Position().x == spawn && punchline != taunt, "Revealed taunt did not arm the trap");
    game.Update(0, {.completeDialogue = true});
    game.Update(2.75f, {});
    Check(test::Near(level.Dialogue().Opacity(), 0.5f), "Final quip did not hold and fade");
    game.Update(0, {.pause = true});
    game.Update(20, {});
    game.Update(0, {.pause = true});
    Check(test::Near(level.Dialogue().Opacity(), 0.5f), "Fade advanced while paused");
    game.Update(1, {});
    game.Update(0, {.replayDialogue = true});
    game.Update(0.1f, {.move = 1, .completeDialogue = true});
    Check(level.GetPlayer().Position().x > spawn && level.CurrentDialogueId() == punchline && level.Dialogue().Opacity() == 1,
          "Replaying the faded punchline retriggered the trap or failed to restore the message");
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
    TestDialogueEffects();
    TestIdleTrap();
    TestHallwaySequence();
    std::cout << "PASS: dialogue milestones, effects, idle trap and hallway sequence\n";
}
}
