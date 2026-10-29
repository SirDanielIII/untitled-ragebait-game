#include "ui/text/text.h"
#include "ui/dialogue/dialogue.h"
#include <algorithm>

namespace rage {
    void DrawTextFit(const Resources &resources, const std::string &text, Rectangle bounds, float size, Color color) {
        size = std::min(size, bounds.height);
        while (size > 10 && resources.MeasureText(text, size, 1).x > bounds.width) size -= 1;
        resources.DrawText(text, {bounds.x, bounds.y}, size, 1, color);
    }

    void DrawParagraph(const Resources &resources, const std::string &text, Rectangle bounds, float size, Color color) {
        std::vector<TextLine> lines;
        while (true) {
            lines = WrapText(text, bounds.width, [&](std::string_view value) {
                return resources.MeasureText(value, size, 1).x;
            });
            if (size <= 14 || lines.size() * (size + 8) <= bounds.height) break;
            size -= 1;
        }
        for (std::size_t i = 0; i < lines.size(); ++i)
            resources.DrawText(text.substr(lines[i].begin, lines[i].end - lines[i].begin).c_str(),
                               {bounds.x, bounds.y + i * (size + 8)}, size, 1, color);
    }
}
