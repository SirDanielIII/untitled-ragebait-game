#include "dialogue_box.h"

#include <algorithm>
#include <string>

namespace rage
{
std::vector<TextLine> WrapText(const std::string& text, float width, const TextMeasure& measure)
{
    std::vector<TextLine> lines;
    std::size_t start = 0;
    std::size_t cursor = 0;
    std::size_t space = std::string::npos;
    while (cursor < text.size())
    {
        if (text[cursor] == '\n')
        {
            lines.push_back({start, cursor});
            start = ++cursor;
            space = std::string::npos;
            continue;
        }
        const auto next = NextCharacter(text, cursor);
        if (cursor > start && measure(std::string_view(text).substr(start, next - start)) > width)
        {
            const auto end = space != std::string::npos ? space : cursor;
            lines.push_back({start, end});
            start = space != std::string::npos ? space + 1 : cursor;
            cursor = start;
            space = std::string::npos;
            continue;
        }
        if (text[cursor] == ' ') space = cursor;
        cursor = next;
    }
    lines.push_back({start, text.size()}); // Includes empty messages and trailing blank lines.
    return lines;
}

void DialogueBox::Draw(Font font, Rectangle bounds, const char* speaker, float fontSize) const
{
    DrawRectangleRec(bounds, Color{24, 29, 44, 255});
    DrawRectangleLinesEx(bounds, 2.0f, Color{127, 151, 176, 255});
    DrawTextEx(font, speaker, {bounds.x + 18, bounds.y + 12}, 18, 1, Color{242, 197, 112, 255});
    const float width = std::max(1.0f, bounds.width - 36.0f);
    // Wrap the full message so unrevealed words reserve their final positions.
    const auto lines = WrapText(writer.Text(), width, [&](std::string_view text)
    {
        return MeasureTextEx(font, std::string(text).c_str(), fontSize, 1.0f).x;
    });
    const float lineHeight = fontSize + 7.0f;
    const int capacity = std::max(1, static_cast<int>((bounds.height - 76.0f) / lineHeight));
    int currentLine = 0;
    for (int i = 0; i < static_cast<int>(lines.size()); ++i)
        if (lines[i].begin <= writer.VisibleBytes()) currentLine = i;
    const int first = std::max(0, currentLine - capacity + 1);
    BeginScissorMode(static_cast<int>(bounds.x + 16), static_cast<int>(bounds.y + 40),
                     static_cast<int>(width + 4), static_cast<int>(std::max(lineHeight, bounds.height - 76.0f)));
    for (int i = first; i < std::min(first + capacity, static_cast<int>(lines.size())); ++i)
    {
        const auto line = lines[i];
        if (writer.VisibleBytes() < line.begin) break;
        const auto end = std::min(line.end, writer.VisibleBytes());
        const auto visible = writer.Text().substr(line.begin, end - line.begin);
        DrawTextEx(font, visible.c_str(), {bounds.x + 18, bounds.y + 42 + (i - first) * lineHeight}, fontSize, 1, RAYWHITE);
    }
    EndScissorMode();
    DrawTextEx(font, writer.IsComplete() ? "T: replay message" : "Enter: reveal entire message",
               {bounds.x + 18, bounds.y + bounds.height - 26}, 16, 1, Color{167, 181, 198, 255});
}
}
