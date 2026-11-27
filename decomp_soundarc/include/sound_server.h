// sound_server.h : types, constants and globals of SoundArc, the sound server of Gurumin.
//
// SoundArc is a single-instance tray program. The game (the "client") finds its window, writes the arguments of a
// command into a shared mailbox with WM_USER messages and then sends the command number; the server forwards each
// command to sound3d.dll and, when c:\sound.log exists, logs it.
#ifndef SOUND_SERVER_H
#define SOUND_SERVER_H

#include <windows.h>

// ---- resource IDs (SoundArc.rc) -----------------------------------------------------------------------------

const int IDD_ABOUTBOX = 103;        // About dialog (same number as IDS_APP_TITLE: different resource kinds)
const int IDS_APP_TITLE = 103;       // window title string
const int IDM_ABOUT = 104;           // tray menu: About
const int IDM_EXIT = 105;            // tray menu: Exit
const int IDS_HELLO = 106;           // template string loaded by the window procedure, unused
const int IDI_SOUNDARC = 107;        // tray icon
const int IDI_SMALL = 108;           // small class icon
const int IDC_SOUNDARC = 109;        // window class name string and menu
const int IDM_LOG_TOGGLE = 130;      // tray menu: switch logging to c:\sound.log
const int IDM_PAUSE_TOGGLE = 0x8003; // tray menu: second toggle item

// ---- window messages ----------------------------------------------------------------------------------------

const UINT WM_TRAYICON = WM_APP + 100;   // 0x8064, callback message of the tray icon
const UINT WM_SOUND_COMMAND = WM_USER;   // 0x400, command from the game (wParam = command or mailbox slot)
const UINT TRAY_ICON_ID = 1;             // uID of the tray icon

const UINT TIMER_ID_POLL = 10;           // SetTimer id
const UINT TIMER_POLL_MS = 100;          // SetTimer period

// ---- mailbox protocol ---------------------------------------------------------------------------------------
//
// WM_SOUND_COMMAND with wParam < MAILBOX_SLOT_COUNT stores lParam into mailbox slot wParam. A command is a wParam
// >= MAILBOX_SLOT_COUNT. Slots 0..9 hold the numeric arguments; WM_COPYDATA fills the text area behind the 256
// argument slots with a file name.

const int MAILBOX_SLOT_COUNT = 0x200;
const int MAILBOX_ARG_COUNT = 0x100;
const int MAILBOX_TEXT_SIZE = 0x400;

union MailboxSlot {
    int i;      // integer argument
    float f;    // floating point argument (the same four bytes)
};

struct MailboxText {
    MailboxSlot arg[MAILBOX_ARG_COUNT];   // numeric arguments
    char text[MAILBOX_TEXT_SIZE];         // file name sent with WM_COPYDATA
};

union Mailbox {
    MailboxSlot slot[MAILBOX_SLOT_COUNT]; // every slot, as addressed by wParam
    MailboxText named;                    // the same memory split into arguments and text
};

// Command numbers (wParam of WM_SOUND_COMMAND when >= MAILBOX_SLOT_COUNT).
const WPARAM SND_PLAY_FADE = 0x200;
const WPARAM SND_FADE_OUT = 0x201;
const WPARAM SND_PLAY_FADE_B = 0x202;
const WPARAM SND_PLAY_SE = 0x203;
const WPARAM SND_PLAY_SE_B = 0x204;
const WPARAM SND_PLAY_FADE_EX = 0x205;
const WPARAM SND_PLAY_FADE_B_EX = 0x206;
const WPARAM SND_PLAY = 0x207;
const WPARAM SND_PLAY_B = 0x208;
const WPARAM SND_FADE_OUT_EX = 0x212;
const WPARAM SND_PLAY_STOP = 0x213;
const WPARAM SND_VOL_FADE = 0x21c;
const WPARAM SND_STOP_ALL_SE = 0x221;
const WPARAM SND_STOP_SE = 0x224;
const WPARAM SND_GET_POS = 0x225;
const WPARAM SND_SET_SE_VOL = 0x226;
const WPARAM SND_SET_BGM_VOL = 0x227;
const WPARAM SND_SET_MEM = 0x230;
const WPARAM SND_SET_LISTEN = 0x235;
const WPARAM SND_SET_SE_POS = 0x267;
const WPARAM SND_SET_CLIENT = 0x400;     // first contact: remember the client window and load sound3d.dll
const WPARAM SND_CLIENT_WINDOW = 0x401;  // replace the remembered client window

// ---- sound3d.dll entry points -------------------------------------------------------------------------------
//
// All __cdecl. Resolved by name in LoadSound3D; a null pointer means the DLL lacks the export.

typedef int (__cdecl *PFN_INIT)(int);
typedef int (__cdecl *PFN_VOID)(void);
typedef int (__cdecl *PFN_INT1)(int);
typedef int (__cdecl *PFN_INT2)(int, int);
typedef int (__cdecl *PFN_INT3)(int, int, int);
typedef int (__cdecl *PFN_PLAY)(int, const char *, int, int, int, int);
typedef int (__cdecl *PFN_PLAY_EX)(int, const char *, int, int, int, int, int);
typedef int (__cdecl *PFN_PLAY_SE)(const char *, float *, float *, float *, int, int, float);
typedef int (__cdecl *PFN_SET_SE_POS)(int, float *, float *, float *, float *, float *, float *);
typedef int (__cdecl *PFN_SET_LISTEN)(float, float, float, float, float, float, float, float, float, float);

struct Sound3dApi {
    PFN_INIT init;                  // IInit
    PFN_INIT reInit;                // IReInit
    PFN_PLAY_SE playSE;             // IPlaySE
    PFN_PLAY_SE playSEB;            // IPlaySEB
    PFN_PLAY_SE playSEEx;           // IPlaySEEx
    PFN_VOID setMap;                // SetMap
    PFN_PLAY playFade;              // IPlayFade
    PFN_PLAY_EX playFadeEx;         // IPlayFadeEx
    PFN_PLAY play;                  // IPlay
    PFN_PLAY playFadeB;             // IPlayFadeB
    PFN_PLAY_EX playFadeBEx;        // IPlayFadeBEx
    PFN_PLAY playB;                 // IPlayB
    PFN_INT1 playStop;              // IPlayStop
    PFN_INT1 stopSE;                // IStopSE
    PFN_INT1 setBGMVol;             // ISetBGMVol
    PFN_INT1 setSEVol;              // ISetSEVol
    PFN_INT1 fadeOut;               // IFadeOut
    PFN_VOID eax;                   // EAX
    PFN_VOID stopAllSE;             // IStopAllSE
    PFN_VOID loop;                  // ILoop
    PFN_VOID enter;                 // IEnter
    PFN_VOID leave;                 // ILeave
    PFN_VOID release;               // IRelease
    PFN_VOID timeGetTime;           // ITimeGetTime
    PFN_INT1 getPos;                // IGetPos
    PFN_VOID setLoopPos;            // ISetLoopPos
    PFN_VOID setSEVolume;           // ISetSEVolume
    PFN_INT3 volFade;               // IVolFade
    PFN_SET_SE_POS setSEPos;        // ISetSEPos
    PFN_SET_LISTEN setListen;       // ISetListen
    PFN_INT2 fadeOutEx;             // IFadeOutEx
    PFN_INT2 setMem;                // ISetMem
    PFN_VOID mixOpen;               // MixOpen
    PFN_VOID reset;                 // IReset
    PFN_VOID mixClose;              // MixClose
    PFN_VOID getParam;              // GetParam
    PFN_VOID setParam;              // SetParam
    HMODULE module;                 // sound3d.dll, null while not loaded
};

// ---- globals ------------------------------------------------------------------------------------------------

extern char g_mutexName[];                // "SoundArc", the single-instance mutex
extern NOTIFYICONDATA g_trayIcon;         // the tray icon
extern HWND g_mainWindow;                 // the hidden main window
extern char g_windowClassName[100 + 4];   // loaded from IDC_SOUNDARC
extern char g_windowTitle[100];           // loaded from IDS_APP_TITLE
extern HINSTANCE g_instance;
extern Mailbox g_mailbox;
extern HWND g_clientWindow;               // the game's window; the server quits when it goes away
extern int g_logEnabled;                  // c:\sound.log exists
extern int g_logPaused;                   // second tray menu toggle
extern int g_textPending;                 // a WM_COPYDATA file name arrived and is not consumed yet
extern Sound3dApi g_sound3d;

// ---- functions ----------------------------------------------------------------------------------------------

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
void PumpOneMessage(void);
ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow);
int LogLine(const char *line);
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
int LoadSound3D(int clientParam);
int ReleaseSound3D(void);

// Empty class of the application skeleton. Constructor returns this, destructor does nothing. Never called.
class SoundArcApp {
public:
    SoundArcApp();
    ~SoundArcApp();
};


#endif
