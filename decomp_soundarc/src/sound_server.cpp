// sound_server.cpp : SoundArc (shipped as SoundArc.dll but a GUI executable), the sound server of Gurumin.
//
// WinMain starts a hidden tray program, the window procedure carries out the commands the game sends and
// LoadSound3D / ReleaseSound3D bind and unbind sound3d.dll. Original flags: /Od /ZI /Gy /GX (no /O2, /Ob1, /Op or /GS), see the report.
#include <stdio.h>
#include <string.h>
#include "sound_server.h"

// ---- globals ------------------------------------------------------------------------------------------------

char g_mutexName[] = "SoundArc";
NOTIFYICONDATA g_trayIcon;
HWND g_mainWindow;
char g_windowClassName[100 + 4];
char g_windowTitle[100];
HINSTANCE g_instance;
Mailbox g_mailbox;
HWND g_clientWindow;
int g_logEnabled;
int g_logPaused;
int g_textPending;
Sound3dApi g_sound3d;

// Path of the log file; logging is on when it exists at start-up.
static const char LOG_FILE_NAME[] = "c:\\sound.log";

SoundArcApp::SoundArcApp()
{
}

SoundArcApp::~SoundArcApp()
{
}

// Program entry (called by the CRT startup). Refuses to run twice (named mutex), creates the hidden window and the
// tray icon, notes whether c:\sound.log exists, then runs the message loop. The loop also ends when the game's
// window has disappeared. Returns the exit code of WM_QUIT.
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    HANDLE hMutex;
    MSG msg;
    HACCEL hAccelTable;
    FILE *logProbe;
    FILE *logFile;
    int idleLoops;
    int clientVisible;
    int clientExists;

    hMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, g_mutexName);
    if (hMutex) {
        CloseHandle(hMutex);
        return 0;
    }
    hMutex = CreateMutex(NULL, FALSE, g_mutexName);

    LoadString(hInstance, IDS_APP_TITLE, g_windowTitle, 100);
    LoadString(hInstance, IDC_SOUNDARC, g_windowClassName, 100);
    MyRegisterClass(hInstance);
    if (!InitInstance(hInstance, nCmdShow))
        return 0;

    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SOUNDARC));

    memset(&g_trayIcon, 0, sizeof(NOTIFYICONDATA));
    g_trayIcon.cbSize = sizeof(NOTIFYICONDATA);
    g_trayIcon.hWnd = g_mainWindow;
    g_trayIcon.uID = TRAY_ICON_ID;
    g_trayIcon.uFlags = NIF_MESSAGE | NIF_ICON;
    g_trayIcon.uCallbackMessage = WM_TRAYICON;
    g_trayIcon.hIcon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_SOUNDARC), IMAGE_ICON, 16, 16, 0);
    Shell_NotifyIcon(NIM_DELETE, &g_trayIcon);
    Shell_NotifyIcon(NIM_DELETE, &g_trayIcon);
    Shell_NotifyIcon(NIM_ADD, &g_trayIcon);

    logProbe = fopen(LOG_FILE_NAME, "rt");
    if (logProbe) {
        g_logEnabled = 1;
        fclose(logProbe);
        logFile = fopen(LOG_FILE_NAME, "at");
        if (logFile) {
            // The shipped code writes the header through the probe handle it has just closed (an original bug).
            fprintf(logProbe, "\x81\x9a\x81\x9a\x81\x9aLOG-Start!!---------------\n");
            fclose(logProbe);
        }
    }

    SetTimer(g_mainWindow, TIMER_ID_POLL, TIMER_POLL_MS, NULL);

    idleLoops = 0;
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        Sleep(0);
        idleLoops++;
        if (idleLoops > 200 && !g_logPaused && g_clientWindow) {
            idleLoops = 0;
            clientVisible = IsWindowVisible(g_clientWindow);
            clientExists = IsWindow(g_clientWindow);
            if (!clientVisible || !clientExists)
                break;
        }
    }

    Shell_NotifyIcon(NIM_DELETE, &g_trayIcon);
    ReleaseSound3D();
    ReleaseMutex(hMutex);
    return msg.wParam;
}

// Takes one pending message off the queue, dispatches it and sleeps 1 ms. Not called from anywhere in the shipped
// program (a leftover alternative to the GetMessage loop).
void PumpOneMessage(void)
{
    MSG msg;

    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        Sleep(1);
    }
}

// Registers the window class (called by WinMain). Standard Win32 template values.
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = (WNDPROC)WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SOUNDARC));
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCE(IDC_SOUNDARC);
    wcex.lpszClassName = g_windowClassName;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    return RegisterClassEx(&wcex);
}

// Creates the main window (called by WinMain): a tiny minimised tool window that is made invisible right after
// creation. Returns FALSE when CreateWindowEx fails.
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    LONG style;

    g_instance = hInstance;
    g_mainWindow = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT, g_windowClassName, g_windowTitle,
                                  WS_MINIMIZE | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME, 0, 0, 128, 24,
                                  NULL, NULL, hInstance, NULL);
    if (!g_mainWindow)
        return FALSE;
    UpdateWindow(g_mainWindow);
    style = GetWindowLong(g_mainWindow, GWL_STYLE);
    SetWindowLong(g_mainWindow, GWL_STYLE, style & ~WS_VISIBLE);
    return TRUE;
}

// Appends one line to c:\sound.log (if the file can be opened). Called by the window procedure for every command
// while logging is on. Always returns 0.
int LogLine(const char *line)
{
    FILE *logFile;

    logFile = fopen(LOG_FILE_NAME, "at");
    if (logFile) {
        fprintf(logFile, "%s\n", line);
        fclose(logFile);
    }
    return 0;
}

// Window procedure of the hidden main window. Handles the tray icon, the tray menu, WM_COPYDATA (file name from the
// game) and WM_SOUND_COMMAND, the game's channel: slots below MAILBOX_SLOT_COUNT receive arguments, the other
// wParams are commands forwarded to sound3d.dll (and logged first when logging is on).
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int result;
    int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc;
    TCHAR szHello[100];

    LoadString(g_instance, IDS_HELLO, szHello, 100);
    switch (message) {
    case WM_TIMER:
        break;

    case WM_TRAYICON:
        if (wParam == TRAY_ICON_ID && (lParam == WM_LBUTTONDOWN || lParam == WM_RBUTTONDOWN)) {
            HMENU hMenu;
            HMENU hSubMenu;
            POINT pt;

            GetCursorPos(&pt);
            hMenu = LoadMenu(NULL, MAKEINTRESOURCE(IDC_SOUNDARC));
            hSubMenu = GetSubMenu(hMenu, 0);
            if (!g_logPaused) {
                MENUITEMINFO info;
                char label[256];

                info.cbSize = sizeof(MENUITEMINFO);
                info.dwTypeData = 0;
                info.fMask = MIIM_STATE;
                GetMenuItemInfo(hSubMenu, IDM_PAUSE_TOGGLE, FALSE, &info);
                info.dwTypeData = label;
                GetMenuItemInfo(hSubMenu, IDM_PAUSE_TOGGLE, FALSE, &info);
                info.fState = MFS_CHECKED;
                SetMenuItemInfo(hSubMenu, IDM_PAUSE_TOGGLE, FALSE, &info);
            }
            if (g_logEnabled) {
                MENUITEMINFO info;
                char label[256];

                info.cbSize = sizeof(MENUITEMINFO);
                info.dwTypeData = 0;
                info.fMask = MIIM_STATE;
                GetMenuItemInfo(hSubMenu, IDM_LOG_TOGGLE, FALSE, &info);
                info.dwTypeData = label;
                GetMenuItemInfo(hSubMenu, IDM_LOG_TOGGLE, FALSE, &info);
                info.fState = MFS_CHECKED;
                SetMenuItemInfo(hSubMenu, IDM_LOG_TOGGLE, FALSE, &info);
            }
            TrackPopupMenu(hSubMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
        }
        break;

    case WM_SOUND_COMMAND:
        if (wParam < MAILBOX_SLOT_COUNT) {
            g_mailbox.slot[wParam].i = lParam;
        } else {
        if (wParam == SND_PLAY_FADE && g_sound3d.playFade) {
            if (g_logEnabled) {
                char logText[256];
                sprintf(logText, "PlayFade(%d,%s,%d,%d,%d,%d)", g_mailbox.slot[0].i, g_mailbox.named.text,
                        g_mailbox.slot[1].i, g_mailbox.slot[2].i, g_mailbox.slot[3].i, g_mailbox.slot[4].i);
                LogLine(logText);
            }
            result = g_sound3d.playFade(g_mailbox.slot[0].i, g_mailbox.named.text, g_mailbox.slot[1].i,
                                        g_mailbox.slot[2].i, g_mailbox.slot[3].i, g_mailbox.slot[4].i);
            g_textPending = 0;
            return result;
        }
        if (wParam == SND_PLAY_FADE_B && g_sound3d.playFadeB) {
            if (g_logEnabled) {
                char logText[256];
                sprintf(logText, "PlayFadeB(%d,%s,%d,%d,%d,%d)", g_mailbox.slot[0].i, g_mailbox.named.text,
                        g_mailbox.slot[1].i, g_mailbox.slot[2].i, g_mailbox.slot[3].i, g_mailbox.slot[4].i);
                LogLine(logText);
            }
            result = g_sound3d.playFadeB(g_mailbox.slot[0].i, g_mailbox.named.text, g_mailbox.slot[1].i,
                                         g_mailbox.slot[2].i, g_mailbox.slot[3].i, g_mailbox.slot[4].i);
            g_textPending = 0;
            return result;
        }
        if (wParam == SND_FADE_OUT && g_sound3d.fadeOut) {
            if (g_logEnabled) {
                char logText[256];
                sprintf(logText, "FadeOut(%d)", g_mailbox.slot[0].i);
                LogLine(logText);
            }
            result = g_sound3d.fadeOut(g_mailbox.slot[0].i);
            return result;
        }
        if (wParam == SND_PLAY_SE && g_sound3d.playSE && g_mailbox.slot[4].i) {
            if (g_logEnabled) {
                char logText[256];
                sprintf(logText, "PlaySE(%s,%f,%f,%f,%d,%d,%f)", g_mailbox.named.text, g_mailbox.slot[1].f,
                        g_mailbox.slot[2].f, g_mailbox.slot[3].f, g_mailbox.slot[4].i, g_mailbox.slot[0].i,
                        g_mailbox.slot[5].f);
                LogLine(logText);
            }
            result = g_sound3d.playSE(g_mailbox.named.text, &g_mailbox.slot[1].f, &g_mailbox.slot[2].f,
                                      &g_mailbox.slot[3].f, g_mailbox.slot[4].i, g_mailbox.slot[0].i,
                                      g_mailbox.slot[5].f);
            g_textPending = 0;
            return result;
        }
        if (wParam == SND_PLAY_SE_B && g_sound3d.playSEB && g_mailbox.slot[4].i) {
            if (g_logEnabled) {
                char logText[256];
                sprintf(logText, "PlaySEB(%s,%f,%f,%f,%d,%d,%f)", g_mailbox.named.text, g_mailbox.slot[1].f,
                        g_mailbox.slot[2].f, g_mailbox.slot[3].f, g_mailbox.slot[4].i, g_mailbox.slot[0].i,
                        g_mailbox.slot[5].f);
                LogLine(logText);
            }
            result = g_sound3d.playSEB(g_mailbox.named.text, &g_mailbox.slot[1].f, &g_mailbox.slot[2].f,
                                       &g_mailbox.slot[3].f, g_mailbox.slot[4].i, g_mailbox.slot[0].i,
                                       g_mailbox.slot[5].f);
            g_textPending = 0;
            return result;
        }
        if (wParam == SND_PLAY_SE && g_sound3d.playSE && !g_mailbox.slot[4].i) {
            if (g_logEnabled) {
                char logText[256];
                sprintf(logText, "PlaySE(%s,0,0,0,%d,%d)", g_mailbox.named.text, g_mailbox.slot[4].i,
                        g_mailbox.slot[0].i);
                LogLine(logText);
            }
            result = g_sound3d.playSE(g_mailbox.named.text, 0, 0, 0, g_mailbox.slot[4].i, g_mailbox.slot[0].i,
                                      g_mailbox.slot[5].f);
            g_textPending = 0;
            return result;
        }
        if (wParam == SND_PLAY_SE_B && g_sound3d.playSEB && !g_mailbox.slot[4].i) {
            if (g_logEnabled) {
                char logText[256];
                sprintf(logText, "PlaySEB(%s,0,0,0,%d,%d)", g_mailbox.named.text, g_mailbox.slot[4].i,
                        g_mailbox.slot[0].i);
                LogLine(logText);
            }
            result = g_sound3d.playSEB(g_mailbox.named.text, 0, 0, 0, g_mailbox.slot[4].i, g_mailbox.slot[0].i,
                                       g_mailbox.slot[5].f);
            g_textPending = 0;
            return result;
        }
        if (wParam == SND_PLAY_FADE_EX && g_sound3d.playFadeEx) {
            if (g_logEnabled) {
                char logText[256];
                sprintf(logText, "PlayFadeEx(%d,%s,%d,%d,%d,%d,%d)", g_mailbox.slot[0].i, g_mailbox.named.text,
                        g_mailbox.slot[1].i, g_mailbox.slot[2].i, g_mailbox.slot[3].i, g_mailbox.slot[4].i,
                        g_mailbox.slot[5].i);
                LogLine(logText);
            }
            result = g_sound3d.playFadeEx(g_mailbox.slot[0].i, g_mailbox.named.text, g_mailbox.slot[1].i,
                                          g_mailbox.slot[2].i, g_mailbox.slot[3].i, g_mailbox.slot[4].i,
                                          g_mailbox.slot[5].i);
            g_textPending = 0;
            return result;
        }
        if (wParam == SND_PLAY_FADE_B_EX && g_sound3d.playFadeBEx) {
            if (g_logEnabled) {
                char logText[256];
                sprintf(logText, "PlayFadeBEx(%d,%s,%d,%d,%d,%d,%d)", g_mailbox.slot[0].i, g_mailbox.named.text,
                        g_mailbox.slot[1].i, g_mailbox.slot[2].i, g_mailbox.slot[3].i, g_mailbox.slot[4].i,
                        g_mailbox.slot[5].i);
                LogLine(logText);
            }
            result = g_sound3d.playFadeBEx(g_mailbox.slot[0].i, g_mailbox.named.text, g_mailbox.slot[1].i,
                                           g_mailbox.slot[2].i, g_mailbox.slot[3].i, g_mailbox.slot[4].i,
                                           g_mailbox.slot[5].i);
            g_textPending = 0;
            return result;
        }
        if (wParam == SND_PLAY && g_sound3d.play) {
            if (g_logEnabled) {
                char logText[256];
                sprintf(logText, "Play(%d,%s,%d,%d,%d,%d)", g_mailbox.slot[0].i, g_mailbox.named.text,
                        g_mailbox.slot[1].i, g_mailbox.slot[2].i, g_mailbox.slot[3].i, g_mailbox.slot[4].i);
                LogLine(logText);
            }
            result = g_sound3d.play(g_mailbox.slot[0].i, g_mailbox.named.text, g_mailbox.slot[1].i,
                                    g_mailbox.slot[2].i, g_mailbox.slot[3].i, g_mailbox.slot[4].i);
            g_textPending = 0;
            return result;
        }
        if (wParam == SND_PLAY_B && g_sound3d.playB) {
            if (g_logEnabled) {
                char logText[256];
                sprintf(logText, "PlayB(%d,%s,%d,%d,%d,%d)", g_mailbox.slot[0].i, g_mailbox.named.text,
                        g_mailbox.slot[1].i, g_mailbox.slot[2].i, g_mailbox.slot[3].i, g_mailbox.slot[4].i);
                LogLine(logText);
            }
            result = g_sound3d.playB(g_mailbox.slot[0].i, g_mailbox.named.text, g_mailbox.slot[1].i,
                                     g_mailbox.slot[2].i, g_mailbox.slot[3].i, g_mailbox.slot[4].i);
            g_textPending = 0;
            return result;
        }
        if (wParam == SND_FADE_OUT_EX && g_sound3d.fadeOutEx) {
            if (g_logEnabled) {
                char logText[256];
                sprintf(logText, "FadeOutEx(%d,%d)", g_mailbox.slot[0].i, g_mailbox.slot[1].i);
                LogLine(logText);
            }
            result = g_sound3d.fadeOutEx(g_mailbox.slot[0].i, g_mailbox.slot[1].i);
            return result;
        }
        if (wParam == SND_PLAY_STOP && g_sound3d.playStop) {
            if (g_logEnabled) {
                char logText[256];
                sprintf(logText, "PlayStop(%d)", g_mailbox.slot[0].i);
                LogLine(logText);
            }
            result = g_sound3d.playStop(g_mailbox.slot[0].i);
            return result;
        }
        if (wParam == SND_VOL_FADE && g_sound3d.volFade) {
            if (g_logEnabled) {
                char logText[256];
                sprintf(logText, "VolFade(%d,%d,%d)", g_mailbox.slot[0].i, g_mailbox.slot[1].i,
                        g_mailbox.slot[2].i);
                LogLine(logText);
            }
            g_sound3d.volFade(g_mailbox.slot[0].i, g_mailbox.slot[1].i, g_mailbox.slot[2].i);
            return result;
        }
        if (wParam == SND_STOP_ALL_SE && g_sound3d.stopAllSE) {
            if (g_logEnabled) {
                char logText[256];
                sprintf(logText, "StopAllSE()");
                LogLine(logText);
            }
            return g_sound3d.stopAllSE();
        }
        if (wParam == SND_STOP_SE && g_sound3d.stopSE) {
            if (g_logEnabled) {
                char logText[256];
                sprintf(logText, "StopSE(%d)", lParam);
                LogLine(logText);
            }
            return g_sound3d.stopSE(lParam);
        }
        if (wParam == SND_GET_POS && g_sound3d.getPos) {
            if (g_logEnabled) {
                char logText[256];
                sprintf(logText, "GetPos(%d)", lParam);
                LogLine(logText);
            }
            return g_sound3d.getPos(lParam);
        }
        if (wParam == SND_SET_SE_VOL && g_sound3d.setSEVol) {
            if (g_logEnabled) {
                char logText[256];
                sprintf(logText, "SetSEVol(%d)", lParam);
                LogLine(logText);
            }
            return g_sound3d.setSEVol(lParam);
        }
        if (wParam == SND_SET_BGM_VOL && g_sound3d.setBGMVol) {
            if (g_logEnabled) {
                char logText[256];
                sprintf(logText, "SetBGMVol(%d)", lParam);
                LogLine(logText);
            }
            return g_sound3d.setBGMVol(lParam);
        }
        if (wParam == SND_SET_MEM && g_sound3d.setMem) {
            if (g_logEnabled) {
                char logText[256];
                sprintf(logText, "SetMem(%d,%d)", g_mailbox.slot[0].i, g_mailbox.slot[1].i);
                LogLine(logText);
            }
            result = g_sound3d.setMem(g_mailbox.slot[0].i, g_mailbox.slot[1].i);
            return result;
        }
        if (wParam == SND_SET_LISTEN && g_sound3d.setListen) {
            if (g_logEnabled) {
                char logText[256];
                sprintf(logText, "SetListen(%f,%f,%f,%f,%f,%f,%f,%f,%f,%f)", g_mailbox.slot[9].f,
                        g_mailbox.slot[0].f, g_mailbox.slot[1].f, g_mailbox.slot[2].f, g_mailbox.slot[3].f,
                        g_mailbox.slot[4].f, g_mailbox.slot[5].f, g_mailbox.slot[6].f, g_mailbox.slot[7].f,
                        g_mailbox.slot[8].f);
                LogLine(logText);
            }
            result = g_sound3d.setListen(g_mailbox.slot[9].f, g_mailbox.slot[0].f, g_mailbox.slot[1].f,
                                         g_mailbox.slot[2].f, g_mailbox.slot[3].f, g_mailbox.slot[4].f,
                                         g_mailbox.slot[5].f, g_mailbox.slot[6].f, g_mailbox.slot[7].f,
                                         g_mailbox.slot[8].f);
            return result;
        }
        if (wParam == SND_SET_SE_POS && g_sound3d.setSEPos) {
            if (g_logEnabled) {
                char logText[256];
                sprintf(logText, "SetSEPos(%d,%f,%f,%f,%f,%f,%f)", g_mailbox.slot[0].i, g_mailbox.slot[1].f,
                        g_mailbox.slot[2].f, g_mailbox.slot[3].f, g_mailbox.slot[4].f, g_mailbox.slot[5].f,
                        g_mailbox.slot[6].f);
                LogLine(logText);
            }
            result = g_sound3d.setSEPos(g_mailbox.slot[0].i, &g_mailbox.slot[1].f, &g_mailbox.slot[2].f,
                                        &g_mailbox.slot[3].f, &g_mailbox.slot[4].f, &g_mailbox.slot[5].f,
                                        &g_mailbox.slot[6].f);
            return result;
        }
        if (wParam == SND_CLIENT_WINDOW)
            g_clientWindow = (HWND)lParam;
        if (wParam == SND_SET_CLIENT) {
            if (g_clientWindow == 0)
                g_clientWindow = (HWND)lParam;
            if ((LPARAM)g_clientWindow == lParam && g_sound3d.module == 0) {
                if (g_logEnabled) {
                    char logText[256];
                    sprintf(logText, "Init(%d)", lParam);
                    LogLine(logText);
                }
                g_textPending = 0;
                LoadSound3D(lParam);
            }
            return (LPARAM)g_clientWindow == lParam;
        }
        }
        break;

    case WM_COPYDATA:
        {
            COPYDATASTRUCT *copyData = (COPYDATASTRUCT *)lParam;

            if (copyData && copyData->lpData) {
                memcpy(g_mailbox.named.text, copyData->lpData, copyData->cbData);
                g_textPending = 1;
            }
        }
        break;

    case WM_COMMAND:
        wmId = LOWORD(wParam);
        wmEvent = HIWORD(wParam);
        switch (wmId) {
        case IDM_PAUSE_TOGGLE:
            g_logPaused = !g_logPaused;
            break;
        case IDM_LOG_TOGGLE:
            g_logEnabled = !g_logEnabled;
            break;
        case IDM_ABOUT:
            DialogBox(g_instance, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, (DLGPROC)About);
            break;
        case IDM_EXIT:
            if (g_clientWindow) {
                int clientVisible = IsWindowVisible(g_clientWindow);
                int clientExists = IsWindow(g_clientWindow);

                if (clientVisible && clientExists) {
                    char clientTitle[256];
                    char message[256];

                    GetWindowText(g_clientWindow, clientTitle, 256);
                    sprintf(message, "\x22%s\x22\x82\xaa\x8b\x4e\x93\xae\x92\x86\x82\xcd\x8f\x49\x97\xb9\x82\xc5\x82\xab\x82\xdc\x82\xb9\x82\xf1\x81\x42",
                            clientTitle);
                    MessageBox(hWnd, message, "\x8a\x6d\x94\x46", MB_ICONEXCLAMATION);
                } else {
                    DestroyWindow(hWnd);
                }
            } else {
                DestroyWindow(hWnd);
            }
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;

    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        {
            RECT rt;

            GetClientRect(hWnd, &rt);
        }
        EndPaint(hWnd, &ps);
        break;

    case WM_DESTROY:
        if (g_sound3d.module) {
            g_sound3d.release();
            ReleaseSound3D();
        }
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Dialog procedure of the About box: closes it on OK or Cancel. Standard template. Used by WndProc (IDM_ABOUT).
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_INITDIALOG:
        return TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }
        break;
    }
    return FALSE;
}

// Loads sound3d.dll from the folder of the executable and resolves every I... entry point into g_sound3d, then
// calls IInit(clientParam) and MixOpen. Shows an error box when ISetSEPos is missing (library version mismatch).
// Returns 0 when the DLL cannot be loaded, else 1. Called by WndProc on the first client contact (SND_SET_CLIENT).
int LoadSound3D(int clientParam)
{
    HMODULE library;
    int unused1 = 0;  // set and never read: the unoptimised build still stores both, so the source had them
    int unused2 = 0;
    char path[256];
    int length;
    int initResult;

    GetModuleFileName(NULL, path, 256);
    length = strlen(path) - 1;
    while (path[length] != '\\') {
        path[length] = 0;
        length--;
    }
    strcat(path, "sound3d.dll");
    library = LoadLibrary(path);
    g_sound3d.module = library;
    if (!library)
        return 0;

    g_sound3d.init = (PFN_INIT)GetProcAddress(library, "IInit");
    g_sound3d.reInit = (PFN_INIT)GetProcAddress(library, "IReInit");
    g_sound3d.playSE = (PFN_PLAY_SE)GetProcAddress(library, "IPlaySE");
    g_sound3d.playSEB = (PFN_PLAY_SE)GetProcAddress(library, "IPlaySEB");
    g_sound3d.playSEEx = (PFN_PLAY_SE)GetProcAddress(library, "IPlaySEEx");
    g_sound3d.playFade = (PFN_PLAY)GetProcAddress(library, "IPlayFade");
    g_sound3d.playFadeEx = (PFN_PLAY_EX)GetProcAddress(library, "IPlayFadeEx");
    g_sound3d.play = (PFN_PLAY)GetProcAddress(library, "IPlay");
    g_sound3d.playFadeB = (PFN_PLAY)GetProcAddress(library, "IPlayFadeB");
    g_sound3d.playFadeBEx = (PFN_PLAY_EX)GetProcAddress(library, "IPlayFadeBEx");
    g_sound3d.playB = (PFN_PLAY)GetProcAddress(library, "IPlayB");
    g_sound3d.playStop = (PFN_INT1)GetProcAddress(library, "IPlayStop");
    g_sound3d.setBGMVol = (PFN_INT1)GetProcAddress(library, "ISetBGMVol");
    g_sound3d.setSEVol = (PFN_INT1)GetProcAddress(library, "ISetSEVol");
    g_sound3d.fadeOut = (PFN_INT1)GetProcAddress(library, "IFadeOut");
    g_sound3d.fadeOutEx = (PFN_INT2)GetProcAddress(library, "IFadeOutEx");
    g_sound3d.setMem = (PFN_INT2)GetProcAddress(library, "ISetMem");
    g_sound3d.setSEPos = (PFN_SET_SE_POS)GetProcAddress(library, "ISetSEPos");
    if (g_sound3d.setSEPos == 0)
        MessageBox(NULL,
                   "\x83\x89\x83\x43\x83\x75\x83\x89\x83\x8a\x82\xcc\x83\x6f\x81\x5b\x83\x57\x83\x87\x83\x93\x82\xaa\x88\xea\x92\x76\x82\xb5\x82\xdc\x82\xb9\x82\xf1",
                   "\x83\x47\x83\x89\x81\x5b", MB_OK);
    g_sound3d.stopAllSE = (PFN_VOID)GetProcAddress(library, "IStopAllSE");
    g_sound3d.loop = (PFN_VOID)GetProcAddress(library, "ILoop");
    g_sound3d.enter = (PFN_VOID)GetProcAddress(library, "IEnter");
    g_sound3d.leave = (PFN_VOID)GetProcAddress(library, "ILeave");
    g_sound3d.release = (PFN_VOID)GetProcAddress(library, "IRelease");
    g_sound3d.timeGetTime = (PFN_VOID)GetProcAddress(library, "ITimeGetTime");
    g_sound3d.getPos = (PFN_INT1)GetProcAddress(library, "IGetPos");
    g_sound3d.mixOpen = (PFN_VOID)GetProcAddress(library, "MixOpen");
    g_sound3d.mixClose = (PFN_VOID)GetProcAddress(library, "MixClose");
    g_sound3d.getParam = (PFN_VOID)GetProcAddress(library, "GetParam");
    g_sound3d.setParam = (PFN_VOID)GetProcAddress(library, "SetParam");
    g_sound3d.setMap = (PFN_VOID)GetProcAddress(library, "SetMap");
    g_sound3d.setListen = (PFN_SET_LISTEN)GetProcAddress(library, "ISetListen");
    g_sound3d.eax = (PFN_VOID)GetProcAddress(library, "EAX");
    g_sound3d.setLoopPos = (PFN_VOID)GetProcAddress(library, "ISetLoopPos");
    g_sound3d.reset = (PFN_VOID)GetProcAddress(library, "IReset");
    g_sound3d.stopSE = (PFN_INT1)GetProcAddress(library, "IStopSE");
    g_sound3d.setSEVolume = (PFN_VOID)GetProcAddress(library, "ISetSEVolume");
    g_sound3d.volFade = (PFN_INT3)GetProcAddress(library, "IVolFade");

    initResult = 0;
    if (g_sound3d.init)
        initResult = g_sound3d.init(clientParam);
    g_sound3d.mixOpen();
    return 1;
}

// Closes sound3d.dll: MixClose, IRelease, FreeLibrary (only when it is loaded) and clears the handle. Always
// returns 1. Called by WinMain at exit and by WndProc on WM_DESTROY.
int ReleaseSound3D(void)
{
    if (g_sound3d.module) {
        g_sound3d.mixClose();
        g_sound3d.release();
        FreeLibrary(g_sound3d.module);
    }
    g_sound3d.module = 0;
    return 1;
}
