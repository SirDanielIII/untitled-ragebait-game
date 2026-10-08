#include "typewriter.h"

#include <algorithm>
#include <cmath>
#include <utility>

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
}
