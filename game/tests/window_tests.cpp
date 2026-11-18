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
    if (!GetWindowRect(window, &outer) || !GetClientRect(window, &client))
        throw std::runtime_error("Could not read the native window bounds");
    const int frameWidth = (outer.right - outer.left) - (client.right - client.left);
    const int frameHeight = (outer.bottom - outer.top) - (client.bottom - client.top);
    for (int edge = WMSZ_LEFT; edge <= WMSZ_BOTTOMRIGHT; ++edge)
    {
        RECT proposed{100, 100, 1100, 800};
        if (!SendMessage(window, WM_SIZING, edge, reinterpret_cast<LPARAM>(&proposed)))
            throw std::runtime_error("Native window did not handle the resize");
        const int width = proposed.right - proposed.left - frameWidth;
        const int height = proposed.bottom - proposed.top - frameHeight;
        if (std::abs(width * 9 - height * 16) > 16)
            throw std::runtime_error("Native window resizing did not preserve 16:9");
    }
}
}
#endif
