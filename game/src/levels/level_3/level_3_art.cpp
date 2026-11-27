#include "levels/level_3/level_3_art.h"

#include <cmath>

namespace rage::level3art {
    namespace {
        constexpr Color wallLight{54, 52, 80, 255};
        constexpr Color wallDark{30, 25, 44, 255};
        constexpr Color panel{64, 58, 90, 255};
        constexpr Color trim{100, 92, 130, 255};
        constexpr Color metal{150, 160, 178, 255};
        constexpr Color metalDark{98, 106, 124, 255};
        constexpr Color paper{236, 230, 214, 255};

        // Locker colors
        constexpr Color lockerBlue{70, 112, 150, 255};
        constexpr Color lockerLight{116, 147, 176, 255};
        constexpr Color lockerDark{45, 72, 97, 255};
        constexpr Color lockerNumber{162, 183, 202, 255};

        void Text(const char *text, float x, float y, int size, Color colour) {
            DrawText(text, static_cast<int>(x), static_cast<int>(y), size, colour);
        }

        void TextCentred(const char *text, float centreX, float y, int size, Color colour) {
            Text(text, centreX - MeasureText(text, size) / 2.0f, y, size, colour);
        }

        void Sign(Rectangle r, Color colour, const char *text) {
            DrawLineEx({r.x + 10, r.y - 22}, {r.x + 10, r.y}, 2, metalDark);
            DrawLineEx({r.x + r.width - 10, r.y - 22}, {r.x + r.width - 10, r.y}, 2, metalDark);
            DrawRectangleRounded(r, 0.2f, 4, colour);
            TextCentred(text, r.x + r.width / 2, r.y + (r.height - 12) / 2, 12, RAYWHITE);
        }

        void Window(float x, float y, bool redMoon) {
            const float w = 70, h = 90;
            DrawRectangleRec({x - 4, y - 4, w + 8, h + 8}, trim);
            DrawRectangleRec({x, y, w, h}, {18, 22, 48, 255});
            // A few stars, at the same spots in every window.
            const Color star{210, 210, 240, 255};
            DrawRectangleRec({x + 12, y + 14, 2, 2}, star);
            DrawRectangleRec({x + 50, y + 20, 2, 2}, star);
            DrawRectangleRec({x + 26, y + 58, 2, 2}, star);
            DrawRectangleRec({x + 58, y + 72, 2, 2}, star);
            DrawRectangleRec({x + 8, y + 80, 2, 2}, star);
            if (redMoon) DrawCircle(static_cast<int>(x + 48), static_cast<int>(y + 26), 12, {214, 86, 92, 255});
            DrawRectangleRec({x + w / 2 - 2, y, 4, h}, trim);
            DrawRectangleRec({x, y + h / 2 - 2, w, 4}, trim);
            DrawRectangleRec({x - 8, y + h + 4, w + 16, 6}, trim);
        }
        //First set of lockers
        void Lockers(float x, int count) {
            const float w = 28, h = 130, top = 650 - h;
            for (int i = 0; i < count; ++i) {
                const float lx = x + i * w;
                DrawRectangleRec({lx, top, w - 2, h}, lockerBlue);
                DrawRectangleRec({lx, top, w - 2, 3}, lockerLight);
                for (int v = 0; v < 4; ++v)
                    DrawRectangleRec({lx + 6, top + 12 + v * 6, w - 14, 2}, lockerDark);
                DrawRectangleRec({lx + w - 9, top + 60, 3, 14}, metal);
                Text(TextFormat("%d", 102 + i), lx + 5, top + 40, 10, lockerNumber);
            }
        }

        // Clock
        void Clock(float cx, float cy, float time) {
            DrawCircle(static_cast<int>(cx), static_cast<int>(cy), 20, trim);
            DrawCircle(static_cast<int>(cx), static_cast<int>(cy), 17, paper);
            for (int i = 0; i < 12; ++i) {
                const float a = i * PI / 6;
                DrawLineEx({cx + std::cos(a) * 13, cy + std::sin(a) * 13},
                           {cx + std::cos(a) * 16, cy + std::sin(a) * 16}, 1, DARKGRAY);
            }
            const float minute = time * 2.0f - PI / 2;
            const float hour = time * 0.17f - PI / 2 + 4.4f;
            DrawLineEx({cx, cy}, {cx + std::cos(hour) * 8, cy + std::sin(hour) * 8}, 2, BLACK);
            DrawLineEx({cx, cy}, {cx + std::cos(minute) * 13, cy + std::sin(minute) * 13}, 1.5f, {190, 50, 60, 255});
        }

        void NoticeBoard(float x, float y) {
            DrawRectangleRec({x - 4, y - 4, 158, 78}, {110, 74, 48, 255});
            DrawRectangleRec({x, y, 150, 70}, {176, 132, 88, 255});
            struct Note { float dx, dy, w, h; Color colour; const char *line1; const char *line2; };
            const Note notes[] = {
                {3, 5, 46, 30, paper, "LOST:", "SLEEP"},
                {52, 5, 46, 30, {250, 226, 120, 255}, "QUIZ", "TODAY"},
                {101, 5, 46, 30, {170, 220, 240, 255}, "HELP", ""},
                {6, 40, 68, 27, {246, 180, 190, 255}, "OFFICE HRS", "NEVER"},
                {80, 40, 64, 27, paper, "CISC 320", "BE THERE"},
            };
            for (const auto &n : notes) {
                DrawRectangleRec({x + n.dx, y + n.dy, n.w, n.h}, n.colour);
                DrawCircle(static_cast<int>(x + n.dx + n.w / 2), static_cast<int>(y + n.dy + 2), 2, RED);
                Text(n.line1, x + n.dx + 3, y + n.dy + 5, 10, DARKGRAY);
                Text(n.line2, x + n.dx + 3, y + n.dy + 15, 10, DARKGRAY);
            }
        }

        // Lights
        void CeilingLight(float x) {
            DrawLineEx({x + 10, 340}, {x + 10, 362}, 2, metalDark);
            DrawLineEx({x + 60, 340}, {x + 60, 362}, 2, metalDark);
            DrawRectangleRec({x, 362, 70, 6}, metalDark);
            DrawRectangleRec({x + 3, 368, 64, 3}, {240, 244, 255, 255});
            DrawTriangle({x + 35, 371}, {x - 50, 650}, {x + 120, 650}, {255, 244, 210, 14});
        }

        void WetFloorSign(float x) {
            const float bottom = 650;
            DrawTriangle({x + 15, bottom - 46}, {x, bottom}, {x + 30, bottom}, {244, 204, 58, 255});
            DrawTriangle({x + 15, bottom - 38}, {x + 6, bottom - 6}, {x + 24, bottom - 6}, {40, 36, 30, 255});
            Text("!", x + 13, bottom - 30, 14, {244, 204, 58, 255});
        }
    }

    void Hallway(float worldWidth, float hallX, float time) {
        // The wall fades from lighter to darker
        DrawRectangleGradientH(0, 0, static_cast<int>(hallX), 650, wallLight, wallDark);
        DrawRectangleRec({0, 562, hallX, 88}, panel);
        DrawRectangleRec({0, 558, hallX, 4}, trim);
        DrawRectangleRec({0, 330, hallX, 10}, trim);

        Clock(120, 430, time);
        Lockers(20, 3);
        WetFloorSign(170);
        Sign({270, 392, 130, 26}, {57, 110, 142, 255}, "LECTURE HALL >");
        Sign({560, 378, 160, 26}, {176, 66, 70, 255}, "NO RUNNING IN HALLS");
        NoticeBoard(880, 410);
        Window(1100, 395, false);
        Window(1320, 395, false);
        Window(1960, 395, false);
        Sign({2390, 392, 170, 26}, {57, 110, 142, 255}, "CISC 320 - 8:30 AM >");
        Window(2600, 395, true);

        for (float x = 60; x < worldWidth; x += 340) CeilingLight(x);
    }

    void LectureHall(float hallX, float time) {
        // Brick front of the lecture hall
        DrawRectangleRec({hallX, 300, 400, 350}, {96, 54, 58, 255});
        for (int row = 0; row < 18; ++row) {
            const float y = 300 + row * 20.0f;
            DrawLineEx({hallX, y}, {hallX + 400, y}, 1, {70, 38, 44, 255});
            for (float bx = hallX + (row % 2) * 20.0f; bx < hallX + 400; bx += 40)
                DrawLineEx({bx, y}, {bx, y + 20}, 1, {70, 38, 44, 255});
        }
        DrawRectangleRec({hallX, 296, 400, 8}, trim);


        const float doorX = hallX + 80, doorW = 110;
        DrawRectangleRounded({doorX - 25, 462, doorW + 50, 40}, 0.2f, 4, {30, 38, 56, 255});
        TextCentred("LECTURE HALL 101", doorX + doorW / 2, 468, 12, RAYWHITE);
        TextCentred("IN SESSION", doorX + doorW / 2, 485, 10, {235, 120, 120, 255});
    }

    bool Object(const std::string &assetId, const std::string &id, Rectangle b, float time) {
        if (assetId == "platform" && id == "floor") {
            // Polished hallway tiles
            DrawRectangleRec(b, {104, 98, 124, 255});
            for (float x = b.x; x < b.x + b.width; x += 48) {
                if (static_cast<int>(x / 48) % 2 == 0)
                    DrawRectangleRec({x, b.y, 48, b.height}, {112, 106, 134, 255});
                DrawLineEx({x, b.y}, {x, b.y + b.height}, 1, {84, 78, 104, 255});
            }
            DrawRectangleRec({b.x, b.y, b.width, 3}, {160, 154, 184, 255});
            return true;
        }
        if (assetId == "pillar") {
            // A tower of textbooks
            const float heights[] = {20, 16, 22, 18, 14, 20, 18, 22};
            const float insets[] = {0, 4, 2, 6, 1, 3, 5, 0};
            const Color colours[] = {{70, 120, 190, 255}, {190, 70, 80, 255}, {80, 150, 100, 255},
                                     {214, 160, 60, 255}, {130, 90, 170, 255}, {60, 140, 150, 255}};
            const char *titles[] = {"CALC", "CISC", "PHYS", "STATS", "ECON", "CHEM", "LING", "MATH"};
            // Stack the books from the bottom up
            float bottom = b.y + b.height;
            for (int i = 0; i < 8; ++i) {
                const float h = heights[i];
                if (bottom - h < b.y) break;
                const Rectangle book{b.x + insets[i], bottom - h, b.width - insets[i] * 2, h};
                DrawRectangleRec(book, colours[i % 6]);
                DrawRectangleRec({book.x, book.y, book.width, 2}, {255, 255, 255, 80});  // shine on top
                DrawRectangleRec({book.x + 6, book.y + 3, 3, h - 5}, {0, 0, 0, 80});     // dark spine line
                TextCentred(titles[i], book.x + book.width / 2 + 3, book.y + (h - 10) / 2, 10, RAYWHITE);
                bottom -= h;
            }
            return true;
        }

        return false;
    }
}
