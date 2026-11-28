#pragma once

#include "game.h"

#include <chrono>
#include <cmath>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace rage::test {
    inline void Check(bool condition, const char *message) {
        if (!condition) throw std::runtime_error(message);
    }

    inline bool Near(float a, float b, float tolerance = 0.05f) {
        return std::abs(a - b) < tolerance;
    }

    class TestRenderTarget {
    public:
        explicit TestRenderTarget(int width = canvasWidth, int height = canvasHeight)
            : target(LoadRenderTexture(width, height)) {
            Check(target.id != 0, "Could not create test render target");
        }
        ~TestRenderTarget() { UnloadRenderTexture(target); }
        TestRenderTarget(const TestRenderTarget &) = delete;
        TestRenderTarget &operator=(const TestRenderTarget &) = delete;
        RenderTexture2D target;
    };

    class TestDirectory {
    public:
        const std::filesystem::path path = std::filesystem::temp_directory_path() /
            ("ragebait_test_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));

        TestDirectory() { std::filesystem::create_directories(path); }
        ~TestDirectory() {
            std::error_code error;
            std::filesystem::remove_all(path, error);
        }

        TestDirectory(const TestDirectory &) = delete;
        TestDirectory &operator=(const TestDirectory &) = delete;
    };

    inline InputFrame ClickAction(const Game &game, ActionKind kind, int value = 0) {
        for (const auto &item: game.Navigation().Items()) {
            const auto *button = std::get_if<Button>(&item);
            if (button && button->action.kind == kind && button->action.value == value) {
                const auto bounds = button->bounds;
                return {.mouse = {bounds.x + bounds.width / 2, bounds.y + bounds.height / 2}, .click = true};
            }
        }
        throw std::runtime_error("Requested menu action is missing");
    }
}
