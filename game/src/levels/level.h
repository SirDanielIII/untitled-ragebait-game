#pragma once

#include "raylib.h"
#include "engine/assets/animator.h"
#include "engine/assets/render_queue.h"
#include "engine/resources/asset_paths.h"
#include "levels/event_manager.h"
#include "objects/object_manager.h"
#include <vector>
#include <string>
#include "player/player.h"
#include "ui/dialogue/dialogue.h"
#include "engine/resources/resources.h"
#include <memory>

namespace rage {
    // Game Data Asset ID for the player. Its animations are <name>_idle, _walk, _jump and _fall
    // (e.g. player_walk); see docs/ART_ASSETS.md.
    inline constexpr const char *playerAssetId = "player_character";

    // Explicit values keep existing saved settings compatible.
    enum class LevelId { Level0 = 0, Level1 = 1, Level2 = 2, Level3 = 3 };

    using DialogueMessageId = std::uint64_t;
    namespace dialogueEvents {
        inline constexpr char revealStarted[] = "dialogue.reveal_started";
        inline constexpr char revealCompleted[] = "dialogue.reveal_completed";
    }

    struct DialogueCue {
        std::string text;
        std::string speaker = "The Entity";
        float speedMultiplier = 1.0f; // Multiplies the player's preference; 0 = instant.
        float startDelay = 0.0f;
        float fontSize = 24.0f;
        bool allowSkip = true;
        bool allowReplay = true;
        DialogueEffects effects = {};
        DialogueMessageId id = 0; // Assigned by Say for this request in this level run.
    };

    enum class DialoguePolicy { WhenAvailable, Interrupt };

    // Content is data; behavior belongs in the corresponding Level subclass.
    struct LevelDefinition {
        LevelId id;
        const char *title;
        const char *description;
        const char *plannedContent;
        DialogueCue intro;
        Color accent;
        ArtId backdrop;
        Rectangle world;
        Vector2 spawn;
        // Every object in the level, as reusable definitions. The
        // Level creates them in its ObjectManager on construction;
        // collision geometry comes from these, not from artwork.
        // (Previously a list of platform rectangles; see objects/.)
        std::vector<GameObjectDefinition> objects;
    };

    class Level {
    public:
        explicit Level(const LevelDefinition &definition);

        virtual ~Level() = default;

        void Reset(const GameConfig &config);

        void Update(float dt, const InputFrame &input, const GameConfig &config);

        void Draw(const Resources &resources) const;

        const LevelDefinition &Definition() const { return definition; }
        const Player &GetPlayer() const { return player; }
        const Typewriter &Dialogue() const { return dialogue.writer; }
        DialogueMessageId CurrentDialogueId() const { return currentCue.id; }
        const Camera2D &Camera() const { return camera; }
        float ElapsedSeconds() const { return elapsedSeconds; }
        const Animator &PlayerAnimator() const { return playerAnimator; }
        Facing PlayerFacing() const { return playerFacing; }
        ObjectManager &Objects() { return objects; }
        const ObjectManager &Objects() const { return objects; }
        bool IsComplete() const { return levelComplete; }
        Vector2 RespawnPoint() const { return respawnPoint; }

    protected:
        EventManager &Events() { return events; }
        Player &MutablePlayer() { return player; }
        // Event-facing object actions. These are the consistent
        // interface events use (see ObjectManager): they change the
        // object and post the matching objectEvents notification, so
        // other events can react to it in the next update.
        bool ActivateObject(const std::string &id);
        bool DeactivateObject(const std::string &id);
        bool StartObjectFall(const std::string &id);
        GameObject *SpawnObject(const GameObjectDefinition &definition) { return objects.Spawn(definition); }
        bool RemoveObject(const std::string &id) { return objects.Remove(id); }
        bool CanSay() const;
        // Returns 0 if busy. Use CanSay in polling event conditions to retry.
        DialogueMessageId Say(DialogueCue cue, DialoguePolicy policy = DialoguePolicy::WhenAvailable);

        // Rounds a drawing position to the nearest canvas pixel, so sprites and the camera
        // never sit between pixels (which makes pixel art shimmer while scrolling).
        // Use it only for drawing; gameplay positions stay exact.
        Vector2 SnapToPixels(Vector2 point) const;

        virtual void OnReset() {
        }

        virtual void OnUpdate(float, const InputFrame &) {
        }

        virtual void DrawWorld(const Resources &) const {
        }

        // Queue this level's Game Data sprites (hazards, props, effects). Each is drawn on its
        // configured layer; Middle-layer sprites appear behind the player.
        // The base implementation queues the level's game objects;
        // subclasses add their own sprites on top of it.
        virtual void SubmitSprites(RenderQueue &queue, const Resources &resources) const;

        // Called when the player enters a trigger object. The base
        // version handles the id conventions: "checkpoint-*" sets the
        // respawn point, "exit-*" completes the level. Subclasses can
        // override it for stage-specific triggers.
        virtual void OnTrigger(GameObject &object);

    private:
        void UpdateCamera();
        void CheckHazardsAndTriggers();
        void NotifyObject(const GameObject &object, const char *topic);

        bool ApplyDialogueSpeed();
        void PublishDialogueMilestones(DialogueRevealMethod method);

        void UpdatePlayerAnimation(float dt);

        void SubmitPlayer(RenderQueue &queue, const Resources &resources) const;

        const LevelDefinition &definition;
        Player player;
        DialogueBox dialogue;
        Camera2D camera = {};
        DialogueCue currentCue;
        bool requestedThisFrame = false;
        bool revealStarted = false;
        bool revealCompleted = false;
        float dialogueDelay = 0;
        float preferredTextSpeed = 40;
        float elapsedSeconds = 0;
        Animator playerAnimator;
        Facing playerFacing = Facing::Right;
        EventManager events;
        ObjectManager objects;
        Vector2 respawnPoint{};
        bool levelComplete = false;
    };

    // Level list and factories are implemented in main.cpp, beside the level handler.
    const std::vector<LevelDefinition> &LevelCatalog();

    const LevelDefinition &FindLevel(LevelId id);

    // Constructs and resets a level; Game::Apply replaces the active level.
    std::unique_ptr<Level> CreateLevel(LevelId id, const GameConfig &config);

    // Each definition/factory pair lives in its own level file.
    const LevelDefinition &Level0Definition();

    const LevelDefinition &Level1Definition();

    const LevelDefinition &Level2Definition();

    const LevelDefinition &Level3Definition();

    std::unique_ptr<Level> MakeLevel0();

    std::unique_ptr<Level> MakeLevel1();

    std::unique_ptr<Level> MakeLevel2();

    std::unique_ptr<Level> MakeLevel3();
}
