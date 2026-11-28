#pragma once

#include "raylib.h"
#include "engine/assets/animator.h"
#include "engine/assets/render_queue.h"
#include "engine/resources/asset_paths.h"
#include "levels/event_manager.h"
#include "objects/object_manager.h"
#include <vector>
#include <string>
#include <functional>
#include <string_view>
#include "player/player.h"
#include "player/jump_particles.h"
#include "ui/dialogue/dialogue.h"
#include "engine/resources/resources.h"
#include <memory>

namespace rage {
    // Game Data Asset ID for the player. Its animations are <name>_idle, _walk, _jump and _fall
    // (e.g. player_walk); see docs/ART_ASSETS.md.
    inline constexpr const char *playerAssetId = "player_character";

    // Audio Data Sound IDs the level requests when the player's state changes.
    namespace playerSounds {
        inline constexpr const char *jump = "player_jump";
        inline constexpr const char *land = "player_land";
        inline constexpr const char *hurt = "player_hurt";
        inline constexpr const char *death = "player_death";
    }

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
    // One line of a level's sprites file: the object named `thing` is drawn with the
    // Game Data sprite whose Asset ID is `assetId`. A `thing` ending in '*' names every
    // object starting with the rest ("car-*"). See level_<n>/level_<n>_sprites.cpp.
    struct SpriteAssignment {
        std::string thing;
        std::string assetId;
    };

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
        bool sideBoundaries = true;
        bool ceilingBoundary = true;
        bool bottomKillZone = true;
        // Asset IDs for the objects above, from the level's sprites file. These
        // replace an object's own .assetId when the level loads.
        std::vector<SpriteAssignment> sprites;
    };

    class Level {
    public:
        explicit Level(const LevelDefinition &definition);

        virtual ~Level() = default;

        void Reset(const GameConfig &config);

        void Update(float dt, const InputFrame &input, const GameConfig &config);

        void DrawScene(const Resources &resources, const GameConfig &config) const;
        void DrawUi(const Resources &resources, const GameConfig &config) const;

        const LevelDefinition &Definition() const { return definition; }
        // Names in the level's sprites file that match no object (typos). The game prints
        // them when it creates the level.
        const std::vector<std::string> &UnmatchedSprites() const { return unmatchedSprites; }
        bool CanClimb() const { return definition.id == LevelId::Level2; }
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

        // Sound IDs requested since the last call, oldest first. The level never plays audio
        // itself: GameWindow takes these each frame and plays them (see engine/audio/README.md).
        std::vector<std::string> TakeSoundRequests();

        // This level's music: "level_<number>_theme" (an Audio Data Sound ID) unless a level
        // overrides it. Return "" for silence.
        virtual std::string MusicTrack() const;

    protected:
        void Restart();
        // Scripted animation supplies geometry/identity; detection and entry tracking
        // still use the shared system. Call each frame while the shape exists.
        // obstacleSensor marks a non-blocking obstacle for the debug overlay only.
        bool ScriptContact(const std::string &id, Rectangle bounds, CollisionType type = CollisionType::Hazard,
                           bool enabled = true, bool obstacleSensor = false);

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
        // Moves object `id` so its box's top-left is at the top-left of `bounds`. Used to keep
        // an art-only object on a hitbox the level's own script moves (see Level 1's
        // SyncObjects). Returns false when there is no object with that id.
        bool PlaceObject(const std::string &id, Rectangle bounds);
        // Gives object `id` the level's own drawing, used until its sprite exists in Game
        // Data. `draw` gets the object's box. Call it in the level's constructor.
        bool SetStandIn(const std::string &id, std::function<void(Rectangle bounds)> draw);
        bool RemoveObject(const std::string &id) { return objects.Remove(id); }
        bool CanSay() const;
        // Returns 0 if busy. Use CanSay in polling event conditions to retry.
        DialogueMessageId Say(DialogueCue cue, DialoguePolicy policy = DialoguePolicy::WhenAvailable);

        // Asks for a sound to play this frame, for scripted moments in events or OnUpdate,
        // e.g. RequestSound("level1_slide"). Player sounds are requested automatically.
        void RequestSound(std::string soundId);

        // Rounds a drawing position to the nearest canvas pixel, so sprites and the camera
        // never sit between pixels (which makes pixel art shimmer while scrolling).
        // Use it only for drawing; gameplay positions stay exact.
        Vector2 SnapToPixels(Vector2 point) const;

        virtual void OnReset() {
        }

        virtual void OnUpdate(float, const InputFrame &) {
        }

        // Drawing happens in this order: DrawWorld, then the sprites of the level's objects,
        // then DrawWorldFront, then the player. Put scenery that must cover object sprites,
        // such as a tunnel wall that hides cars, in DrawWorldFront; the player stays in front.
        virtual void DrawWorld(const Resources &) const {
        }

        virtual void DrawWorldFront(const Resources &) const {
        }

        virtual void DrawHud(const Resources &) const {
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
        bool CheckHazardsAndTriggers();
        void RefreshColliders();
        void NotifyObject(const GameObject &object, const char *topic);

        bool ApplyDialogueSpeed();
        void PublishDialogueMilestones(DialogueRevealMethod method);

        void UpdatePlayerAnimation(float dt);

        // Requests jump/land/hurt/death sounds by comparing the player with the last frame.
        void UpdatePlayerSounds(float dt);

        void SubmitPlayer(RenderQueue &queue, const Resources &resources) const;

        const LevelDefinition &definition;
        Player player;
        JumpParticles jumpParticles;
        mutable RenderQueue renderQueue; // Flush empties jobs but retains per-layer storage.
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
        CollisionSystem collisions;
        CollisionSystem scriptCollisions;
        std::unordered_map<std::string, Collider> scriptBounds;
        std::vector<Collider> scriptShapes;
        std::vector<CollisionContact> scriptContacts;
        std::vector<CollisionContact> previousScriptContacts;
        std::vector<std::string> unmatchedSprites;
        Rectangle scriptPlayerFrom{};
        Vector2 respawnPoint{};
        bool levelComplete = false;
        std::vector<std::string> soundRequests;
        bool trackingPlayerSounds = false; // False until there is a previous frame to compare with.
        bool wasGrounded = false;
        int lastHealth = 0;
        Vector2 lastPlayerPosition{};
        float lastPlayerSpeed = 0;
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
