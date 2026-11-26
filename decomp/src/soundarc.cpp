// Messages to soundarc.dll, the separate sound process (docs/MODDING_MAP.md "Sound architecture").
// Every command is SendMessage(soundarc window, WM_USER, code, value); arguments are sent first with codes 0 and 1.
// DAT_1020aee4 = sound enabled.

#include <windows.h>
#include <stdio.h>

#include "sound_vars.h"

extern "C" HWND FUN_10001570(void);

// Command 0x230 with two arguments.
extern "C" LRESULT FUN_10001660(int first, int second)
{
    if (DAT_1020aee4 == 0)
        return 0;
    HWND wnd = FUN_10001570();
    if (wnd != 0)
    {
        SendMessageA(wnd, 0x400, 0, first);
        SendMessageA(wnd, 0x400, 1, second);
        return SendMessageA(wnd, 0x400, 0x230, 0);
    }
    return 0;
}

// Command 0x201: stop stream `value`.
extern "C" LRESULT FUN_10001ba0(int value)
{
    if (DAT_1020aee4 == 0)
        return 0;
    HWND wnd = FUN_10001570();
    if (wnd != 0)
    {
        SendMessageA(wnd, 0x400, 0, value);
        return SendMessageA(wnd, 0x400, 0x201, 0);
    }
    return 0;
}

// Command 0x212: fade out stream `first` over `second` ms.
extern "C" LRESULT FUN_10001bf0(int first, int second)
{
    if (DAT_1020aee4 == 0)
        return 0;
    HWND wnd = FUN_10001570();
    if (wnd != 0)
    {
        SendMessageA(wnd, 0x400, 0, first);
        SendMessageA(wnd, 0x400, 1, second);
        return SendMessageA(wnd, 0x400, 0x212, 0);
    }
    return 0;
}

// Command 0x224 with one value.
extern "C" LRESULT FUN_10001c50(LPARAM param)
{
    if (DAT_1020aee4 != 0)
    {
        HWND wnd = FUN_10001570();
        if (wnd != 0)
            return SendMessageA(wnd, 0x400, 0x224, param);
    }
    return 0;
}

// Command 0x225 with one value.
extern "C" LRESULT FUN_10001c80(LPARAM param)
{
    if (DAT_1020aee4 != 0)
    {
        HWND wnd = FUN_10001570();
        if (wnd != 0)
            return SendMessageA(wnd, 0x400, 0x225, param);
    }
    return 0;
}

// SoundArc_Init: remembers where soundarc.dll is (`dir`, or the current folder) and starts it when `enable`.
// Returns 1 when its window was found or started.
extern "C" int FUN_10001ee0(int enable, const char *dir)
{
    if (dir != 0)
        sprintf(DAT_1020ade0, "%s\\soundarc.dll", dir);
    else
        sprintf(DAT_1020ade0, ".\\soundarc.dll");
    DAT_1020aee4 = enable;
    return FUN_10001570() != 0;
}

// Shuts the sound process down (WM_CLOSE to its window).
extern "C" int FUN_10001f30(void)
{
    if (DAT_1020aee4 != 0)
    {
        HWND wnd = FUN_10001570();
        SendMessageA(wnd, WM_CLOSE, 0, 0);
        DAT_1020aee4 = 0;
    }
    return 1;
}


// Plays sound file `file` once (command 0x203): argument 0 = `channel`, 1..3 = the position (*x, *y, *z, or 0 for a
// sound without position), 4 = `pan`, 5 = `volume`; the file name goes over as WM_COPYDATA (0x100 bytes).
// Returns 0.
extern "C" int FUN_100016c0(const char *file, const LPARAM *x, const LPARAM *y, const LPARAM *z, LPARAM pan,
                            LPARAM channel, LPARAM volume)
{
    if (DAT_1020aee4 == 0)
        return 0;
    HWND wnd = FUN_10001570();
    if (wnd != 0)
    {
        SendMessageA(wnd, 0x400, 0, channel);
        if (x != 0)
        {
            SendMessageA(wnd, 0x400, 1, *x);
            SendMessageA(wnd, 0x400, 2, *y);
            SendMessageA(wnd, 0x400, 3, *z);
            SendMessageA(wnd, 0x400, 4, pan);
            SendMessageA(wnd, 0x400, 5, volume);
        }
        else
        {
            SendMessageA(wnd, 0x400, 1, 0);
            SendMessageA(wnd, 0x400, 2, 0);
            SendMessageA(wnd, 0x400, 3, 0);
            SendMessageA(wnd, 0x400, 4, pan);
            SendMessageA(wnd, 0x400, 5, volume);
        }
        COPYDATASTRUCT data;
        data.dwData = 0;
        data.lpData = (void *)file;
        data.cbData = 0x100;
        SendMessageA(wnd, WM_COPYDATA, 0, (LPARAM)&data);
        SendMessageA(wnd, 0x400, 0x203, 0);
    }
    return 0;
}

// Same as FUN_100016c0 for a looping sound (command 0x204); returns soundarc's answer to the play command.
extern "C" LRESULT FUN_10001980(const char *file, const LPARAM *x, const LPARAM *y, const LPARAM *z, LPARAM pan,
                            LPARAM channel, LPARAM volume)
{
    if (DAT_1020aee4 == 0)
        return 0;
    HWND wnd = FUN_10001570();
    if (wnd != 0)
    {
        SendMessageA(wnd, 0x400, 0, channel);
        if (x != 0)
        {
            SendMessageA(wnd, 0x400, 1, *x);
            SendMessageA(wnd, 0x400, 2, *y);
            SendMessageA(wnd, 0x400, 3, *z);
            SendMessageA(wnd, 0x400, 4, pan);
            SendMessageA(wnd, 0x400, 5, volume);
        }
        else
        {
            SendMessageA(wnd, 0x400, 1, 0);
            SendMessageA(wnd, 0x400, 2, 0);
            SendMessageA(wnd, 0x400, 3, 0);
            SendMessageA(wnd, 0x400, 4, pan);
            SendMessageA(wnd, 0x400, 5, volume);
        }
        COPYDATASTRUCT data;
        data.dwData = 0;
        data.lpData = (void *)file;
        data.cbData = 0x100;
        SendMessageA(wnd, WM_COPYDATA, 0, (LPARAM)&data);
        return SendMessageA(wnd, 0x400, 0x204, 0);
    }
    return 0;
}

// SoundArc_GetWindow (0x10001570). Returns the window of the sound process ("SOUNDARC" / "SoundArc"). When it is
// already running it is told whether sound is on (WM_USER code 0x400) and the window is returned only if it
// answers. Otherwise soundarc.dll (the path in DAT_1020ade0) is started as a high-priority, minimised process and
// polled for up to 100 x 20 ms;
// a window found that way is told code 0x401 and returned. Returns 0 when there is none. Defined last in the file
// so that the functions above call it like the original does (no knowledge of its registers).
extern "C" HWND FUN_10001570(void)
{
    HWND window = FindWindowA("SOUNDARC", "SoundArc");
    if (window != 0)
        return SendMessageA(window, WM_USER, 0x400, DAT_1020aee4) != 0 ? window : 0;
    STARTUPINFOA startup;
    PROCESS_INFORMATION process;
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWMINNOACTIVE;
    CreateProcessA(0, DAT_1020ade0, 0, 0, FALSE, HIGH_PRIORITY_CLASS, 0, 0, &startup, &process);
    if (process.hProcess != 0)
    {
        int tries = 0;
        do
        {
            tries++;
            window = FindWindowA("SOUNDARC", "SoundArc");
            Sleep(20);
            if (window != 0)
            {
                SendMessageA(window, WM_USER, 0x401, DAT_1020aee4);
                return window;
            }
        } while (tries < 100);
    }
    return 0;
}
