#include "ui/text/text.h"
#include "ui/dialogue/dialogue.h"
#include <algorithm>
#include <cmath>

namespace rage {
    void DrawTextFit(const Resources &resources, std::string_view text, Rectangle bounds, float size, Color color, FontRole role) {
        size = std::min(size, bounds.height);
        while (size > 10) {
            const float width = resources.MeasureText(text, size, 1, role).x;
            if (width <= bounds.width) break;
            size = std::max(10.0f, std::min(size - 1, std::floor(size * bounds.width / width)));
        }
        resources.DrawText(text, {bounds.x, bounds.y}, size, 1, color, role);
    }

    void DrawParagraph(const Resources &resources, std::string_view text, Rectangle bounds, float size, Color color, FontRole role) {
        std::vector<TextLine> lines;
        while (true) {
            lines = WrapText(text, bounds.width, [&](std::string_view value) {
                return resources.MeasureText(value, size, 1, role).x;
            });
            if (size <= 14 || lines.size() * (size + 8) <= bounds.height) break;
            size -= 1;
        }
        for (std::size_t i = 0; i < lines.size(); ++i)
            resources.DrawText(text.substr(lines[i].begin, lines[i].end - lines[i].begin),
                               {bounds.x, bounds.y + i * (size + 8)}, size, 1, color, role);
    }
}
