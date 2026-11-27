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
            float low;
            float high;
        };

        // One registry keeps player tuning validation and serialization in sync.
        constexpr PlayerSetting playerSettings[] = {
            {"move_speed", &GameConfig::moveSpeed, 1, 1000},
            {"ground_acceleration", &GameConfig::groundAcceleration, 1, 20000},
            {"counter_strafe_acceleration", &GameConfig::counterStrafeAcceleration, 1, 20000},
            {"ground_friction", &GameConfig::groundFriction, 0, 20000},
            {"start_speed", &GameConfig::startSpeed, 0, 800},
            {"jump_speed", &GameConfig::jumpSpeed, 1, 1500},
            {"gravity", &GameConfig::gravity, 1, 4000},
            {"climb_speed", &GameConfig::climbSpeed, 1, 800},
            {"climb_contact_distance", &GameConfig::climbContactDistance, 0, 20},
            {"bounce_climb_lock", &GameConfig::bounceClimbLock, 0, 5},
            {"standing_width", &GameConfig::standingWidth, 1, 200},
            {"standing_height", &GameConfig::standingHeight, 1, 200},
            {"crouch_height_multiplier", &GameConfig::crouchHeightMultiplier, 0.1f, 1},
            {"crouch_speed_multiplier", &GameConfig::crouchSpeedMultiplier, 0.01f, 1},
            {"crouch_jump_multiplier", &GameConfig::crouchJumpMultiplier, 1, 5},
            {"max_stamina", &GameConfig::maxStamina, 1, 10000},
            {"climb_stamina_drain", &GameConfig::climbStaminaDrain, 0, 10000},
            {"jump_stamina_cost", &GameConfig::jumpStaminaCost, 0, 10000},
            {"crouch_jump_stamina_cost", &GameConfig::crouchJumpStaminaCost, 0, 10000},
            {"crouch_stamina_drain", &GameConfig::crouchStaminaDrain, 0, 10000},
            {"stamina_regen", &GameConfig::staminaRegen, 0, 10000},
            {"low_stamina_threshold", &GameConfig::lowStaminaThreshold, 0, 1},
            {"exhaustion_recovery_threshold", &GameConfig::exhaustionRecoveryThreshold, 0, 1},
            {"exhausted_speed_multiplier", &GameConfig::exhaustedSpeedMultiplier, 0.01f, 1},
        };

        std::string Trim(std::string text) {
            const auto first = text.find_first_not_of(" \r\n");
            if (first == std::string::npos) return {};
            return text.substr(first, text.find_last_not_of(" \r\n") - first + 1);
        }

        bool Number(const std::string &text, int &output, int low, int high) {
            int value = 0;
            const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
            if (result.ec != std::errc{} || result.ptr != text.data() + text.size() || value < low || value > high) return false;
            output = value;
            return true;
        }

        bool Number(const std::string &text, float &output, float low, float high) {
            std::istringstream input(text);
            input.imbue(std::locale::classic());
            float value = 0.0f;
            if (!(input >> value) || !input.eof() || !std::isfinite(value) || value < low || value > high) return false;
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
            if (fullKey == "window.width") accepted = Number(value, values.windowWidth, 640, 3840);
            else if (fullKey == "window.height") accepted = Number(value, values.windowHeight, 480, 2160);
            else if (fullKey == "window.target_fps") accepted = Number(value, values.targetFps, minTargetFps, maxTargetFps);
            else if (fullKey == "window.unlimited_fps") accepted = Boolean(value, values.unlimitedFps);
            else if (fullKey == "window.show_fps") accepted = Boolean(value, values.showFps);
            else if (fullKey == "audio.master_volume") accepted = Number(value, values.masterVolume, 0.0f, 1.0f);
            else if (fullKey == "audio.muted") accepted = Boolean(value, values.muted);
            else if (fullKey == "audio.music_volume") accepted = Number(value, values.musicVolume, 0.0f, 1.0f);
            else if (fullKey == "audio.sfx_volume") accepted = Number(value, values.sfxVolume, 0.0f, 1.0f);
            else if (section == "player") {
                for (const auto &setting: playerSettings) {
                    if (setting.key == key) {
                        accepted = Number(value, values.*(setting.field), setting.low, setting.high);
                        break;
                    }
                }
            }
            else if (fullKey == "dialogue.characters_per_second") accepted = Number(value, values.charactersPerSecond, 0.0f, 240.0f);
            else if (fullKey == "session.last_level") accepted = Number(value, values.lastLevel, 0, 3);
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
