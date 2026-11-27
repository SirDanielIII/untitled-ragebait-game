#include "engine/config/config.h"
#include "raylib.h"

#include <charconv>
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
                if (!value.empty() || (key != "window" && key != "audio" && key != "player" && key != "dialogue" && key != "session")) {
                    section.clear();
                    reject();
                }
                continue;
            }
            const std::string fullKey = section + "." + key;
            // Old settings files included gameplay values. They must not mask tuning edits.
            if (preferencesOnly && section == "player") continue;
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
            else if (fullKey == "audio.master_volume") accepted = Number(value, values.masterVolume);
            else if (fullKey == "audio.muted") accepted = Boolean(value, values.muted);
            else if (fullKey == "audio.music_volume") accepted = Number(value, values.musicVolume);
            else if (fullKey == "audio.sfx_volume") accepted = Number(value, values.sfxVolume);
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
        file << std::setprecision(6)
                << "# Settings; no gameplay progress or unlocks are stored.\n"
                << "window:\n  width: " << values.windowWidth << "\n  height: " << values.windowHeight
                << "\n  target_fps: " << values.targetFps << "\n  unlimited_fps: " << (values.unlimitedFps ? "true" : "false")
                << "\n  show_fps: " << (values.showFps ? "true" : "false")
                << "\naudio:\n  master_volume: " << values.masterVolume << "\n  muted: " << (values.muted ? "true" : "false")
                << "\n  music_volume: " << values.musicVolume << "\n  sfx_volume: " << values.sfxVolume;
        if (!preferencesOnly) {
            file << "\nplayer:";
            for (const auto &setting: playerSettings) file << "\n  " << setting.key << ": " << values.*(setting.field);
        }
        file << "\ndialogue:\n  characters_per_second: " << values.charactersPerSecond
                << "\nsession:\n  last_level: " << values.lastLevel << '\n';
        file.flush();
        if (!file) {
            diagnostics.push_back("Cannot save settings: " + path.string());
            return false;
        }
        return true;
    }

    std::filesystem::path ConfigStore::DefaultSettingsPath() {
        return std::filesystem::path(GetApplicationDirectory()) / "settings.yaml";
    }
}
