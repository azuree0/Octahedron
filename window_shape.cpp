#include "window_shape.h"

#include <SFML/Graphics.hpp>

#include <algorithm>

#if defined(_WIN32)
#include <windows.h>
#endif

void applySquareWindowShape(sf::RenderWindow& window) {
#if defined(_WIN32)
    const sf::Vector2u sz = window.getSize();
    if (sz.x < 4 || sz.y < 4) {
        return;
    }
    HWND hwnd = static_cast<HWND>(window.getNativeHandle());
    if (hwnd == nullptr) {
        return;
    }

    RECT clientR{};
    if (GetClientRect(hwnd, &clientR) == 0) {
        return;
    }
    POINT topLeft{};
    topLeft.x = clientR.left;
    topLeft.y = clientR.top;
    static_cast<void>(ClientToScreen(hwnd, &topLeft));
    RECT winR{};
    if (GetWindowRect(hwnd, &winR) == 0) {
        return;
    }
    const LONG ox = topLeft.x - winR.left;
    const LONG oy = topLeft.y - winR.top;
    const LONG cw = clientR.right - clientR.left;
    const LONG ch = clientR.bottom - clientR.top;
    const LONG side = std::min(cw, ch);
    const LONG insetX = ox + (cw - side) / 2;
    const LONG insetY = oy + (ch - side) / 2;

    HRGN rgn = CreateRectRgn(insetX, insetY, insetX + side, insetY + side);
    if (rgn != nullptr) {
        static_cast<void>(SetWindowRgn(hwnd, rgn, TRUE));
    }
#else
    (void)window;
#endif
}
