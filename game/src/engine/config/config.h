#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace rage {
    inline constexpr int minTargetFps = 30;
    inline constexpr int maxTargetFps = 360;

    struct GameConfig {
        int windowWidth = 1600;
        int windowHeight = 900;
        int targetFps = 60;
        bool unlimitedFps = false;
        bool showFps = false;
        bool fullscreen = false;
        float masterVolume = 0.5f;
        bool muted = false;
        float musicVolume = 0.8f; // Music slider; multiplied by the master volume.
        float sfxVolume = 1.0f;   // Sound effects, UI and voice slider.
        bool showHitboxes = false;
        float collisionOffsetX = 0;
        float collisionOffsetY = 0;
        int maxHealth = 1;
        float moveSpeed = 220.0f;
        float groundAcceleration = 1100.0f;
        float counterStrafeAcceleration = 1800.0f;
        float groundFriction = 500.0f;
        float startSpeed = 90.0f;
        float jumpSpeed = 580.0f;
        float gravity = 1750.0f;
        float climbSpeed = 180.0f;
        float climbContactDistance = 2.0f;
        float bounceClimbLock = 0.2f;
        float standingWidth = 30.0f;
        float standingHeight = 44.0f;
        float crouchHeightMultiplier = 0.5f;
        float crouchSpeedMultiplier = 0.2f;
        float crouchJumpMultiplier = 1.2f;
        float charactersPerSecond = 40.0f;
        float maxStamina = 100.0f;
        float climbStaminaDrain = 45.0f;
        float jumpStaminaCost = 18.0f;
        float crouchJumpStaminaCost = 30.0f;
        float crouchStaminaDrain = 2.0f;
        float staminaRegen = 40.0f;
        float lowStaminaThreshold = 0.3f; // Red bar and horizontal slowdown; fraction of max stamina.
        float exhaustionRecoveryThreshold = 0.15f; // Fraction of max stamina.
        float exhaustedSpeedMultiplier = 0.5f;
        int lastLevel = 0;
        int EffectiveTargetFps() const { return unlimitedFps ? 0 : targetFps; }
    };

    // A deliberately small YAML subset: section maps containing plain number/bool scalars.
    // Load overlays valid keys onto the current values; bad/missing keys keep their defaults.
    // Runtime user preferences exclude player tuning so packaged edits always apply.
    enum class ConfigFile { All, Tuning, Settings, SaveData };

    class ConfigStore {
    public:
        GameConfig values;
        std::vector<std::string> diagnostics;

        bool Load(const std::filesystem::path &path, bool optional = false, bool preferencesOnly = false);

        bool Save(const std::filesystem::path &path, bool preferencesOnly = false);

        bool LoadFile(const std::filesystem::path &path, ConfigFile kind, bool optional = false);
        bool SaveFile(const std::filesystem::path &path, ConfigFile kind);
        static std::filesystem::path DefaultSettingsPath();
        static std::filesystem::path DefaultGamePath();
    private:
        ConfigFile fileKind = ConfigFile::All;
    };
}
