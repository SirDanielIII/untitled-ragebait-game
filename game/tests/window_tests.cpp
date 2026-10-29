#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdlib>
#include <stdexcept>

namespace rage
{
// Exercise the actual native resize handler for each edge and corner.
void TestWindowResize(void* handle)
{
    const auto window = static_cast<HWND>(handle);
    RECT outer{}, client{};
    GetWindowRect(window, &outer);
    GetClientRect(window, &client);
    const int frameWidth = (outer.right - outer.left) - client.right;
    const int frameHeight = (outer.bottom - outer.top) - client.bottom;
    for (int edge = WMSZ_LEFT; edge <= WMSZ_BOTTOMRIGHT; ++edge)
    {
        RECT proposed{100, 100, 1100, 800};
        SendMessage(window, WM_SIZING, edge, reinterpret_cast<LPARAM>(&proposed));
        const int width = proposed.right - proposed.left - frameWidth;
        const int height = proposed.bottom - proposed.top - frameHeight;
        if (std::abs(width * 9 - height * 16) > 16)
            throw std::runtime_error("Native window resizing did not preserve 16:9");
    }
}
}
#endif
