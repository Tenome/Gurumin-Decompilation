// The save, load, quit and "return to the title" menu of the pause screen (Menu_SaveLoad).

#include <stdio.h>
#include <string.h>
#include <windows.h>

#include "vec4.h"
#include "matrix4.h"
#include "node.h"
#include "game.h"
#include "texture_object.h"
#include "doppler_loops.h"
#include "item_dictionary.h"
#include "save_summary.h"

#include "char_vars.h"
#include "game_vars.h"
#include "map_vars.h"
#include "menu_save_load.h"
#include "message_window.h"
#include "menu_scene.h"
#include "render_state.h"
#include "render_vars.h"
#include "ui_vars.h"
#include "win_vars.h"

// Shift-JIS texts: the questions of the dialogs and the lines of the slot list. "s064" and the like in the slot
// formats are the text engine's position tags.
#define TEXT_SAVE_HERE "\x82\xb1\x82\xb1\x82\xdc\x82\xc5\x82\xcc\x93\xe0\x97" "e\x82\xf0\x83Z\x81[\x83u\x82\xb5\x82\xdc\x82\xb7\x82\xa9\x81H"
#define TEXT_SAVE_CLEAR "\x83N\x83\x8a\x83" "A\x83" "f\x81[\x83^\x82\xf0\x83Z\x81[\x83u\x82\xb5\x82\xdc\x82\xb7\x82\xa9\x81H"
#define TEXT_ASK_LOAD "\x96{\x93\x96\x82\xc9\x83\x8d\x81|\x83h\x82\xb5\x82\xdc\x82\xb7\x82\xa9\x81H"
#define TEXT_ASK_TITLE "\x96{\x93\x96\x82\xc9\x83^\x83" "C\x83g\x83\x8b\x82\xc9\x96\xdf\x82\xe8\x82\xdc\x82\xb7\x82\xa9\x81H"
#define TEXT_ASK_QUIT "\x96{\x93\x96\x82\xc9\x8fI\x97\xb9\x82\xb5\x82\xdc\x82\xb7\x82\xa9\x81H"
#define TEXT_ASK_SAVE "\x96{\x93\x96\x82\xc9\x83Z\x81[\x83u\x82\xb5\x82\xdc\x82\xb7\x82\xa9\x81H"
#define SLOT_FORMAT "%ss064%02d\x81" "F%02d\x81" "F%02ds180%s"                         // name, hours, minutes, seconds, costume
#define SLOT_FORMAT_LAST "%ss064%02d\x81" "F%02d\x81" "F%02ds180%ss260\x81\x9a"               // the same, marked as the slot saved last
#define SLOT_FORMAT_CLEAR "\x81|\x82" "b\x82\x8c\x82\x85\x82\x81\x82\x92s060\x82" "c\x82\x81\x82\x94\x82\x81\x81|"             // "Clear Data"
#define SLOT_FORMAT_CLEAR_LAST "\x81|\x82" "b\x82\x8c\x82\x85\x82\x81\x82\x92s060\x82" "c\x82\x81\x82\x94\x82\x81\x81|s260\x81\x9a"
#define TEXT_SLOT_EMPTY "\x82" "d\x82\x8d\x82\x90\x82\x94\x82\x99"                      // "Empty"
#define TEXT_COMMAND_MARK "\x87@"
#define NAME_NORMAL "\x82m\x82\x8f\x82\x92\x82\x8d\x82\x81\x82\x8c"
#define NAME_HARD "\x82g\x82\x81\x82\x92\x82\x84"
#define NAME_BEGINNER "\x82" "a\x82\x85\x82\x87\x82\x89\x82\x8e\x82\x8e\x82\x85\x82\x92"
#define NAME_PLEASURE "\x82o\x82\x8c\x82\x85\x82\x81\x82\x93\x82\x95\x82\x92\x82\x85"
#define NAME_CRAZY "\x82" "b\x82\x92\x82\x81\x82\x9a\x82\x99"
#define SECONDS_PER_HOUR 3600
#define SECONDS_PER_MINUTE 60
#define SOUND_MENU_CANCEL 0x394
#define SOUND_MENU_SAVE 0x11d
#define MODE_PANEL_CLOSING 0x15
#define SCRATCH_SIZE 256
#define SYSTEM_RECORD_COSTUMES 0x80
#define QUIT_COMMAND_COUNT 5

// Length of the buffer for a save file name.
#define SAVE_NAME_LENGTH 0x100

// 0x1003b180. Callback of the save slot list (Game::m_7654): when the cursor moves onto another slot, loads that
// save's preview picture into the 256 x 256 texture "LOADPRE" (DAT_102024a0, which the dark panel draws), or only
// frees the texture (-1 = none) when the slot is empty or unreadable. `caption` is not used. Always returns 0.
extern "C" int FUN_1003b180(int cursor, char *caption)
{
    char name[SAVE_NAME_LENGTH];
    unsigned char preview[SAVE_PREVIEW_BYTES];
    if (DAT_104bff04 != cursor)
    {
        sprintf(name, SAVE_NAME_FORMAT, DAT_104c02c0 * SLOTS_PER_PAGE + cursor);
        if (FUN_10011240(name, preview) != 0)
        {
            FUN_10174c70(DAT_102024a0, 1);
            DAT_102024a0 = FUN_10175c90("LOADPRE", SAVE_PREVIEW_SIZE, SAVE_PREVIEW_SIZE, preview, 0, 0);
        }
        else
        {
            FUN_10174c70(DAT_102024a0, 1);
            DAT_102024a0 = -1;
        }
        DAT_104bff04 = cursor;
    }
    return 0;
}

// 0x10064630 (Menu_SaveLoad). Runs the save/load/quit menu. The first call after DAT_1030c568 is set opens the
// command list (window 0x222: DAT_104bfd20 is 0 for the quit menu, 1 for the save menu and 2 for the load menu).
// Later calls handle the flags the menus set: a quit confirmed (closes the game window), the cancel of the
// commands, a rebuild of the slot list (window 0x22c: seven saves per page, each with its player name, play time and
// costume) that first redraws the scene behind it, a picked slot with its overwrite question, the save itself
// (writes gdataNN.sav, remembers the slot as "LSF" in the config file and the costume bits in system.sav), the
// "return to the title" and "quit" questions, the load list and the load of a picked slot. Returns 1 when the menu
// was closed or opened this frame, else 0.
extern "C" int FUN_10064630(void)
{
    char path[200];
    int map;
    int playTime;
    char playerName[40];
    int costume;
    char scratch[SCRATCH_SIZE];

    if (DAT_1030c568 != 0)
    {
        int offset = 0;
        DAT_1030c568 = 0;
        if (DAT_104bfd20 == 1)
            offset = 0x18;
        else if (DAT_104bfd20 == 2)
            offset = 0x80;
        if (DAT_104bfd20 != 0)
        {
            DAT_104b7200.OpenWindow(WINDOW_COMMANDS, offset + 0x44, 0x72, 0x100, 0x9c);
            DAT_104b7200.ConfirmWindow(WINDOW_COMMANDS);
            DAT_104b7200.SetWindowScroll(WINDOW_COMMANDS, 0x16);
            if (DAT_104bfd20 == 1)
                DAT_104b7200.SetWindowLastItem(WINDOW_COMMANDS, 8);
            else if (DAT_104bfd20 == 2)
                DAT_104b7200.SetWindowLastItem(WINDOW_COMMANDS, 10);
        }
        else
        {
            DAT_104b7200.OpenWindow(WINDOW_COMMANDS, 0xc4, 0x7a, 0x100, 0x9c);
            DAT_104b7200.ConfirmWindow(WINDOW_COMMANDS);
            DAT_104b7200.SetWindowScroll(WINDOW_COMMANDS, 0x16);
            DAT_104b7200.SetWindowLastItem(WINDOW_COMMANDS, QUIT_COMMAND_COUNT);
        }
        DAT_104b7200.SetWindowCallbackValue(WINDOW_COMMANDS, FUN_100106d0);
        DAT_104b7200.SetWindowResult(WINDOW_COMMANDS, &DAT_1030c570);
        for (int i = 0; i < DAT_104bfd24; i++)
            DAT_104b7200.AddRangeEntry(WINDOW_COMMANDS, TEXT_COMMAND_MARK, 0, 0, &DAT_1030c56c);
        DAT_104bfd30 = DAT_104bfd24 - 1;
        DAT_104b7200.SetWindowCursor(WINDOW_COMMANDS, DAT_104bfd30);
        return 1;
    }
    if (DAT_1030c504 != 0)
    {
        DAT_1030c504 = 0;
        DAT_104b7200.CloseWindow(WINDOW_YESNO_PAGE);
        DAT_104b7200.CloseWindow(WINDOW_YESNO);
        DAT_1020e0f8[0] = 0;
        return 1;
    }
    if (DAT_1030c508 != 0)
    {
        DAT_1030c508 = 0;
        FUN_100106a0();
        MessageWindow_SetTitle(0);
        DAT_1030c504 = 0;
        DAT_104b7200.CloseWindow(WINDOW_MESSAGE);
        PostMessageA(DAT_10f6569c, WM_CLOSE, 0, 0);
    }
    if (DAT_1030c52c != 0 && DAT_1030c540 == 0 && DAT_1030c7ec == 0)
    {
        if (DAT_1030c558 == 0 && DAT_1030c55c == 0 && DAT_1030c578 == 0 && DAT_1030c574 == 0)
        {
            DAT_102028dc = MODE_PANEL_CLOSING;
            DAT_1030c52c = 0;
            DAT_1020c338 = 0;
            FUN_100106a0();
            DAT_104b7200.CloseWindow(WINDOW_MESSAGE);
            FUN_1003ad20(SOUND_MENU_CANCEL, 0, 1.0f);
        }
    }
    if (DAT_1030c558 != 0)
    {
        DAT_102028dc = MODE_PANEL_CLOSING;
        FUN_100106a0();
        DAT_104b7200.CloseWindow(WINDOW_MESSAGE);
        DAT_1020c338 = 0;
    }
    if (DAT_1030c55c != 0)
    {
        DAT_1020c338 = 0;
        DAT_102028dc = MODE_PANEL_CLOSING;
        FUN_100106a0();
        DAT_104b7200.CloseWindow(WINDOW_MESSAGE);
    }
    if (DAT_1030c540 != 0)
    {
        DAT_1020c338 = 0;
        DAT_10c66d18 = 0;
        DAT_104b7200.CloseWindow(WINDOW_MESSAGE);
        FUN_100106a0();
        FUN_1013f070(RENDER_DEPTH_TEST, 1, 0);
        DAT_118d0ba8[DAT_108360f8].UpdateImage(0);
        FUN_1013f070(RENDER_FLUSH_SCENE, 0, 0);
        if (DAT_10c6337c != 0)
            DAT_10da90d0.Set(0.0f, 0.0f, 0.0f, 1.0f);
        else
            DAT_10da90d0.Set(0.5f, 0.5f, 0.5f, 1.0f);
        DAT_1042da68 = 0;
        DAT_104c0220 = 1;
        if (DAT_10f65484 != 0)
        {
            DAT_10f65484 = 0;
            if (DAT_104bf8c0 == 1)
                Scene_DrawFrame(0);
            else
                Menu_DrawScene();
            FUN_1013f070(RENDER_FLUSH_SCENE, 0, 0);
            DAT_10f65484 = 1;
        }
        else
        {
            if (DAT_104bf8c0 == 1)
                Scene_DrawFrame(0);
            else
                Menu_DrawScene();
            FUN_1013f070(RENDER_FLUSH_SCENE, 0, 0);
        }
        DAT_104c0220 = 0;
        FUN_1013f070(RENDER_FLUSH_SCENE, 0, 0);
        FUN_10021e70();
        if (DAT_10c6337c != 0)
            DAT_10da90d0.Set(0.0f, 0.0f, 0.0f, 1.0f);
        else
            DAT_10da90d0.Set(1.0f, 1.0f, 1.0f, 1.0f);
        DAT_1042da68 = 0;
        FUN_1014b480();
        FUN_1013f070(RENDER_DEPTH_TEST, 1, 0);
        DAT_104bfd7c = 1;
        DAT_104bfd78 = 1;
        FUN_100106a0();
        DAT_1030c540 = 0;
        FUN_1000cb30("");
        FUN_1000cb90("");
        DAT_104b7200.OpenWindow(WINDOW_SLOTS, 0x1c, 0x74, 0x186, 0xcc);
        DAT_104b7200.SetWindowResult(WINDOW_SLOTS, &DAT_1030c54c);
        DAT_104b7200.m_01dc[WINDOW_SLOTS] = 1;
        DAT_104b7200.SetWindowCallbackValue(WINDOW_SLOTS, FUN_1003b180);
        if (DAT_104c02e8 != 0 && DAT_104c02ec == 0)
        {
            DAT_104c02c0 = (DAT_104c02e8 - 1) / SLOTS_PER_PAGE;
            DAT_104b7200.SetWindowCursor(WINDOW_SLOTS, (DAT_104c02e8 - 1) % SLOTS_PER_PAGE);
            DAT_104c02ec = 1;
        }
        DAT_104b7200.SetWindowLayout(WINDOW_SLOTS, 0, 0x13, &DAT_104c02c0, &DAT_1030c540);
        DAT_104bff04 = -1;
        DAT_104bff00 = 1;
        for (int i = 0; i < SLOTS_PER_PAGE; i++)
        {
            sprintf(path, SAVE_NAME_FORMAT, DAT_104c02c0 * SLOTS_PER_PAGE + i);
            FUN_10010d10(path, &map, &playTime, playerName, &costume);
            if (playerName[0] == 0)
                DAT_104b7200.AddFlagEntry(WINDOW_SLOTS, TEXT_SLOT_EMPTY, &DAT_1030c544);
            else
            {
                if (costume == 0)
                    strcpy(scratch, NAME_NORMAL);
                else if (costume == 1)
                    strcpy(scratch, NAME_BEGINNER);
                else if (costume == 2)
                    strcpy(scratch, NAME_HARD);
                else if (costume == 3)
                    strcpy(scratch, NAME_PLEASURE);
                else if (costume == 5)
                    strcpy(scratch, NAME_CRAZY);
                if (costume == 4)
                {
                    if (DAT_104c02e8 == 0 || DAT_104c02e8 - 1 != DAT_104c02c0 * SLOTS_PER_PAGE + i)
                        sprintf(path, SLOT_FORMAT_CLEAR, playerName, playTime / SECONDS_PER_HOUR,
                                (playTime % SECONDS_PER_HOUR) / SECONDS_PER_MINUTE, (playTime % SECONDS_PER_HOUR) % SECONDS_PER_MINUTE,
                                scratch);
                    else
                        sprintf(path, SLOT_FORMAT_CLEAR_LAST, playerName, playTime / SECONDS_PER_HOUR,
                                (playTime % SECONDS_PER_HOUR) / SECONDS_PER_MINUTE, (playTime % SECONDS_PER_HOUR) % SECONDS_PER_MINUTE,
                                scratch);
                }
                else
                {
                    if (DAT_104c02e8 == 0 || DAT_104c02e8 - 1 != DAT_104c02c0 * SLOTS_PER_PAGE + i)
                        sprintf(path, SLOT_FORMAT, playerName, playTime / SECONDS_PER_HOUR,
                                (playTime % SECONDS_PER_HOUR) / SECONDS_PER_MINUTE, (playTime % SECONDS_PER_HOUR) % SECONDS_PER_MINUTE,
                                scratch);
                    else
                        sprintf(path, SLOT_FORMAT_LAST, playerName, playTime / SECONDS_PER_HOUR,
                                (playTime % SECONDS_PER_HOUR) / SECONDS_PER_MINUTE, (playTime % SECONDS_PER_HOUR) % SECONDS_PER_MINUTE,
                                scratch);
                }
                DAT_104b7200.AddFlagEntry(WINDOW_SLOTS, path, &DAT_1030c544);
            }
        }
    }
    if (DAT_1030c544 != 0)
    {
        DAT_104bff04 = DAT_104b7200.m_0140[DAT_104b7200.m_0088];
        DAT_1030c544 = 0;
        FUN_10010520(TEXT_ASK_SAVE, (int)&DAT_1030c548, (int)&DAT_1030c540);
    }
    if (DAT_1030c548 != 0)
    {
        int slot;
        FUN_1003ad20(SOUND_MENU_SAVE, 0, 1.0f);
        slot = DAT_104bff04;
        DAT_1030c548 = 0;
        DAT_104b7200.CloseWindow(WINDOW_YESNO_PAGE);
        DAT_104b7200.CloseWindow(WINDOW_YESNO);
        DAT_1020e0f8[0] = 0;
        DAT_1030c540 = 1;
        sprintf(scratch, SAVE_NAME_FORMAT, DAT_104c02c0 * SLOTS_PER_PAGE + slot);
        FUN_1003b260(scratch);
        DAT_104c02e8 = DAT_104c02c0 * SLOTS_PER_PAGE + 1 + slot;
        FUN_1015fe20(DAT_10f65370, "CONFIG", "LSF", DAT_104c02e8);
        FUN_1015fea0(DAT_10256110, SYSTEM_RECORD_COSTUMES, (unsigned short)DAT_104c0230);
        return 1;
    }
    if (DAT_1030c54c != 0)
    {
        DAT_1030c54c = 0;
        DAT_104bfd78 = 0;
        DAT_104b7200.CloseWindow(WINDOW_SLOTS);
        FUN_1000cb30("");
        DAT_104b7200.CloseWindow(WINDOW_YESNO_PAGE);
        DAT_104b7200.CloseWindow(WINDOW_YESNO);
        DAT_1020e0f8[0] = 0;
        if (DAT_10c63384 != 0)
        {
            DAT_10c63380 = 0;
            DAT_10c6337c = 0;
            FUN_1000cb30("");
            FUN_1000cb90("");
            DAT_104bf8d4 = 1;
            if (DAT_10255f08 != 0)
            {
                Node *field = DAT_10255f08->Find("Field3", 0);
                if (field != 0)
                    field->ClearFlags(NODE_FLAG_HIDDEN);
            }
        }
        DAT_10c66d18 = 1;
        return 1;
    }
    if (DAT_1030c574 != 0)
    {
        DAT_1020c338 = 0;
        DAT_104bff04 = DAT_104b7200.m_0140[DAT_104b7200.m_0088];
        DAT_1030c574 = 0;
        FUN_1000cb30("");
        DAT_104bf8d4 = 1;
        FUN_100105e0(TEXT_ASK_QUIT, (int)&DAT_1030c508, (int)&DAT_1030c504);
    }
    if (DAT_1030c578 != 0)
    {
        DAT_1020c338 = 0;
        DAT_104bff04 = DAT_104b7200.m_0140[DAT_104b7200.m_0088];
        DAT_1030c578 = 0;
        FUN_1000cb30("");
        DAT_104bf8d4 = 1;
        FUN_10010520(TEXT_ASK_TITLE, (int)&DAT_1030c550, (int)&DAT_1030c504);
    }
    if (DAT_1030c7ec != 0)
    {
        FUN_100106a0();
        DAT_104b7200.CloseWindow(WINDOW_MESSAGE);
        DAT_104bff00 = 1;
        DAT_104bfd7c = 1;
        DAT_104bfd78 = 1;
        DAT_1030c7ec = 0;
        DAT_104b7200.OpenWindow(WINDOW_SLOTS, 0x7c, 0xf4, 0x186, 0xcc);
        DAT_104b7200.SetWindowResult(WINDOW_SLOTS, &DAT_1030c54c);
        DAT_104b7200.m_01dc[WINDOW_SLOTS] = 1;
        DAT_104b7200.SetWindowCallbackValue(WINDOW_SLOTS, FUN_1003b180);
        DAT_104b7200.SetWindowLayout(WINDOW_SLOTS, 0, 0x13, &DAT_104c02c0, &DAT_1030c7ec);
        DAT_104bff04 = -1;
        for (int i = 0; i < SLOTS_PER_PAGE; i++)
        {
            sprintf(path, SAVE_NAME_FORMAT, DAT_104c02c0 * SLOTS_PER_PAGE + i);
            FUN_10010d10(path, &map, &playTime, playerName, &costume);
            if (playerName[0] == 0)
                DAT_104b7200.AddFlagEntry(WINDOW_SLOTS, TEXT_SLOT_EMPTY, &DAT_1030c770);
            else
            {
                if (costume == 0)
                    strcpy(scratch, NAME_NORMAL);
                else if (costume == 1)
                    strcpy(scratch, NAME_BEGINNER);
                else if (costume == 2)
                    strcpy(scratch, NAME_HARD);
                else if (costume == 3)
                    strcpy(scratch, NAME_PLEASURE);
                else if (costume == 5)
                    strcpy(scratch, NAME_CRAZY);
                sprintf(path, SLOT_FORMAT, playerName, playTime / SECONDS_PER_HOUR,
                        (playTime % SECONDS_PER_HOUR) / SECONDS_PER_MINUTE, (playTime % SECONDS_PER_HOUR) % SECONDS_PER_MINUTE,
                        scratch);
                DAT_104b7200.AddFlagEntry(WINDOW_SLOTS, path, &DAT_1030c7e8);
            }
        }
    }
    if (DAT_1030c7e8 != 0)
    {
        DAT_104bff04 = DAT_104b7200.m_0140[DAT_104b7200.m_0088];
        DAT_1030c7e8 = 0;
        if (DAT_104bf8c0 != 0)
            FUN_10010520(TEXT_ASK_LOAD, (int)&DAT_1030c7f0, (int)&DAT_1030c7ec);
        else
            DAT_1030c7f0 = 1;
    }
    if (DAT_1030c7f0 != 0)
    {
        DAT_104b7200.CloseWindow(WINDOW_YESNO_PAGE);
        DAT_104b7200.CloseWindow(WINDOW_YESNO);
        memset(DAT_10c62250, 0, sizeof(DAT_10c62250));
        memset(DAT_10c62650, 0, sizeof(DAT_10c62650));
        DAT_1020e0f8[0] = 0;
        DAT_104b7200.CloseWindow(WINDOW_PAGE_A);
        DAT_104b7200.CloseWindow(WINDOW_SLOTS);
        DAT_104b7200.CloseWindow(WINDOW_PAGE_B);
        DAT_104b7200.CloseWindow(WINDOW_MESSAGE);
        MessageWindow_SetTitle(0);
        DAT_1030c7f0 = 0;
        sprintf(DAT_1032d6a8, SAVE_NAME_FORMAT, DAT_104c02c0 * SLOTS_PER_PAGE + DAT_104bff04);
        FUN_1005cdc0(DAT_1032d6a8, 0);
        DAT_104bfd18 = 1;
        DAT_1021cff8 = 1;
        DAT_1042de20 = DAT_10c62ed0;
        DAT_1030c900 = 0;
        DAT_103086a8 = 0;
        DAT_104bfd50 = 1;
        DAT_104c00d5 = 1;
        DAT_104c00d4 = 5;
        DAT_104c00d0 = 1;
        DAT_104bf8bc = 0;
        DAT_104bf8c0 = 0xf;
        if (DAT_10c62ed0 != 999)
            DAT_104bf8c0 = 1;
    }
    if (DAT_1030c84c != 0)
    {
        DAT_10c63380 = 1;
        DAT_1030c84c = 0;
        FUN_1000cb30("");
        DAT_104bf8d4 = 1;
        if (DAT_10c646e4 != 0)
            FUN_10010520(TEXT_SAVE_CLEAR, (int)&DAT_1030c540, (int)&DAT_1030c54c);
        else
            FUN_10010520(TEXT_SAVE_HERE, (int)&DAT_1030c540, (int)&DAT_1030c54c);
    }
    return 0;
}
