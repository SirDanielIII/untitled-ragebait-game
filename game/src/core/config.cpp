#include "config.h"

#include <charconv>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <locale>
#include <set>
#include <sstream>
#include <string_view>

namespace rage
{
namespace
{
std::string Trim(std::string text)
{
    const auto first = text.find_first_not_of(" \r\n");
    if (first == std::string::npos) return {};
    return text.substr(first, text.find_last_not_of(" \r\n") - first + 1);
}

bool Number(const std::string& text, int& output, int low, int high)
{
    int value = 0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size() || value < low || value > high) return false;
    output = value;
    return true;
}

bool Number(const std::string& text, float& output, float low, float high)
{
    std::istringstream input(text);
    input.imbue(std::locale::classic());
    float value = 0.0f;
    if (!(input >> value) || !input.eof() || !std::isfinite(value) || value < low || value > high) return false;
    output = value;
    return true;
}

bool Boolean(const std::string& text, bool& output)
{
    if (text != "true" && text != "false") return false;
    output = text == "true";
    return true;
}
}

bool ConfigStore::Load(const std::filesystem::path& path, bool optional)
{
    std::ifstream file(path);
    if (!file)
    {
        if (!optional) diagnostics.push_back("Cannot read " + path.string() + "; using defaults.");
        return false;
    }
    std::string section;
    std::string line;
    std::set<std::string> seen;
    int lineNumber = 0;
    bool valid = true;
    while (std::getline(file, line))
    {
        ++lineNumber;
        if (lineNumber == 1 && line.starts_with("\xEF\xBB\xBF")) line.erase(0, 3);
        // Comments must be separated from a scalar by whitespace, as in YAML.
        const auto comment = line.find('#');
        if (comment != std::string::npos && (comment == 0 || line[comment - 1] == ' ')) line.erase(comment);
        if (Trim(line).empty()) continue;
        const auto indent = line.find_first_not_of(' ');
        const auto colon = line.find(':');
        auto reject = [&]()
        {
            diagnostics.push_back(path.filename().string() + ":" + std::to_string(lineNumber) + ": ignored invalid/unknown setting.");
            valid = false;
        };
        if (line.find('\t') != std::string::npos || colon == std::string::npos || (indent != 0 && indent != 2))
        {
            section.clear();
            reject();
            continue;
        }
        const std::string key = Trim(line.substr(indent, colon - indent));
        const std::string value = Trim(line.substr(colon + 1));
        if (indent == 0)
        {
            section = key;
            if (!value.empty() || (key != "window" && key != "audio" && key != "player" && key != "dialogue" && key != "session"))
            {
                section.clear();
                reject();
            }
            continue;
        }
        const std::string fullKey = section + "." + key;
        if (section.empty() || !seen.insert(fullKey).second) { reject(); continue; }
        bool accepted = false;
        if (fullKey == "window.width") accepted = Number(value, values.windowWidth, 640, 3840);
        else if (fullKey == "window.height") accepted = Number(value, values.windowHeight, 480, 2160);
        else if (fullKey == "window.target_fps") accepted = Number(value, values.targetFps, 30, 240);
        else if (fullKey == "window.show_fps") accepted = Boolean(value, values.showFps);
        else if (fullKey == "audio.master_volume") accepted = Number(value, values.masterVolume, 0.0f, 1.0f);
        else if (fullKey == "audio.muted") accepted = Boolean(value, values.muted);
        else if (fullKey == "player.move_speed") accepted = Number(value, values.moveSpeed, 50.0f, 800.0f);
        else if (fullKey == "player.jump_speed") accepted = Number(value, values.jumpSpeed, 100.0f, 1200.0f);
        else if (fullKey == "player.gravity") accepted = Number(value, values.gravity, 200.0f, 4000.0f);
        else if (fullKey == "dialogue.characters_per_second") accepted = Number(value, values.charactersPerSecond, 0.0f, 240.0f);
        else if (fullKey == "session.last_level") accepted = Number(value, values.lastLevel, 0, 3);
        if (!accepted) reject();
    }
    if (file.bad()) { diagnostics.push_back("Read failed: " + path.string()); return false; }
    return valid;
}

bool ConfigStore::Save(const std::filesystem::path& path)
{
    std::error_code error;
    if (!path.parent_path().empty()) std::filesystem::create_directories(path.parent_path(), error);
    if (error) { diagnostics.push_back("Cannot create settings directory: " + error.message()); return false; }
    std::ofstream file(path);
    file.imbue(std::locale::classic());
    file << std::setprecision(6)
         << "# User preferences; no gameplay progress or unlocks are stored.\n"
         << "window:\n  width: " << values.windowWidth << "\n  height: " << values.windowHeight
         << "\n  target_fps: " << values.targetFps << "\n  show_fps: " << (values.showFps ? "true" : "false")
         << "\naudio:\n  master_volume: " << values.masterVolume << "\n  muted: " << (values.muted ? "true" : "false")
         << "\nplayer:\n  move_speed: " << values.moveSpeed << "\n  jump_speed: " << values.jumpSpeed << "\n  gravity: " << values.gravity
         << "\ndialogue:\n  characters_per_second: " << values.charactersPerSecond
         << "\nsession:\n  last_level: " << values.lastLevel << '\n';
    file.flush();
    if (!file) { diagnostics.push_back("Cannot save settings: " + path.string()); return false; }
    return true;
}

std::filesystem::path ConfigStore::DefaultSettingsPath()
{
#if defined(_WIN32)
    if (const char* base = std::getenv("LOCALAPPDATA")) return std::filesystem::path(base) / "UntitledRagebaitGame" / "settings.yaml";
#else
    if (const char* base = std::getenv("XDG_DATA_HOME")) return std::filesystem::path(base) / "untitled_ragebait_game" / "settings.yaml";
    if (const char* base = std::getenv("HOME")) return std::filesystem::path(base) / ".local/share/untitled_ragebait_game/settings.yaml";
#endif
    return std::filesystem::temp_directory_path() / "untitled_ragebait_game/settings.yaml";
}
}
