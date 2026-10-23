#pragma once
#include <array>
#include <cstddef>

namespace rage
{
// Font source: game/resources/fonts/ARCADEPI.TTF. Paths are relative to resources.
inline constexpr const char* uiFontPath = "fonts/ARCADEPI.TTF";

enum class ArtId : std::size_t
{
    MainMenu, Settings, StageSelect, Pause, Student, Entity, Platform,
    Level0, Level1, Level2, Level3, DialoguePanel, Button, Count
};
// Optional exported art: missing files use the placeholder renderer.
// Keep entries in the same order as ArtId. Runtime PNGs go in game/resources/art/.
inline constexpr std::array<const char*, static_cast<std::size_t>(ArtId::Count)> artPaths = {
    "art/menus/main.png", "art/menus/settings.png", "art/menus/stage_select.png", "art/menus/pause.png",
    "art/characters/student.png", "art/characters/entity.png", "art/world/platform.png",
    "art/levels/level_0.png", "art/levels/level_1.png", "art/levels/level_2.png", "art/levels/level_3.png",
    "art/ui/dialogue.png", "art/ui/button.png"
};
enum class ArtFit { Contain, Stretch };
}
