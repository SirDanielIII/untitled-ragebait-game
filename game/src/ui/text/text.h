#pragma once
#include "engine/resources/resources.h"
#include <string>
namespace rage
{
// Menu text stays inside its layout when the selected font changes.
void DrawTextFit(const Resources& resources, const std::string& text, Rectangle bounds, float size, Color color);
void DrawParagraph(const Resources& resources, const std::string& text, Rectangle bounds, float size, Color color);
}
