#pragma once

#include "object.h"
#include "node.h"
#include "map_tables.h"

struct FontRenderer;   // font_renderer.h
struct Tracker;        // tracker.h

// Event trigger (0xc8 bytes): runs when its map matches and all four script flags pass.
struct GameTrigger
{
    int map;                    // map number, 9999 = any map
    int flags[4];               // script flags that must be set (negative = must be clear, 0 = no condition)
    int value;                  // 0x14: event id (for map events)
    char name[0x10];            // 0x18: event name, e.g. "GAMEOVER", "KSKON", "TALK", "OPEN"
    union
    {
        struct
        {
            int argument;       // 0x28: what the event reacts to (for example the character or object id)
            int ids[5];         // 0x2c: with `argument`, the six ids a "DEAD" event waits for
            int extra[2];       // 0x40
        };
        int values[8];          // 0x28: the numbers of an event line (map events read six, triggers eight)
    };
    char labels[8][0x10];       // 0x48: the string read before each number
};

// Per-window entry table: 0x80 menu entries (a pointer to the variable an entry sets, or for value entries the
// value itself), then the pointer that receives the window's chosen result (entries[MENU_RESULT]) and one more.
enum { MENU_RESULT = 0x80 };

struct GameMenuItems
{
    int *entries[0x82];
};

// Menu control state the game keeps current (Game::controls): one byte per control, non-zero while it is pressed.
struct MenuControls
{
    char pad_0000[4];
    char previousPage;          // 0x04: page left
    char nextPage;              // 0x05: page right
    char pad_0006[2];
    char select;                // 0x08: choose the entry under the cursor
    char cancel;                // 0x09
    char pad_000a[8];
    char left;                  // 0x12
    char right;                 // 0x13
    char up;                    // 0x14
    char down;                  // 0x15
    char hold;                  // 0x16: while held the direction keys are ignored in grid windows
    char confirm;               // 0x17: grid windows: the confirm result
    char alternate;             // 0x18
};

// Scroll marks of the window lists, Shift-JIS: filled while the blink frame is at most 5, outlined after that.
#define MARK_UP "£"              // filled up triangle
#define MARK_UP_OUTLINE "¢"      // outlined up triangle
#define MARK_DOWN "¥"            // filled down triangle
#define MARK_DOWN_OUTLINE "¤"    // outlined down triangle

// Map table entry (0xe8 bytes): map number, four script-flag conditions and the map's names.
struct GameNamedEntry
{
    int map;                    // map number (9999 = any)
    int flags[4];               // script flags that must pass (see TestScriptFlag)
    char name[0x20];            // 0x14: map name used by scripts
    char file[0x20];            // 0x34
    char title[0x40];           // 0x54
    int m_0094[3];              // 0x94
    int m_00a0;
    int value;                  // 0xa4
    char text[2][0x20];         // 0xa8
};

// The game object: global instance DAT_104b7200. Holds menu textures, HUD state, the script variable tables and
// 32 node bodies for menu models.
// Number of "#Script" sections the map data can hold (Game::scripts).
enum { SCRIPT_LIMIT = 0x1000 };

struct Game : Object
{
    int m_0000;
    int m_0004;
    int texMenu1;
    int texMenu4;
    int m_0010;
    int texMenu2;
    int texMenu5;
    int texBalloon5;
    int texBalloon6;
    int texFrame;
    int m_0028;
    int texMap01;
    int texMap02;
    int texUdChip;
    int texWaku01;
    int texWaku02;
    int texWaku03;
    int texWaku04;
    int texIcon01;
    int texWaku05;
    char pad_0050[0x2c];
    int texBalloon7;
    int texMenu15;
    int texMenu14;
    int m_0088;
    int m_008c[3];
    int m_0098[3];
    int m_00a4[3];              // 0xa4: id of the menu window in each slot
    int m_00b0[3];              // 0xb0: window x
    int m_00bc[3];              // 0xbc: window y
    int m_00c8[3];              // 0xc8: window width
    int m_00d4[3];              // 0xd4: window height
    int m_00e0[3];              // 0xe0: index of the last entry added to the window
    int m_00ec[3];
    int m_00f8[3];
    int m_0104;                 // 0x104: set to 10 when a window opens
    int m_0108;                 // 0x108: key repeat delay (0 while no direction is held)
    int m_010c;                 // 0x10c: part of the open window under the cursor (0 = list, 1 = previous page tab, 2 = next page tab)
    int m_0110[3];              // 0x110: 1 while the window slot is open
    int m_011c[3];
    int m_0128[3];
    int m_0134[3];
    int m_0140[3];              // 0x140: cursor position in the window (restored from m_0434 when remembered)
    int m_014c[3];              // 0x14c: rows of a grid window
    int m_0158[3];
    int m_0164[3];
    int m_0170[3];              // 0x170: column of the cursor in grid windows
    int m_017c[3];              // 0x17c: row of the cursor in grid windows
    int *m_0188[3];             // 0x188: set to 1 when the page number changes
    int *m_0194[3];             // 0x194: the page number a window shows (zero-based)
    int m_01a0[3];
    int m_01ac[3];
    int *m_01b8[3];             // 0x1b8: value drawn at (m_01c4, m_01d0) in the window
    int m_01c4[3];
    int m_01d0[3];
    char m_01dc[0x258];
    char m_0434[0x258];
    char m_068c[4];
    int (*m_0690)(int x, int y, int slot, float alpha);   // 0x690: draws the icon of a list entry (Item_DrawIcon while the item list is open)
    MenuControls *controls;     // 0x694
    Tracker *tracker;           // 0x698: mouse tracker
    char m_069c[3][0x80][0x36]; // 0x69c: per-window item labels (0x80 items of 0x36 characters)
    GameMenuItems m_579c[3];    // 0x579c: per-window entries and result pointer
    int m_5db4[3][0x82];        // 0x5db4: entry kind (0 = flag cleared on open, 1 = value, 2 = range, 5)
    int m_63cc[3][0x82];        // 0x63cc: range entries' upper value (-1 for the others)
    int m_69e4[3][0x82];        // 0x69e4: range entries' lower value
    int m_6ffc;
    float m_7000[3][0x82];      // 0x7000: per entry highlight (0 to 255)
    char pad_7618[0x18];
    int m_7630;                 // 0x7630: menu cursor target x
    int m_7634;                 // 0x7634: menu cursor target y
    int m_7638;                 // 0x7638: menu cursor x (follows the target)
    int m_763c;                 // 0x763c: menu cursor y
    int m_7640;
    int m_7644;
    int m_7648;
    int m_764c;
    int *m_7650;                // 0x7650: while it points at a non-zero flag the windows ignore input
    int (*m_7654[3])(int cursor, char *caption);    // 0x7654: per-window callback run when the cursor is over an entry (the save slot window loads the slot's preview, the command list notes the command); may write a caption
    void (*m_7660)(int);
    FontRenderer *textRenderer;   // 0x7664: draws the window marks (set by LoadMenuTextures)
    float m_7668[3];            // 0x7668: per-window fade
    char pad_7674[0x11f4];
    Node m_8868[0x20];          // 0x8868: menu model nodes, destroyed in ~Game
    char pad_39c68[0x2b2a8];    // 0x39c68: up to 0x64f10 the map tables are described by MapFront (map_front.h)
    union                       // 0x64f10: the battle table (32 groups) overlaps the three tables that follow its first four groups
    {
        MapBattleEntry battle[0x20][0x20];
        struct
        {
            MapBattleEntry battleHead[4][0x20];
            MapBgmEntry bgm[0x40];              // 0x66b10
            MapFaceFrame faceFrame[0x80];       // 0x67710
            MapSeEntry se[1100];                // 0x67910
        };
    };
    int mapVersion;             // 0x78c10
    char pad_78c14[0x44];
    MapBtMotEntry btMot[127];   // 0x78c58
    char pad_81d34[0xe0];
    MapAbilityEntry ability[0x80];  // 0x81e14
    MapUsdgEntry usdg[0x100];   // 0x93e14
    GameNamedEntry m_95e14[0x100];
    int m_a4614;
    MapCharacter charTable[160];        // 0xa4618
    int charCount;                      // 0xb2e98
    MapStatusEntry status[160];         // 0xb2e9c
    int pad_ba69c;
    MapCharPos charPos[256];            // 0xba6a0
    int charPosCount;                   // 0xc9aa0
    MapMonsterPos charMonsPos[1440];    // 0xc9aa4
    int charMonsPosCount;               // 0x11f724
    MapObjAnime objAnime[0x400];        // 0x11f728
    int objAnimeCount;                  // 0x153728
    GameTrigger triggers[3400]; // 0x15372c: event triggers of the loaded map
    int triggerCount;           // 0x1f976c
    GameTrigger queuedTriggers[2000];  // 0x1f9770: triggers of queued events (see IsQueuedEventReady)
    int queuedTriggerCount;     // 0x25b1f0
    GameTrigger mapEvents[2000];// 0x25b1f4: named map events
    int mapEventCount;          // 0x2bcc74
    int pad_2bcc78;
    MapCharMotion charMot[0x40];        // 0x2bcc7c
    char *scripts[SCRIPT_LIMIT];            // 0x3e997c: the "#Script" sections, allocated by LoadMapData
    char *text;                 // 0x3ed97c: script text being tokenised
    int pos;                    // 0x3ed980: read position in `text`
    int end;                    // 0x3ed984: text length
    char token[0x80];           // 0x3ed988: the token read last
    int runningEvents[0x100];   // 0x3eda08: the queued event ids being run (RunQueuedEvents)
    int queuedEvents[0x100][2]; // 0x3ede08: queued event ids (> 0) and whether each was handled (non-zero)
    char killed[0x80];          // 0x3ee608: 1 for each id killed (entry 0 is always set)
    int currentMap;             // 0x3ee688: number of the loaded map
    int pad_3ee68c[2];
    int m_3ee694[4000];

    ~Game();
    int FindSlot(int id);
    void M_10007f20();
    void M_100082e0();          // 0x100082e0: clears the three window slots' entry tables (Game_Reset)
    int LoadMenuTextures(FontRenderer *renderer);
    int LoadMapData(const char *path, void (*progress)(const char *, int));   // 0x10002440 (MapData_Load): bin\map.itm tables
    // Tests script flag `index` (4000 flags in m_3ee694, saved with the game). A negative index tests that the
    // flag is clear. Flag 0 is always set; out-of-range indexes read as clear (or as set for the negated test).
    // Inline: callers expand it (the out-of-line copy is at 0x100022b0).
    int TestScriptFlag(int index)
    {
        if (index < 0)
        {
            index = -index;
            if (index > 3999)
                return 1;
            return m_3ee694[index] == 0;
        }
        if (index > 3999)
            return 0;
        if (index == 0)
            return 1;
        return m_3ee694[index];
    }
    int IsQueuedEventReady();
    int RunQueuedEvents(int (*callback)(GameTrigger *trigger));
    int ForEachActiveTrigger(int (*callback)(GameTrigger *trigger));
    int RaiseKskonEvents(int argument);
    int RaiseTalkEvents(int argument);
    int RaiseOpenEvents(int argument);
    int RaiseFireEvents(int argument, int on);
    int RaiseLeverEvents(int argument, int on);
    int RaiseDoorEvents(int argument, int on);
    int RegisterEvent(int id);  // 0x100020d0 (flag_list.cpp: FlagOwner::M_100020d0)
    int RaiseGameOverEvents();
    int MarkKilled(int id);
    int FindMapEntry(int map, char *name, char *file, char *title, int *unused1, int *unused2, int *unused3, int *value);
    int SetScriptFlag(int index, int value);
    int ReadWord();
    int PointInRect(int x, int y, int left, int top, int right, int bottom);   // 0x10008430 (app_0003.cpp)
    int ReadWordInLine();
    int IsAnyWindowBusy();
    void CloseAllWindows();
    int SetWindowCallbackValue(int id, int (*callback)(int cursor, char *caption));
    int ConfirmWindow(int id);
    int SetWindowScroll(int id, int value);
    int SetWindowLastItem(int id, int count);
    int DrawPanel(int x, int y, int w, int h, float z, float alpha);
    int DrawPanelAlt(int x, int y, int w, int h, float z, float alpha);
    int DrawLabelledNumber(int value, int x, int y, float z, float fade, float scale, float alpha);  // 0x10008a30
    int DrawNumber(int value, int x, int y, float z, float fade, float scale, float alpha);     // 0x10008c40
    int DrawNumberAlt(int value, int x, int y, float z, float fade, float scale, float alpha);  // 0x10008df0
    int MoveCursor();  // 0x10009050 (game_panels.cpp)
    int UpdateWindows();  // 0x1000a0b0: advances the open menu windows by one frame (fades, cursor, entries)
    int DrawBalloon(int x, int y, int lit, int rows, int a, int b, float fade);  // fade: passed to DrawTinted, unused there
    int DrawCursor(int x, int y, float fade, int frame);
    int DrawWindowMarks(int frame);
    int DrawWindows(int frame);  // 0x100094e0 (game_draw_windows.cpp)
    int DrawDebugText(const char *text, int x, int y);
    void FreeScripts();  // 0x10002030: frees every loaded script section (app_0006.cpp)
    int FindConstant(const char *name);
    int CloseWindow(int id);
    int SetWindowCursor(int id, int value);
    int SetWindowLayout(int id, int a, int b, int *c, int *d);
    int SetWindowLayout2(int id, int a, int b, int *c);
    int OpenWindow(int id, int x, int y, int width, int height);  // 0x100084b0
    int SetWindowResult(int id, int *result);
    int AddValueEntry(int id, const char *label, int value);                    // 0x1000b060
    int AddFlagEntry(int id, const char *label, int *flag);                     // 0x1000b2c0
    int AddKind5Entry(int id, const char *label, int *flag);                    // 0x1000b4e0
    int AddRangeEntry(int id, const char *label, int low, int high, int *value);  // 0x1000b750
};

inline int Game::FindSlot(int id)
{
    int slot = 0;
    do
    {
        if (m_0110[slot] != 0 && m_00a4[slot] == id)
            break;
        slot++;
    } while (slot < 3);
    return slot;
}

// The game object.
extern "C" Game DAT_104b7200;
// A second object the script-flag code runs on with the Game layout (TestScriptFlag's out-of-line copy and
// RegisterEvent are called with this address). Its relation to DAT_104b7200 is not settled yet: the two addresses
// are 0x3bd648 bytes apart, less than the size of Game.
extern "C" Game DAT_10874848;
