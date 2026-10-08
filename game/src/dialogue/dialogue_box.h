#pragma once

#include "typewriter.h"
#include "raylib.h"
#include <functional>
#include <vector>

namespace rage
{
struct TextLine { std::size_t begin; std::size_t end; };
using TextMeasure = std::function<float(std::string_view)>;
std::vector<TextLine> WrapText(const std::string& text, float width, const TextMeasure& measure);

class DialogueBox
{
public:
    Typewriter writer;
    void Draw(Font font, Rectangle bounds, const char* speaker = "The Entity", float fontSize = 20.0f) const;
};
}
