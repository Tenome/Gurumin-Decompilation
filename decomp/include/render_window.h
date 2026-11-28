// RenderWindow, GameWindow: declared here once (methods in src/window_create.cpp; ime.cpp sets the IME callbacks).
#ifndef GURUMIN_RENDER_WINDOW_H
#define GURUMIN_RENDER_WINDOW_H

#include <windows.h>

typedef int (*WindowCallback)();    // an IME event callback, called by GameWindow::WindowProc (returns 0)

// The engine's window: creates the Win32 window and forwards its messages to the virtual WindowProc.
struct RenderWindow
{
    virtual LRESULT __stdcall WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
    HWND window;                         // 0x04
    int active;                          // 0x08
    HINSTANCE instance;                  // 0x0c
    int compositionResult;               // 0x10: an IME result string arrived
    int compositionEnded;                // 0x14: the IME composition string changed
    WindowCallback onCompositionString;  // 0x18: IME callbacks (ime.cpp)
    WindowCallback onCompositionResult;  // 0x1c
    WindowCallback onCursorPos;          // 0x20
    WindowCallback onAttributes;         // 0x24
    char pad_0028[0x30];
    char title[0x64];                    // 0x58
    char className[0x64];                // 0xbc
    int wheelDelta;                      // 0x120: mouse wheel movement collected by WindowProc
    int middleCapture;                   // 0x124: the middle button holds the mouse capture
    HICON icon;                          // 0x128
    RenderWindow();
    ~RenderWindow() { window = 0; }  // inline: the window handle is forgotten (the window dies with the process)
    void RegisterWindowClass(HINSTANCE instance);
    void Create(HINSTANCE instance);
};

// The game's window (the instance at 0x10f65698): a RenderWindow whose WindowProc handles the game's messages.
struct GameWindow : RenderWindow
{
    LRESULT __stdcall WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
    GameWindow();
    ~GameWindow();  // 0x1013e4e0 (with the RenderWindow destructor expanded inline)
};

extern "C" GameWindow DAT_10f65698;       // the game window
extern "C" RenderWindow *DAT_10f1a860;    // the window whose WindowProc receives the messages (set by Create)

#endif
