// Globals and callbacks of the save/load/quit menu (Menu_SaveLoad, src/menu_save_load.cpp).
// This is the one declaration of each of them: include this header, never redeclare one.
#ifndef GURUMIN_MENU_SAVE_LOAD_H
#define GURUMIN_MENU_SAVE_LOAD_H

#include "scene_draw.h"

// Save slots: seven per page of the slot list, in files gdata00.sav, gdata01.sav, ...
#define SLOTS_PER_PAGE 7
#define SAVE_NAME_FORMAT "gdata%02d.sav"
// The preview picture at the start of a save file: 256 x 256 pixels of 3 bytes each.
#define SAVE_PREVIEW_SIZE 256
#define SAVE_PREVIEW_BYTES 0x30000

// Menu window ids used by the save/load menu.
enum SaveLoadWindow
{
    WINDOW_MESSAGE = 0x64,        // the caption window
    WINDOW_YESNO = 0x216,         // the yes/no dialog
    WINDOW_YESNO_PAGE = 0x218,    // closed together with the yes/no dialog
    WINDOW_COMMANDS = 0x222,      // the list of commands (save, load, title, quit)
    WINDOW_PAGE_A = 0x22b,        // closed when a save is loaded
    WINDOW_SLOTS = 0x22c,         // the list of seven save slots
    WINDOW_PAGE_B = 0x22d         // closed when a save is loaded
};

extern "C" int DAT_1030c504;                  // set when the quit/title question is answered "no" (closes the dialog)
extern "C" int DAT_1030c508;                  // set when the quit question is answered "yes": closes the game
extern "C" int DAT_1030c52c;                  // non-zero while the save/load menu is running
extern "C" int DAT_1030c540;                  // set to rebuild the save slot list (also the "no" of the save questions)
extern "C" int DAT_1030c544;                  // set when a save slot was picked in the slot list
extern "C" int DAT_1030c548;                  // set when the overwrite question was answered "yes"
extern "C" int DAT_1030c54c;                  // set when the slot list was cancelled
extern "C" int DAT_1030c550;                  // set when the "return to the title" question was answered "yes"
extern "C" int DAT_1030c558;                  // set when the command list was cancelled
extern "C" int DAT_1030c55c;                  // set when the command list was cancelled (second kind)
extern "C" int DAT_1030c56c;                  // flag the command entries set
extern "C" int DAT_1030c570;                  // result of the command list
extern "C" int DAT_1030c574;                  // set when the "quit" command was picked
extern "C" int DAT_1030c578;                  // set when the "return to the title" command was picked
extern "C" int DAT_1030c770;                  // flag of an empty load slot (never set)
extern "C" int DAT_1030c7e8;                  // set when a save slot was picked in the load list
extern "C" int DAT_1030c7ec;                  // set to rebuild the load slot list (also the "no" of the load question)
extern "C" int DAT_1030c7f0;                  // set when the load question was answered "yes"
extern "C" int DAT_1030c84c;                  // set when the "save" command was picked
extern "C" int DAT_1030c900;
extern "C" int DAT_103086a8;
extern "C" int DAT_1020c338;
extern "C" int DAT_1021cff8;
extern "C" int DAT_1042de20;                  // the map the loaded save was made on
extern "C" int DAT_104c02c0;                  // page of the slot list (seven slots per page)
extern "C" int DAT_104c02e8;                  // the last slot saved to plus one (0 = none); saved as "LSF" in the config file
extern "C" int DAT_10c6337c;                  // non-zero = the menu scene is drawn dark
extern "C" int DAT_10c63380;
extern "C" int DAT_10c646e4;                  // non-zero = the save was triggered by clearing the game
extern "C" int DAT_10c66d18;
extern "C" char DAT_10f65370[];               // the config file name

extern "C" int FUN_1003b180(int cursor, char *caption);       // 0x1003b180: loads the preview picture of the slot under the cursor
extern "C" int FUN_100106d0(int cursor, char *caption);       // 0x100106d0 (app_0010.cpp): remembers the command under the cursor
extern "C" void FUN_1014b480(void);                           // 0x1014b480 (Render_ResetSwapTarget, render_reset.cpp)
extern "C" int FUN_100106a0(void);                            // 0x100106a0 (Dialog_CloseAll)
extern "C" int FUN_10010520(const char *message, int yesValue, int noValue);   // 0x10010520 (Dialog_YesNo)
extern "C" int FUN_100105e0(const char *message, int yesValue, int noValue);   // 0x100105e0
int FUN_10010d10(const char *name, int *map, int *playTime, char *playerName, int *costume);   // 0x10010d10 (Save_ReadSlotInfo, C++ linkage)
int FUN_1005cdc0(const char *name, int mode);                 // 0x1005cdc0 (Save_LoadSlot, C++ linkage)
extern "C" int FUN_1003b260(const char *name);                // 0x1003b260
extern "C" int FUN_1015fe20(const char *file, const char *section, const char *key, int value);   // Config_SetInt
extern "C" int FUN_1015fea0(const char *path, int index, unsigned int value);                       // SystemSav_WriteRecord

extern "C" int FUN_10064630(void);   // 0x10064630 (Menu_SaveLoad)

int FUN_10011240(const char *name, void *thumbnail);          // 0x10011240 (Save_ReadThumbnail, save_slot.cpp, C++ linkage)
extern "C" int FUN_10174c70(int slot, int force);             // 0x10174c70 (Texture_Release)
extern "C" int FUN_10175c90(const char *name, int width, int height, const unsigned char *pixels, int alpha, int a);   // 0x10175c90 (Texture_CreateFromPixels)

#endif
