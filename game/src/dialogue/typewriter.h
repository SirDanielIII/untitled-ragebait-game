#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace rage
{
// Owns one complete message, including all lines. Rendering never changes reveal state.
class Typewriter
{
public:
    void Replace(std::string text);
    void Reset();
    void Complete();
    void Update(float dt);
    void SetSpeed(float charactersPerSecond);
    bool IsComplete() const { return revealed == boundaries.size(); }
    const std::string& Text() const { return text; }
    std::size_t VisibleBytes() const { return revealed == 0 ? 0 : boundaries[revealed - 1]; }
    std::string_view VisibleText() const { return std::string_view(text).substr(0, VisibleBytes()); }

private:
    std::string text;
    std::vector<std::size_t> boundaries;
    std::size_t revealed = 0;
    double fraction = 0.0;
    float speed = 40.0f;
};

// Returns a UTF-8 codepoint boundary (ASCII is the common case for this scaffold).
std::size_t NextCharacter(std::string_view text, std::size_t offset);
}
