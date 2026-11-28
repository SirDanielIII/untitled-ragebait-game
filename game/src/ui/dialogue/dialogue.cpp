#include "ui/dialogue/dialogue.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <utility>
#include <string>

namespace rage {
    std::size_t NextCharacter(std::string_view text, std::size_t offset) {
        if (offset >= text.size()) return text.size();
        ++offset;
        while (offset < text.size() && (static_cast<unsigned char>(text[offset]) & 0xC0) == 0x80) ++offset;
        return offset;
    }

    void Typewriter::Replace(std::string message) {
        text.clear();
        for (std::size_t i = 0; i < message.size(); ++i) {
            if (message[i] == '\r') {
                text += '\n';
                if (i + 1 < message.size() && message[i + 1] == '\n') ++i;
            } else text += message[i];
        }
        boundaries.clear();
        for (std::size_t i = 0; i < text.size();) {
            i = NextCharacter(text, i);
            boundaries.push_back(i);
        }
        Reset();
    }

    void Typewriter::Reset() {
        revealed = 0;
        fraction = 0.0;
        age = 0;
        timeFullyVisible = 0;
        if (speed == 0.0f) Complete();
    }

    void Typewriter::Complete() {
        revealed = boundaries.size();
        fraction = 0.0;
    }

    void Typewriter::SetSpeed(float charactersPerSecond) {
        speed = std::isfinite(charactersPerSecond) ? std::max(0.0f, charactersPerSecond) : 40.0f;
        if (speed == 0.0f) Complete();
    }

    void Typewriter::Update(float dt) {
        if (!std::isfinite(dt) || dt <= 0.0f) return;
        age += dt;
        if (IsComplete()) {
            timeFullyVisible += dt;
            return;
        }
        fraction += static_cast<double>(dt) * speed;
        const double count = std::floor(fraction);
        const auto remaining = boundaries.size() - revealed;
        if (count >= static_cast<double>(remaining)) {
            timeFullyVisible = (fraction - static_cast<double>(remaining)) / speed;
            Complete();
            return;
        }
        revealed += static_cast<std::size_t>(count);
        fraction -= count;
    }

    void Typewriter::SetEffects(DialogueEffects settings) {
        const auto nonnegative = [](float value) {
            return std::isfinite(value) ? std::max(0.0f, value) : 0.0f;
        };
        effects.shakePixels = std::min(8.0f, nonnegative(settings.shakePixels));
        effects.shakeHz = std::min(30.0f, nonnegative(settings.shakeHz));
        effects.holdSeconds = nonnegative(settings.holdSeconds);
        effects.fadeSeconds = nonnegative(settings.fadeSeconds);
    }

    float Typewriter::Opacity() const {
        if (effects.fadeSeconds == 0 || timeFullyVisible <= effects.holdSeconds) return 1;
        const double fading = (timeFullyVisible - effects.holdSeconds) / effects.fadeSeconds;
        return static_cast<float>(std::max(0.0, 1.0 - fading));
    }

    Vector2 Typewriter::TextOffset() const {
        if (effects.shakePixels == 0 || effects.shakeHz == 0) return {};
        const double phase = 2.0 * std::numbers::pi * effects.shakeHz * age;
        return {effects.shakePixels * static_cast<float>(std::sin(phase)),
                effects.shakePixels * static_cast<float>(std::sin(phase * 1.37))};
    }

    std::vector<TextLine> WrapText(std::string_view text, float width, const TextMeasure &measure) {
        std::vector<TextLine> lines;
        std::size_t start = 0;
        std::size_t cursor = 0;
        std::size_t space = std::string::npos;
        while (cursor < text.size()) {
            if (text[cursor] == '\n') {
                lines.push_back({start, cursor});
                start = ++cursor;
                space = std::string::npos;
                continue;
            }
            const auto next = NextCharacter(text, cursor);
            if (cursor > start && measure(std::string_view(text).substr(start, next - start)) > width) {
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

    void DialogueBox::Draw(const Resources &resources, Rectangle bounds, const char *speaker, float fontSize, bool allowSkip, bool allowReplay) const {
        const float opacity = writer.Opacity();
        const Vector2 hintPosition{bounds.x + 18, bounds.y + bounds.height - 26};
        const Color hintColor{167, 181, 198, 255};
        const char *hint = writer.IsComplete() ? (allowReplay ? "T: replay last message" : "")
                                               : (allowSkip ? "Enter: reveal entire message" : "");
        if (opacity == 0) {
            resources.DrawText(hint, hintPosition, 16, 1, hintColor);
            return;
        }
        if (!resources.DrawArt(ArtId::DialoguePanel, bounds, ArtFit::Stretch, Fade(WHITE, opacity)))
            DrawRectangleRec(bounds, Fade({28, 26, 42, 255}, opacity));
        DrawRectangleLinesEx(bounds, 2.0f, Fade({127, 151, 176, 255}, opacity));
        resources.DrawText(speaker, {bounds.x + 18, bounds.y + 12}, 18, 1, Fade({242, 197, 112, 255}, opacity), FontRole::Heading);
        const float width = std::max(1.0f, bounds.width - 36.0f);
        // Wrap the full message so unrevealed words reserve their final positions.
        const auto lines = WrapText(writer.Text(), width, [&](std::string_view text) {
            return resources.MeasureText(text, fontSize, 1.0f, FontRole::Typewriter).x;
        });
        const float lineHeight = fontSize + 7.0f;
        const int capacity = std::max(1, static_cast<int>((bounds.height - 76.0f) / lineHeight));
        int currentLine = 0;
        for (int i = 0; i < static_cast<int>(lines.size()); ++i)
            if (lines[i].begin <= writer.VisibleBytes()) currentLine = i;
        const int first = std::max(0, currentLine - capacity + 1);
        const auto offset = writer.TextOffset();
        resources.BeginClip({bounds.x + 8, bounds.y + 34, width + 20, std::max(lineHeight, bounds.height - 70.0f)});
        for (int i = first; i < std::min(first + capacity, static_cast<int>(lines.size())); ++i) {
            const auto line = lines[i];
            if (writer.VisibleBytes() < line.begin) break;
            const auto end = std::min(line.end, writer.VisibleBytes());
            const auto visible = std::string_view(writer.Text()).substr(line.begin, end - line.begin);
            resources.DrawText(visible, {bounds.x + 18 + offset.x, bounds.y + 42 + (i - first) * lineHeight + offset.y},
                               fontSize, 1, Fade(RAYWHITE, opacity), FontRole::Typewriter);
        }
        EndScissorMode();
        resources.DrawText(hint, hintPosition, 16, 1, hintColor);
    }
}
