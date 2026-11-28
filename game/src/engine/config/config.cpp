#include "engine/config/config.h"
#include "raylib.h"

#include <charconv>
#include <algorithm>
#include <limits>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <locale>
#include <set>
#include <sstream>
#include <string_view>

namespace rage {
    namespace {
        struct PlayerSetting {
            std::string_view key;
            float GameConfig::*field;
        };

        // One registry keeps player tuning and serialization in sync.
        constexpr PlayerSetting playerSettings[] = {
            {"collision_offset_x", &GameConfig::collisionOffsetX},
            {"collision_offset_y", &GameConfig::collisionOffsetY},
            {"move_speed", &GameConfig::moveSpeed},
            {"ground_acceleration", &GameConfig::groundAcceleration},
            {"counter_strafe_acceleration", &GameConfig::counterStrafeAcceleration},
            {"ground_friction", &GameConfig::groundFriction},
            {"start_speed", &GameConfig::startSpeed},
            {"jump_speed", &GameConfig::jumpSpeed},
            {"gravity", &GameConfig::gravity},
            {"climb_speed", &GameConfig::climbSpeed},
            {"climb_contact_distance", &GameConfig::climbContactDistance},
            {"bounce_climb_lock", &GameConfig::bounceClimbLock},
            {"standing_width", &GameConfig::standingWidth},
            {"standing_height", &GameConfig::standingHeight},
            {"crouch_height_multiplier", &GameConfig::crouchHeightMultiplier},
            {"crouch_speed_multiplier", &GameConfig::crouchSpeedMultiplier},
            {"crouch_jump_multiplier", &GameConfig::crouchJumpMultiplier},
            {"max_stamina", &GameConfig::maxStamina},
            {"climb_stamina_drain", &GameConfig::climbStaminaDrain},
            {"jump_stamina_cost", &GameConfig::jumpStaminaCost},
            {"crouch_jump_stamina_cost", &GameConfig::crouchJumpStaminaCost},
            {"crouch_stamina_drain", &GameConfig::crouchStaminaDrain},
            {"stamina_regen", &GameConfig::staminaRegen},
            {"low_stamina_threshold", &GameConfig::lowStaminaThreshold},
            {"exhaustion_recovery_threshold", &GameConfig::exhaustionRecoveryThreshold},
            {"exhausted_speed_multiplier", &GameConfig::exhaustedSpeedMultiplier},
        };

        std::string Trim(std::string text) {
            const auto first = text.find_first_not_of(" \r\n");
            if (first == std::string::npos) return {};
            return text.substr(first, text.find_last_not_of(" \r\n") - first + 1);
        }

        bool Number(const std::string &text, int &output) {
            int value = 0;
            const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
            if (result.ec != std::errc{} || result.ptr != text.data() + text.size()) return false;
            output = value;
            return true;
        }

        bool Number(const std::string &text, float &output) {
            std::istringstream input(text);
            input.imbue(std::locale::classic());
            float value = 0.0f;
            if (!(input >> value) || !input.eof() || !std::isfinite(value)) return false;
            output = value;
            return true;
        }

        bool Volume(const std::string &text, float &output) {
            float value;
            if (!Number(text, value) || value < 0 || value > 1) return false;
            output = value;
            return true;
        }

        bool Boolean(const std::string &text, bool &output) {
            if (text != "true" && text != "false") return false;
            output = text == "true";
            return true;
        }
    }

    bool ConfigStore::Load(const std::filesystem::path &path, bool optional, bool preferencesOnly) {
        std::ifstream file(path);
        if (!file) {
            if (!optional) diagnostics.push_back("Cannot read " + path.string() + "; using defaults.");
            return false;
        }
        std::string section;
        std::string line;
        std::set<std::string> seen;
        int lineNumber = 0;
        bool valid = true;
        while (std::getline(file, line)) {
            ++lineNumber;
            if (lineNumber == 1 && line.starts_with("\xEF\xBB\xBF")) line.erase(0, 3);
            // Comments must be separated from a scalar by whitespace, as in YAML.
            const auto comment = line.find('#');
            if (comment != std::string::npos && (comment == 0 || line[comment - 1] == ' ')) line.erase(comment);
            if (Trim(line).empty()) continue;
            const auto indent = line.find_first_not_of(' ');
            const auto colon = line.find(':');
            auto reject = [&]() {
                diagnostics.push_back(path.filename().string() + ":" + std::to_string(lineNumber) + ": ignored invalid/unknown setting.");
                valid = false;
            };
            if (line.find('\t') != std::string::npos || colon == std::string::npos || (indent != 0 && indent != 2)) {
                section.clear();
                reject();
                continue;
            }
            const std::string key = Trim(line.substr(indent, colon - indent));
            const std::string value = Trim(line.substr(colon + 1));
            if (indent == 0) {
                section = key;
                if (!value.empty() || (key != "window" && key != "audio" && key != "player" && key != "dialogue" && key != "session" && key != "debug")) {
                    section.clear();
                    reject();
                }
                continue;
            }
            const std::string fullKey = section + "." + key;
            // Old settings files included gameplay values. They must not mask tuning edits.
            const auto kind = preferencesOnly ? ConfigFile::Settings : fileKind;
            if (kind == ConfigFile::Settings && (section == "player" || section == "debug" || section == "session")) continue;
            if (kind == ConfigFile::Tuning && section != "player" && section != "debug") continue;
            if (kind == ConfigFile::SaveData && section != "session") continue;
            if (section.empty() || !seen.insert(fullKey).second) {
                reject();
                continue;
            }
            bool accepted = false;
            if (fullKey == "window.width") accepted = Number(value, values.windowWidth);
            else if (fullKey == "window.height") accepted = Number(value, values.windowHeight);
            else if (fullKey == "window.target_fps") accepted = Number(value, values.targetFps);
            else if (fullKey == "window.unlimited_fps") accepted = Boolean(value, values.unlimitedFps);
            else if (fullKey == "window.show_fps") accepted = Boolean(value, values.showFps);
            else if (fullKey == "audio.master_volume") accepted = Volume(value, values.masterVolume);
            else if (fullKey == "audio.muted") accepted = Boolean(value, values.muted);
            else if (fullKey == "audio.music_volume") accepted = Volume(value, values.musicVolume);
            else if (fullKey == "audio.sfx_volume") accepted = Volume(value, values.sfxVolume);
            else if (fullKey == "debug.show_hitboxes") accepted = Boolean(value, values.showHitboxes);
            else if (fullKey == "player.max_health") accepted = Number(value, values.maxHealth);
            else if (section == "player") {
                for (const auto &setting: playerSettings) {
                    if (setting.key == key) {
                        accepted = Number(value, values.*(setting.field));
                        break;
                    }
                }
            }
            else if (fullKey == "dialogue.characters_per_second") accepted = Number(value, values.charactersPerSecond);
            else if (fullKey == "session.last_level") accepted = Number(value, values.lastLevel);
            if (!accepted) reject();
        }
        if (file.bad()) {
            diagnostics.push_back("Read failed: " + path.string());
            return false;
        }
        return valid;
    }

    bool ConfigStore::Save(const std::filesystem::path &path, bool preferencesOnly) {
        std::error_code error;
        if (!path.parent_path().empty()) std::filesystem::create_directories(path.parent_path(), error);
        if (error) {
            diagnostics.push_back("Cannot create settings directory: " + error.message());
            return false;
        }
        std::ofstream file(path);
        file.imbue(std::locale::classic());
        const auto kind = preferencesOnly ? ConfigFile::Settings : fileKind;
        file << std::setprecision(std::numeric_limits<float>::max_digits10);
        if (kind == ConfigFile::All || kind == ConfigFile::Settings) {
            file << "# Player preferences\nwindow:\n  width: " << values.windowWidth << "\n  height: " << values.windowHeight
                 << "\n  target_fps: " << values.targetFps << "\n  unlimited_fps: " << (values.unlimitedFps ? "true" : "false")
                 << "\n  show_fps: " << (values.showFps ? "true" : "false")
                 << "\naudio:\n  master_volume: " << std::clamp(values.masterVolume, 0.0f, 1.0f)
                 << "\n  muted: " << (values.muted ? "true" : "false")
                 << "\n  music_volume: " << std::clamp(values.musicVolume, 0.0f, 1.0f)
                 << "\n  sfx_volume: " << std::clamp(values.sfxVolume, 0.0f, 1.0f)
                 << "\ndialogue:\n  characters_per_second: " << values.charactersPerSecond << '\n';
        }
        if (kind == ConfigFile::All || kind == ConfigFile::Tuning) {
            file << "player:\n  max_health: " << values.maxHealth;
            for (const auto &setting : playerSettings) file << "\n  " << setting.key << ": " << values.*(setting.field);
            file << "\ndebug:\n  show_hitboxes: " << (values.showHitboxes ? "true" : "false") << '\n';
        }
        if (kind == ConfigFile::All || kind == ConfigFile::SaveData)
            file << "session:\n  last_level: " << values.lastLevel << '\n';
        file.flush();
        if (!file) {
            diagnostics.push_back("Cannot save settings: " + path.string());
            return false;
        }
        return true;
    }

    bool ConfigStore::LoadFile(const std::filesystem::path &path, ConfigFile kind, bool optional) {
        fileKind = kind;
        const bool loaded = Load(path, optional);
        fileKind = ConfigFile::All;
        return loaded;
    }

    bool ConfigStore::SaveFile(const std::filesystem::path &path, ConfigFile kind) {
        fileKind = kind;
        const bool saved = Save(path);
        fileKind = ConfigFile::All;
        return saved;
    }

    std::filesystem::path ConfigStore::DefaultGamePath() {
        return std::filesystem::path(GetApplicationDirectory()) / "game.yaml";
    }

    std::filesystem::path ConfigStore::DefaultSettingsPath() {
        return std::filesystem::path(GetApplicationDirectory()) / "settings.yaml";
    }
}
