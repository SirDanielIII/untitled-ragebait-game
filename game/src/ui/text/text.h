#pragma once
#include "engine/resources/resources.h"
#include <string_view>

namespace rage {
    // Menu text stays inside its layout when the selected font changes.
    void DrawTextFit(const Resources &resources, std::string_view text, Rectangle bounds, float size, Color color, FontRole role = FontRole::Regular);

    void DrawParagraph(const Resources &resources, std::string_view text, Rectangle bounds, float size, Color color, FontRole role = FontRole::Regular);
}
