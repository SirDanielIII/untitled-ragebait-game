#pragma once

#include "raylib.h"

namespace rage
{
// Captured once per frame; controllers can also consume these values in tests.
struct InputFrame
{
    float move = 0.0f;
    bool jump = false;
    bool pause = false;
    bool confirm = false;
    bool back = false;
    bool up = false;
    bool down = false;
    bool decrease = false;
    bool increase = false;
    bool minimum = false;
    bool maximum = false;
    bool completeDialogue = false;
    bool replayDialogue = false;
    bool resetPlayer = false;
    Vector2 mouse = {-1.0f, -1.0f};
    bool mouseMoved = false;
    bool click = false;
    bool mouseDown = false;
};

inline InputFrame CaptureInput(Vector2 mouse)
{
    InputFrame input;
    const bool left = IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT);
    const bool right = IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT);
    input.move = static_cast<float>(right) - static_cast<float>(left);
    input.jump = IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP);
    input.pause = IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P);
    input.back = IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE);
    input.confirm = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER);
    input.up = IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W);
    input.down = IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S) || IsKeyPressed(KEY_TAB);
    input.decrease = IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A);
    input.increase = IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D);
    input.minimum = IsKeyPressed(KEY_HOME);
    input.maximum = IsKeyPressed(KEY_END);
    input.completeDialogue = IsKeyPressed(KEY_ENTER);
    input.replayDialogue = IsKeyPressed(KEY_T);
    input.resetPlayer = IsKeyPressed(KEY_R);
    input.mouse = mouse;
    const Vector2 delta = GetMouseDelta();
    input.mouseMoved = delta.x != 0.0f || delta.y != 0.0f;
    input.click = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    input.mouseDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    return input;
}
}
