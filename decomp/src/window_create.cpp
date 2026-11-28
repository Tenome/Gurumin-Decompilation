// The game window (RenderWindow, global instance DAT_10f65698 / pointer DAT_10f1a860).

#define _WIN32_WINNT 0x0400
#include <windows.h>
#include <string.h>
#include <imm.h>

#include "render_window.h"
#include "render_vars.h"

// Constructor: no window yet, no icon, and the window title and class name both set to "Kernel Class"
// (the game changes the title later).
RenderWindow::RenderWindow()
{
    icon = 0;
    strcpy(title, "Kernel Class");
    strcpy(className, "Kernel Class");
    window = 0;
}

// Registers the window class and creates the window at the configured size (DAT_10202b24 x DAT_10202b28):
// a borderless popup in full screen (DAT_10f65514), a normal overlapped window otherwise. A windowed or
// borderless mode for modern displays would change this.
void RenderWindow::Create(HINSTANCE instance)
{
    middleCapture = 0;
    wheelDelta = 0;
    active = 0;
    RegisterWindowClass(instance);
    this->instance = instance;
    DAT_10f1a860 = this;
    if (DAT_10f65514 != 0)
        window = CreateWindowExA(0, className, title, WS_POPUP, CW_USEDEFAULT, 0, DAT_10202b24, DAT_10202b28, 0, 0, this->instance, 0);
    else
        window = CreateWindowExA(0, className, title, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, DAT_10202b24, DAT_10202b28, 0, 0, this->instance, 0);
}

// The Windows procedure registered for the class: stores the RenderWindow pointer in the window's extra data on
// the first message and forwards every message to its (virtual) WindowProc.
extern "C" LRESULT CALLBACK FUN_1013e410(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (GetWindowLongA(window, 8) == 0)
        SetWindowLongA(window, 8, (LONG)DAT_10f1a860);
    return DAT_10f1a860->WindowProc(window, message, wParam, lParam);
}

// Registers the window class (redraw on resize, arrow cursor, window-colour background, the game icon).
void RenderWindow::RegisterWindowClass(HINSTANCE instance)
{
    WNDCLASSEXA wc;
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = FUN_1013e410;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = instance;
    wc.hIcon = icon;
    wc.hCursor = LoadCursorA(0, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW - 0);
    wc.lpszMenuName = 0;
    wc.lpszClassName = className;
    wc.hIconSm = 0;
    RegisterClassExA(&wc);
}
// 0x1013e450. The base window procedure: every message goes to DefWindowProc.
LRESULT __stdcall RenderWindow::WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProcA(window, message, wParam, lParam);
}

// 0x1013e6a0. Constructor: only the RenderWindow part and the vtable.
GameWindow::GameWindow()
{
}

// 0x1013e4e0. Destructor: puts the RenderWindow vtable back and clears the window handle (RenderWindow's inline
// destructor expanded here). Nothing in the game destroys the GameWindow instance by a call. The linker also kept a
// second copy at 0x1013e6c0 that only jumps here (see open.tsv).
GameWindow::~GameWindow()
{
}

// 0x1013e4f0. Handles the messages the game cares about: quit, paint, focus changes (active), IME composition (forwarded
// to the callbacks set by the text input code), middle-button capture and the mouse wheel; it closes the IME's
// own window (WM_IME_SETCONTEXT) because the game draws the composition itself. Everything else goes to
// DefWindowProc.
LRESULT __stdcall GameWindow::WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_PAINT:
        ValidateRect(window, 0);
        return 0;
    case WM_ACTIVATEAPP:
        active = wParam;
        break;
    // Key down and key up are listed (the original's switch has both labels; the pivot of its case tree depends on
    // them) but only ever re-test Escape and fall through to DefWindowProc.
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
            break;
        break;
    case WM_KEYUP:
        if (wParam == VK_ESCAPE)
            break;
        break;
    case WM_IME_ENDCOMPOSITION:
        onCompositionString();
        compositionEnded = 1;
        return 0;
    case WM_IME_COMPOSITION:
        if (lParam & GCS_COMPSTR)
        {
            onCompositionString();
            compositionEnded = 1;
        }
        if (lParam & GCS_RESULTSTR)
        {
            onCompositionResult();
            compositionResult = 1;
        }
        if (lParam & GCS_CURSORPOS)
            onCursorPos();
        if (lParam & GCS_COMPCLAUSE)
            onAttributes();
        return 0;
    case WM_MBUTTONDOWN:
        SetCapture(window);
        middleCapture = 1;
        break;
    case WM_MBUTTONUP:
        ReleaseCapture();
        middleCapture = 0;
        break;
    case WM_MOUSEWHEEL:
        wheelDelta += (short)HIWORD(wParam);
        break;
    case WM_IME_SETCONTEXT:
        // The IME window is fetched before the message arguments are pushed.
        HWND imeWindow = ImmGetDefaultIMEWnd(window);
        SendMessageA(imeWindow, WM_CLOSE, 0, 0);
        return 0;
    }
    return DefWindowProcA(window, message, wParam, lParam);
}