#include "ui/dialogue/dialogue.h"
#include <algorithm>
#include <cmath>
#include <utility>
#include <string>

namespace rage
{
std::size_t NextCharacter(std::string_view text, std::size_t offset)
{
    if (offset >= text.size()) return text.size();
    ++offset;
    while (offset < text.size() && (static_cast<unsigned char>(text[offset]) & 0xC0) == 0x80) ++offset;
    return offset;
}

void Typewriter::Replace(std::string message)
{
    text.clear();
    for (std::size_t i = 0; i < message.size(); ++i)
    {
        if (message[i] == '\r')
        {
            text += '\n';
            if (i + 1 < message.size() && message[i + 1] == '\n') ++i;
        }
        else text += message[i];
    }
    boundaries.clear();
    for (std::size_t i = 0; i < text.size();) { i = NextCharacter(text, i); boundaries.push_back(i); }
    Reset();
}

void Typewriter::Reset()
{
    revealed = 0;
    fraction = 0.0;
    if (speed == 0.0f) Complete();
}

void Typewriter::Complete()
{
    revealed = boundaries.size();
    fraction = 0.0;
}

void Typewriter::SetSpeed(float charactersPerSecond)
{
    speed = std::isfinite(charactersPerSecond) ? std::max(0.0f, charactersPerSecond) : 40.0f;
    if (speed == 0.0f) Complete();
}

void Typewriter::Update(float dt)
{
    if (IsComplete() || !std::isfinite(dt) || dt <= 0.0f) return;
    fraction += static_cast<double>(dt) * speed;
    const double count = std::floor(fraction);
    const auto remaining = boundaries.size() - revealed;
    if (count >= static_cast<double>(remaining)) { Complete(); return; }
    revealed += static_cast<std::size_t>(count);
    fraction -= count;
}

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

void DialogueBox::Draw(const Resources& resources, Rectangle bounds, const char* speaker, float fontSize, bool allowSkip, bool allowReplay) const
{
    if (!resources.DrawArt(ArtId::DialoguePanel, bounds, ArtFit::Stretch))
        DrawRectangleRec(bounds, Color{28, 26, 42, 255});
    DrawRectangleLinesEx(bounds, 2.0f, Color{127, 151, 176, 255});
    resources.DrawText(speaker, {bounds.x + 18, bounds.y + 12}, 18, 1, Color{242, 197, 112, 255});
    const float width = std::max(1.0f, bounds.width - 36.0f);
    // Wrap the full message so unrevealed words reserve their final positions.
    const auto lines = WrapText(writer.Text(), width, [&](std::string_view text)
    {
        return resources.MeasureText(text, fontSize, 1.0f).x;
    });
    const float lineHeight = fontSize + 7.0f;
    const int capacity = std::max(1, static_cast<int>((bounds.height - 76.0f) / lineHeight));
    int currentLine = 0;
    for (int i = 0; i < static_cast<int>(lines.size()); ++i)
        if (lines[i].begin <= writer.VisibleBytes()) currentLine = i;
    const int first = std::max(0, currentLine - capacity + 1);
    resources.BeginClip({bounds.x + 16, bounds.y + 40, width + 4, std::max(lineHeight, bounds.height - 76.0f)});
    for (int i = first; i < std::min(first + capacity, static_cast<int>(lines.size())); ++i)
    {
        const auto line = lines[i];
        if (writer.VisibleBytes() < line.begin) break;
        const auto end = std::min(line.end, writer.VisibleBytes());
        const auto visible = writer.Text().substr(line.begin, end - line.begin);
        resources.DrawText(visible, {bounds.x + 18, bounds.y + 42 + (i - first) * lineHeight}, fontSize, 1, RAYWHITE);
    }
    EndScissorMode();
    resources.DrawText(writer.IsComplete() ? (allowReplay ? "T: replay message" : "") : (allowSkip ? "Enter: reveal entire message" : ""),
               {bounds.x + 18, bounds.y + bounds.height - 26}, 16, 1, Color{167, 181, 198, 255});
}
}
