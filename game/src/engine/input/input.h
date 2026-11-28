#pragma once

#include "raylib.h"

namespace rage {
    // Captured once per frame; controllers can also consume these values in tests.
    struct InputFrame {
        float move = 0.0f;
        bool jump = false; // Held: automatically jump again when supported and affordable.
        bool pause = false;
        bool confirm = false;
        bool back = false;
        bool up = false;
        bool down = false;
        bool decrease = false;
        bool increase = false;
        bool minimum = false;
        bool maximum = false;
        bool climb = false;
        bool completeDialogue = false;
        bool replayDialogue = false;
        bool resetPlayer = false;
        Vector2 mouse = {-1.0f, -1.0f};
        bool mouseMoved = false;
        bool click = false;
        bool mouseDown = false;
        bool duck = false; // Held S / Down, independent of menu key presses.
        bool toggleFullscreen = false; // Pressed once, not held.
    };

    // Keep shortcut detection testable without a window.
    inline constexpr bool FullscreenShortcut(bool f11Pressed, bool enterPressed, bool altDown) {
        return f11Pressed || (enterPressed && altDown);
    }

    inline InputFrame CaptureInput(Vector2 mouse) {
        InputFrame input;
        const bool left = IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT);
        const bool right = IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT);
        input.move = static_cast<float>(right) - static_cast<float>(left);
        input.jump = IsKeyDown(KEY_SPACE) || IsKeyDown(KEY_W) || IsKeyDown(KEY_UP);
        input.pause = IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P);
        input.back = IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE);
        const bool enter = IsKeyPressed(KEY_ENTER);
        const bool keypadEnter = IsKeyPressed(KEY_KP_ENTER);
#if !defined(PLATFORM_WEB)
        input.toggleFullscreen = FullscreenShortcut(IsKeyPressed(KEY_F11), enter || keypadEnter,
                                                     IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT));
#endif
        input.confirm = (enter || keypadEnter) && !input.toggleFullscreen;
        input.up = IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W);
        input.climb = IsKeyDown(KEY_W) || IsKeyDown(KEY_UP);
        input.down = IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S) || IsKeyPressed(KEY_TAB);
        input.duck = IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN);
        input.decrease = IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A);
        input.increase = IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D);
        input.minimum = IsKeyPressed(KEY_HOME);
        input.maximum = IsKeyPressed(KEY_END);
        input.completeDialogue = enter && !input.toggleFullscreen;
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
