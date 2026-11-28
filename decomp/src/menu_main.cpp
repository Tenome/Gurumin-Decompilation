// The main menu (Menu_Main): the title menu with its difficulty, sound and name entry pages, the debug list of maps,
// the shop commands, the save list (also used by the pause menu's save), the continue list and the start of a loaded
// game. Run every frame by the title screen and the item dictionary.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "vec4.h"
#include "matrix4.h"
#include "node.h"
#include "anim_model.h"
#include "bgm_track.h"
#include "char_slot.h"
#include "game.h"
#include "input_state.h"
#include "item_info.h"
#include "texture_object.h"
#include "tracker.h"
#include "doppler_loops.h"
#include "game_load_data.h"
#include "item_dictionary.h"
#include "save_summary.h"
#include "tutorial_book.h"

#include "camera_vars.h"
#include "char_vars.h"
#include "field_item_menu.h"
#include "game_vars.h"
#include "map_setup.h"
#include "map_vars.h"
#include "menu_main.h"
#include "menu_scene.h"
#include "message_window.h"
#include "render_state.h"
#include "render_vars.h"
#include "script_system.h"
#include "shop_menu.h"
#include "sound_vars.h"
#include "title_screen.h"
#include "ui_vars.h"
#include "win_vars.h"

// Shift-JIS texts. "s064" and the like in the slot formats are the text engine's position tags.
#define TEXT_NAME_ENTRY "\x83l\x81[\x83\x80\x83G\x83\x93\x83g\x83\x8a\x81["
#define TEXT_ENTER_NAME "\x96\xbc\x91O\x82\xf0\x93\xfc\x97\xcd\x82\xb5\x82\xc4\x82\xad\x82\xbe\x82\xb3\x82\xa2\x81" "B"
#define TEXT_DEFAULT_NAME "\x83p\x83\x8a\x83\x93"
#define TEXT_MAIN_MENU "\x83\x81\x83" "C\x83\x93\x83\x81\x83j\x83\x85\x81["
#define TEXT_NEW_GAME "\x90V\x8bK\x82\xc5\x8en\x82\xdf\x82\xe9"
#define TEXT_CONTINUE "\x91\xb1\x82\xab\x82\xa9\x82\xe7\x8en\x82\xdf\x82\xe9"
#define TEXT_SOUND_SETTINGS "\x83T\x83" "E\x83\x93\x83h\x90\xdd\x92\xe8"
#define TEXT_ITEM_DICTIONARY "\x83" "A\x83" "C\x83" "e\x83\x80\x8e\xab\x93T"
#define TEXT_QUIT_GAME "\x83Q\x81[\x83\x80\x8fI\x97\xb9"
#define TEXT_BGM_VOLUME "\x82" "a\x82" "f\x82l\x89\xb9\x97\xca"
#define TEXT_VOICE_PLAYBACK "\x83{\x83" "C\x83X\x8d\xc4\x90\xb6"
#define TEXT_UP "\x81\xa3"
#define TEXT_DOWN "\x81\xa5"
#define TEXT_DIFFICULTY "\x93\xef\x88\xd5\x93x"
#define TEXT_PICK_DIFFICULTY "\x93\xef\x88\xd5\x93x\x82\xf0\x91I\x82\xf1\x82\xc5\x82\xad\x82\xbe\x82\xb3\x82\xa2\x81" "B"
#define TEXT_PICK_SAVE "\x83Z\x81[\x83u\x83" "f\x81[\x83^\x82\xf0\x91I\x82\xf1\x82\xc5\x82\xad\x82\xbe\x82\xb3\x82\xa2"
#define TEXT_ASK_QUIT "\x96{\x93\x96\x82\xc9\x8fI\x97\xb9\x82\xb5\x82\xdc\x82\xb7\x82\xa9\x81H"
#define TEXT_ASK_TITLE "\x96{\x93\x96\x82\xc9\x83^\x83" "C\x83g\x83\x8b\x82\xc9\x96\xdf\x82\xe8\x82\xdc\x82\xb7\x82\xa9\x81H"
#define TEXT_ASK_SAVE "\x96{\x93\x96\x82\xc9\x83Z\x81[\x83u\x82\xb5\x82\xdc\x82\xb7\x82\xa9\x81H"
#define TEXT_ASK_LOAD "\x96{\x93\x96\x82\xc9\x83\x8d\x81|\x83h\x82\xb5\x82\xdc\x82\xb7\x82\xa9\x81H"
#define TEXT_ASK_LEAVE "\x96{\x93\x96\x82\xc9\x8fo\x82\xdc\x82\xb7\x82\xa9\x81H"
#define TEXT_PACK_FULL "\x83" "A\x83" "C\x83" "e\x83\x80\x82\xaa\x82\xa2\x82\xc1\x82\xcf\x82\xa2\x82\xcc\x82\xbd\x82\xdf"
#define TEXT_BOUGHT_ONLY "%d\x8c\xc2\x82\xbe\x82\xaf\x94\x83\x82\xc1\x82\xbd"
#define TEXT_CANNOT_HOLD "\x82\xe0\x82\xa4\x8e\x9d\x82\xc4\x82\xdc\x82\xb9\x82\xf1\x81" "B"
#define TEXT_NO_CASH "\x82" "b\x82\x81\x82\x93\x82\x88\x82\xaa\x91\xab\x82\xe8\x82\xdc\x82\xb9\x82\xf1\x81" "B"
#define TEXT_NO_JUNK_POINTS "\x83K\x83\x89\x83N\x83^\x83|\x83" "C\x83\x93\x83g\x82\xaa\x91\xab\x82\xe8\x82\xdc\x82\xb9\x82\xf1\x81" "B"
#define TEXT_UPGRADE_LIMIT "\x89\xfc\x91\xa2\x82\xcc\x8c\xc0\x8a" "E\x82\xc5\x82\xb7\x81" "B"
#define TEXT_MAP_FORMAT "%03d\x81i%s.it3\x81j"
#define TEXT_COMMAND_MARK "\x87@"
#define TEXT_SLOT_EMPTY "\x82" "d\x82\x8d\x82\x90\x82\x94\x82\x99"
#define TEXT_LOAD "\x83\x8d\x81[\x83h"
#define SLOT_FORMAT "%ss064%02d\x81" "F%02d\x81" "F%02ds180%s"
#define SLOT_FORMAT_LAST "%ss064%02d\x81" "F%02d\x81" "F%02ds180%ss260\x81\x9a"
#define SLOT_FORMAT_CLEAR "\x81|\x82" "b\x82\x8c\x82\x85\x82\x81\x82\x92s060\x82" "c\x82\x81\x82\x94\x82\x81\x81|"
#define SLOT_FORMAT_CLEAR_LAST "\x81|\x82" "b\x82\x8c\x82\x85\x82\x81\x82\x92s060\x82" "c\x82\x81\x82\x94\x82\x81\x81|s260\x81\x9a"
#define NAME_NORMAL "\x82m\x82\x8f\x82\x92\x82\x8d\x82\x81\x82\x8c"
#define NAME_HARD "\x82g\x82\x81\x82\x92\x82\x84"
#define NAME_BEGINNER "\x82" "a\x82\x85\x82\x87\x82\x89\x82\x8e\x82\x8e\x82\x85\x82\x92"
#define NAME_PLEASURE "\x82o\x82\x8c\x82\x85\x82\x81\x82\x93\x82\x95\x82\x92\x82\x85"
#define NAME_CRAZY "\x82" "b\x82\x92\x82\x81\x82\x9a\x82\x99"
#define NAME_CRAZY_BONUS "\x82" "b\x82\x92\x82\x81\x82\x9a\x82\x99\x81i\x82\xa8\x82\xdc\x82\xaf\x81j"
#define SECONDS_PER_HOUR 3600
#define SECONDS_PER_MINUTE 60
#define SOUND_MENU_PICK 0x357
#define SOUND_MENU_SAVE 0x11d
#define SOUND_BOUGHT 0x37d
#define SCRATCH_SIZE 256
#define DEBUG_MAP_FIRST 0xcf
#define DEBUG_MAP_SECOND 0x191
#define MAP_TITLE 999
#define MAP_STAGE 1000
#define COSTUME_CLEAR_DATA 4
#define VOLUME_STEPS 10
#define VOLUME_STEP_ATTENUATION 500
#define WINDOW_SLOT_NONE 3
#define COSTUME_MASK_BONUS (~8)   // every costume bit but 8
#define FADE_FULL 100.0f
#define MESSAGE_FRAMES 0x28

// DAT_104bfd20: what the shop window shows (shop_menu.cpp).
#define SHOP_MODE_LIST 0           // plain list of the texts in DAT_10450808
#define SHOP_MODE_BUY_PREVIEW 1    // the items for sale, with the model of the one under the cursor
#define SHOP_MODE_BUY 2            // the items for sale, without a preview
#define SHOP_MODE_UPGRADE 3        // the helmets that can be upgraded

// Upgrade prices by the level reached so far, and the price shown when nothing can be bought.
#define UPGRADE_PRICE_FIRST 10
#define UPGRADE_PRICE_LEVEL_1 0x32
#define UPGRADE_PRICE_LEVEL_2 0xfa
#define UPGRADE_PRICE_LEVEL_3 999
#define UPGRADE_PRICE_NONE 0x18696
#define UPGRADE_LEVEL_MAX 4
#define UPGRADE_FLAG_OFFSET 3205   // an item's level is in the script flag levelFlag + 3205
#define ITEM_FLAG_LAST 3999

// The shop discounts of the worn helmet's face settings.
#define DISCOUNT_5_PERCENT 0x100000
#define DISCOUNT_10_PERCENT 0x200000
#define DISCOUNT_50_PERCENT 0x300000
#define PERCENT 100

// The slot of the open menu window `id` (WINDOW_SLOT_NONE when it is not open), expanded in place.
static __forceinline int FindWindowSlot(int id)
{
    int slot = 0;
    do
    {
        if (DAT_104b7200.m_0110[slot] != 0 && DAT_104b7200.m_00a4[slot] == id)
            break;
        slot++;
    } while (slot < WINDOW_SLOT_NONE);
    return slot;
}

// Closes menu `id` the way Game::CloseWindow does, expanded in place.
static __forceinline void CloseWindowInline(int id)
{
    int slot = FindWindowSlot(id);
    if (slot != WINDOW_SLOT_NONE)
    {
        DAT_104b7200.m_7660(2);
        DAT_104b7200.m_0110[slot] = 0;
    }
}

// The price of an item for sale: the worn helmet's discount, then the shop's price percentage.
static __forceinline int ShopPrice(int basePrice)
{
    int price = basePrice;
    if ((DAT_104c02c8 & DISCOUNT_50_PERCENT) == DISCOUNT_50_PERCENT)
        price = (int)((float)price * 0.5f);
    else if ((DAT_104c02c8 & DISCOUNT_10_PERCENT) != 0)
        price = (int)((float)price * 0.9f);
    else if ((DAT_104c02c8 & DISCOUNT_5_PERCENT) != 0)
        price = (int)((float)price * 0.95f);
    if (DAT_10c66cfc != 0)
        price = price * DAT_10c66cfc / PERCENT;
    return price;
}

// The text of one line of a save list: the player's name, the play time and the costume's name, marked when the slot is
// the one saved to last. Clear data (costume 4) is shown as a plain label with the same marking.
static __forceinline void FormatSlotLine(char *line, char *costumeName, const char *name, int time, int costume, int slot)
{
    if (costume == 0)
        strcpy(costumeName, NAME_NORMAL);
    else if (costume == 1)
        strcpy(costumeName, NAME_BEGINNER);
    else if (costume == 2)
        strcpy(costumeName, NAME_HARD);
    else if (costume == 3)
        strcpy(costumeName, NAME_PLEASURE);
    else if (costume == 5)
        strcpy(costumeName, NAME_CRAZY);
    if (costume == COSTUME_CLEAR_DATA)
    {
        if (DAT_104c02e8 == 0 || DAT_104c02e8 - 1 != DAT_104c02c0 * SLOTS_PER_PAGE + slot)
            sprintf(line, SLOT_FORMAT_CLEAR, name, time / SECONDS_PER_HOUR, (time % SECONDS_PER_HOUR) / SECONDS_PER_MINUTE,
                    (time % SECONDS_PER_HOUR) % SECONDS_PER_MINUTE, costumeName);
        else
            sprintf(line, SLOT_FORMAT_CLEAR_LAST, name, time / SECONDS_PER_HOUR,
                    (time % SECONDS_PER_HOUR) / SECONDS_PER_MINUTE, (time % SECONDS_PER_HOUR) % SECONDS_PER_MINUTE,
                    costumeName);
    }
    else
    {
        if (DAT_104c02e8 == 0 || DAT_104c02e8 - 1 != DAT_104c02c0 * SLOTS_PER_PAGE + slot)
            sprintf(line, SLOT_FORMAT, name, time / SECONDS_PER_HOUR, (time % SECONDS_PER_HOUR) / SECONDS_PER_MINUTE,
                    (time % SECONDS_PER_HOUR) % SECONDS_PER_MINUTE, costumeName);
        else
            sprintf(line, SLOT_FORMAT_LAST, name, time / SECONDS_PER_HOUR, (time % SECONDS_PER_HOUR) / SECONDS_PER_MINUTE,
                    (time % SECONDS_PER_HOUR) % SECONDS_PER_MINUTE, costumeName);
    }
}

// 0x10112140 (Menu_Main). Runs the main menu. Each block handles a flag the menu entries set: the first call opens the
// name entry; later calls build the title menu (new game, continue, sound settings, the item dictionary once it is
// unlocked, quit), the sound pages (sound output, voice, BGM volume), the difficulty list, the debug list of maps, the
// shop commands (by DAT_104bfd20: the price with the helmet's discount, the coins, the junk points and the upgrade
// levels), the save list with its overwrite question, the continue list and the load of the picked save (which starts
// the game on the map of the save). Returns 1 when the menu was closed or the game window was told to close, else 0.
extern "C" int FUN_10112140(void)
{
    char path[200];
    char scratch[SCRATCH_SIZE];

    if (DAT_1030c500[0] != 0)
    {
        DAT_104bf8ec = 0;
        DAT_1032d4b0 = 0;
        DAT_104bf8f0 = 0;
        DAT_104bf8e8 = 0;
        DAT_104bf8e4 = 0;
        DAT_1030c500[0] = 0;
        DAT_104b7200.CloseWindow(WINDOW_MAIN_MENU);
        DAT_104c012c = 3;
        Bgm_Play(3, 0);
        DAT_104bf8c0 = 10;
        DAT_104bf8bc = 0;
        DAT_1030c504 = 0;
        DAT_104b7200.CloseWindow(WINDOW_SLOTS);
        DAT_104b7200.CloseWindow(WINDOW_LIST_PANEL);
        FUN_1000cb30(TEXT_NAME_ENTRY);
        FUN_1000cb90(TEXT_ENTER_NAME);
        strcpy((char *)DAT_10255a08, TEXT_DEFAULT_NAME);
        DAT_10f68110 = 1;
        FUN_1015f310((char *)DAT_10255a08);
        DAT_10f66dd4 = 1;
        FUN_1015d500(8);
        DAT_104bfd14 = 1;
    }
    if (DAT_1030c504 != 0)
    {
        DAT_1030c504 = 0;
        DAT_104b7200.CloseWindow(WINDOW_MAIN_MENU);
        PostMessageA(DAT_10f6569c, WM_CLOSE, 0, 0);
        return 1;
    }
    if (DAT_1030c508 != 0)
    {
        DAT_1030c508 = 0;
        FUN_100105e0(TEXT_ASK_QUIT, (int)&DAT_1030c504, (int)&DAT_1030c518);
    }
    if (DAT_1030c50c != 0)
    {
        DAT_104bf8ec = 0;
        DAT_1032d4b0 = 0;
        DAT_104bf8f0 = 0;
        DAT_104bf8e8 = 0;
        DAT_104bf8e4 = 0;
        FUN_1000cb30(TEXT_MAIN_MENU);
        FUN_1000cb90("");
        memset(DAT_10c62250, 0, sizeof(DAT_10c62250));
        memset(DAT_10c62650, 0, sizeof(DAT_10c62650));
        memset(DAT_1030c500, 0, sizeof(DAT_1030c500));
        DAT_104bff00 = 0;
        DAT_1030c50c = 0;
        DAT_104b7200.CloseWindow(WINDOW_SLOTS);
        DAT_104b7200.CloseWindow(WINDOW_MAP_LIST);
        DAT_104b7200.CloseWindow(WINDOW_LIST_PANEL);
        int bonusHeight = 0;
        if (((unsigned short)DAT_104c0230 & COSTUME_MASK_BONUS) != 0)
            bonusHeight = 0x18;
        DAT_104b7200.OpenWindow(WINDOW_MAIN_MENU, 0xe0, 0x78, 0xc0, bonusHeight + 0x88);
        DAT_104b7200.AddFlagEntry(WINDOW_MAIN_MENU, TEXT_NEW_GAME, &DAT_1030c500[0]);
        DAT_104b7200.AddFlagEntry(WINDOW_MAIN_MENU, TEXT_CONTINUE, &DAT_1030c7e0);
        DAT_104b7200.AddFlagEntry(WINDOW_MAIN_MENU, TEXT_SOUND_SETTINGS, &DAT_1030c7f8);
        if (((unsigned short)DAT_104c0230 & COSTUME_MASK_BONUS) != 0)
            DAT_104b7200.AddFlagEntry(WINDOW_MAIN_MENU, TEXT_ITEM_DICTIONARY, &DAT_1030c6bc);
        DAT_104b7200.AddFlagEntry(WINDOW_MAIN_MENU, TEXT_QUIT_GAME, &DAT_1030c504);
        DAT_104b7200.SetWindowResult(WINDOW_MAIN_MENU, &DAT_1030c58c);
    }
    if (DAT_1030c7f8 != 0)
    {
        DAT_104b7200.CloseWindow(WINDOW_MAIN_MENU);
        FUN_1003ad20(SOUND_MENU_PICK, 0, 1.0f);
        FUN_1000cb30(TEXT_SOUND_SETTINGS);
        DAT_1030c804 = DAT_104c01b8;
        DAT_1030c808 = DAT_104c0332 == 0;
        DAT_1030c800 = 1;
        DAT_104b7200.OpenWindow(WINDOW_LIST_PANEL, 0x20, 0x50, 0x114, 0x58);
        DAT_104b7200.SetWindowResult(WINDOW_LIST_PANEL, &DAT_1030c818);
        int slot = FindWindowSlot(WINDOW_LIST_PANEL);
        if (slot != WINDOW_SLOT_NONE)
            DAT_104b7200.m_0128[slot] = 0x18;
        DAT_104b7200.AddRangeEntry(WINDOW_LIST_PANEL, TEXT_BGM_VOLUME, DAT_102027d8, 0, &DAT_1030c800);
        DAT_104b7200.AddValueEntry(WINDOW_LIST_PANEL, TEXT_VOICE_PLAYBACK, (int)&DAT_1030c808);
        DAT_1030c7f8 = 0;
    }
    if (DAT_1030c800 != 0)
    {
        DAT_104b7200.CloseWindow(WINDOW_MAIN_MENU);
        FUN_1000cb30(TEXT_SOUND_SETTINGS);
        FUN_1000cb90(TEXT_BGM_VOLUME);
        DAT_104b7200.OpenWindow(WINDOW_VOLUME, 0xd4, 0x3e, 0x54, 0x68);
        DAT_104b7200.SetWindowResult(WINDOW_VOLUME, &DAT_1030c80c);
        int slot = FindWindowSlot(WINDOW_VOLUME);
        if (slot != WINDOW_SLOT_NONE)
            DAT_104b7200.m_0128[slot] = 0x28;
        DAT_104b7200.AddFlagEntry(WINDOW_VOLUME, TEXT_UP, &DAT_1030c810);
        DAT_104b7200.AddFlagEntry(WINDOW_VOLUME, TEXT_DOWN, &DAT_1030c814);
        DAT_104b7200.SetWindowLayout2(WINDOW_VOLUME, 0x22, 0x26, &DAT_102027d8);
        DAT_1030c800 = 0;
    }
    if (DAT_1030c80c != 0)
    {
        DAT_104b7200.CloseWindow(WINDOW_MAIN_MENU);
        DAT_1030c80c = 0;
        DAT_104b7200.CloseWindow(WINDOW_VOLUME);
        DAT_1030c7f8 = 1;
        FUN_1000cb30(TEXT_SOUND_SETTINGS);
    }
    if (DAT_1030c810 != 0)
    {
        if (DAT_102027d8 < VOLUME_STEPS)
            DAT_102027d8 = DAT_102027d8 + 1;
        FUN_10001b70((VOLUME_STEPS - DAT_102027d8) * VOLUME_STEP_ATTENUATION);
        DAT_1030c810 = 0;
    }
    if (DAT_1030c814 != 0)
    {
        if (DAT_102027d8 > 1)
            DAT_102027d8 = DAT_102027d8 - 1;
        FUN_10001b70((VOLUME_STEPS - DAT_102027d8) * VOLUME_STEP_ATTENUATION);
        DAT_1030c814 = 0;
    }
    if (DAT_1030c818 != 0)
    {
        DAT_104c01b8 = DAT_1030c804;
        DAT_104c0332 = DAT_1030c808 == 0;
        DAT_1030c818 = 0;
        DAT_1030c50c = 1;
    }
    if (DAT_1030c58c != 0)
    {
        DAT_1030c58c = 0;
        DAT_104b7200.CloseWindow(WINDOW_MAIN_MENU);
        DAT_104bf890 = 0;
        DAT_104c0324 = 1;
        DAT_104bfd14 = 0;
        DAT_104c012c = 0;
        Bgm_Play(0, 0);
        DAT_104bf8f0 = FADE_FULL;
        DAT_104bf8e8 = 0;
        DAT_104bf8ec = 0;
        DAT_1032d4b0 = -10.0f / 3.0f;
        DAT_104bf8e4 = 0xf;
        MessageWindow_SetTitle(0);
        FUN_1000cb90("");
    }
    if (DAT_1030c510 != 0)
    {
        DAT_1030c510 = 0;
        FUN_100105e0(TEXT_ASK_TITLE, (int)&DAT_1030c514, (int)&DAT_1030c518);
    }
    if (DAT_1030c514 != 0)
    {
        DAT_1030c514 = 0;
        DAT_104bfd84 = 1;
        return 1;
    }
    if (DAT_1030c518 != 0)
    {
        DAT_1030c518 = 0;
        DAT_104b7200.CloseWindow(WINDOW_YESNO_PAGE);
        DAT_104b7200.CloseWindow(WINDOW_YESNO);
        DAT_1020e0f8[0] = 0;
    }
    if (DAT_1030c51c != 0)
    {
        int entryIndex = DAT_104b7200.m_0140[DAT_104b7200.m_0088];
        DAT_104b7200.CloseWindow(WINDOW_MAP_LIST);
        DAT_104b7200.CloseWindow(WINDOW_MAIN_MENU);
        DAT_104bfd18 = 1;
        FUN_100154c0(DAT_10daabb4);
        GameNamedEntry *entry = &DAT_10874848.m_95e14[entryIndex];
        DAT_10db6ee8[0].SetPosition((float)entry->m_0094[0], (float)entry->m_0094[1], (float)entry->m_0094[2]);
        DAT_1021cff8 = 2;
        DAT_1042de20 = entryIndex;
        DAT_10db6ee8[0].Teleport((float)entry->m_0094[0], (float)entry->m_0094[1], (float)entry->m_0094[2]);
        DAT_104bfd50 = 1;
        DAT_104bf8c0 = 1;
        DAT_104bf8bc = 0;
        DAT_1030c51c = 0;
        FUN_1000e750();
        FUN_1000e810();
    }
    if (DAT_1030c520 != 0)
    {
        DAT_104b7200.CloseWindow(WINDOW_MAP_LIST);
        DAT_104b7200.CloseWindow(WINDOW_MAIN_MENU);
        DAT_104bfd18 = 1;
        MessageWindow_SetTitle(0);
        FUN_1000cb90("");
        FUN_100154c0(DAT_10daabb4);
        DAT_10db6ee8[0].SetPosition(0, 0, 0);
        DAT_10c62ed0 = DEBUG_MAP_FIRST;
        DAT_1021cff8 = 1;
        DAT_1042de20 = DEBUG_MAP_FIRST;
        DAT_1030c900 = 0;
        DAT_103086a8 = 0;
        DAT_10db6ee8[0].SetPosition(0, 0, 0);
        DAT_104bfd50 = 1;
        DAT_104bf8c0 = 1;
        DAT_104bf8bc = 0;
        DAT_1030c520 = 0;
        FUN_1000e750();
        FUN_1000e810();
    }
    if (DAT_1030c524 != 0)
    {
        DAT_1030c524 = 0;
        DAT_104b7200.CloseWindow(WINDOW_MAIN_MENU);
        DAT_104bf8c0 = 10;
        DAT_1030c504 = 0;
        DAT_104b7200.CloseWindow(WINDOW_SLOTS);
        DAT_104b7200.CloseWindow(WINDOW_LIST_PANEL);
        FUN_1000cb30(TEXT_NAME_ENTRY);
        FUN_1000cb90(TEXT_ENTER_NAME);
        DAT_10f68110 = 1;
        FUN_1015f310((char *)DAT_10255a08);
        DAT_10f66dd4 = 1;
        FUN_1015d500(8);
        DAT_104bfd14 = 1;
    }
    if (DAT_1030c5a0 != 0)
    {
        FUN_1003ad20(SOUND_MENU_PICK, 0, 1.0f);
        DAT_104b7200.CloseWindow(WINDOW_LIST_PANEL);
        DAT_104bfd14 = 0xb;
        DAT_1030c5a0 = 0;
        DAT_1030c538 = 1;
        DAT_10c635f8 = 2;
        DAT_10c66cc0 = 1;
        FUN_1000e750();
    }
    if (DAT_1030c5a4 != 0)
    {
        FUN_1003ad20(SOUND_MENU_PICK, 0, 1.0f);
        DAT_104b7200.CloseWindow(WINDOW_LIST_PANEL);
        DAT_104bfd14 = 0xb;
        DAT_1030c5a4 = 0;
        DAT_1030c538 = 1;
        DAT_10c635f8 = 2;
        DAT_10c66cc0 = 2;
        FUN_1000e750();
    }
    if (DAT_1030c528 != 0)
    {
        FUN_1003ad20(SOUND_MENU_PICK, 0, 1.0f);
        DAT_104b7200.CloseWindow(WINDOW_LIST_PANEL);
        DAT_104bfd14 = 0xb;
        DAT_1030c528 = 0;
        DAT_1030c538 = 1;
        DAT_10c635f8 = 2;
        FUN_1000e750();
    }
    if (DAT_1030c52c != 0)
    {
        FUN_1003ad20(SOUND_MENU_PICK, 0, 1.0f);
        DAT_104b7200.CloseWindow(WINDOW_LIST_PANEL);
        DAT_104bfd14 = 0xb;
        DAT_1030c52c = 0;
        DAT_1030c538 = 1;
        DAT_10c635f8 = 0;
        FUN_1000e750();
    }
    if (DAT_1030c530 != 0)
    {
        FUN_1003ad20(SOUND_MENU_PICK, 0, 1.0f);
        DAT_104b7200.CloseWindow(WINDOW_LIST_PANEL);
        DAT_104bfd14 = 0xb;
        DAT_1030c530 = 0;
        DAT_1030c538 = 1;
        DAT_10c635f8 = 1;
        FUN_1000e750();
    }
    if (DAT_1030c534 != 0)
    {
        DAT_1030c534 = 0;
        DAT_1030c500[0] = 1;
        DAT_104b7200.CloseWindow(WINDOW_PAGE_B);
    }
    if (DAT_1030c538 != 0)
    {
        DAT_1030c538 = 0;
        DAT_1030c53c = 0;
        DAT_104b7200.CloseWindow(WINDOW_PAGE_B);
        DAT_104bfd14 = 0xb;
        MessageWindow_SetTitle(0);
        FUN_1000cb90("");
    }
    if (DAT_1030c53c != 0)
    {
        FUN_1000cb30(TEXT_DIFFICULTY);
        FUN_1000cb90(TEXT_PICK_DIFFICULTY);
        if ((DAT_104c0230 & 0x10) != 0)
        {
            DAT_104b7200.OpenWindow(WINDOW_LIST_PANEL, 0x20, 0x50, 0xb4, 0xa0);
            DAT_104b7200.SetWindowResult(WINDOW_LIST_PANEL, &DAT_1030c524);
            DAT_104b7200.SetWindowScroll(WINDOW_LIST_PANEL, 0x18);
            DAT_104b7200.AddFlagEntry(WINDOW_LIST_PANEL, NAME_NORMAL, &DAT_1030c52c);
            DAT_104b7200.AddFlagEntry(WINDOW_LIST_PANEL, NAME_BEGINNER, &DAT_1030c530);
            DAT_104b7200.AddFlagEntry(WINDOW_LIST_PANEL, NAME_HARD, &DAT_1030c528);
            DAT_104b7200.AddFlagEntry(WINDOW_LIST_PANEL, NAME_PLEASURE, &DAT_1030c5a0);
            DAT_104b7200.AddFlagEntry(WINDOW_LIST_PANEL, NAME_CRAZY_BONUS, &DAT_1030c5a4);
        }
        else
        {
            unsigned short costumes = (unsigned short)DAT_104c0230 & 7;
            if (costumes == 6 || costumes == 7)
            {
                DAT_104b7200.OpenWindow(WINDOW_LIST_PANEL, 0x20, 0x50, 0xb4, 0x88);
                DAT_104b7200.SetWindowResult(WINDOW_LIST_PANEL, &DAT_1030c524);
                DAT_104b7200.SetWindowScroll(WINDOW_LIST_PANEL, 0x18);
                DAT_104b7200.AddFlagEntry(WINDOW_LIST_PANEL, NAME_NORMAL, &DAT_1030c52c);
                DAT_104b7200.AddFlagEntry(WINDOW_LIST_PANEL, NAME_BEGINNER, &DAT_1030c530);
                DAT_104b7200.AddFlagEntry(WINDOW_LIST_PANEL, NAME_HARD, &DAT_1030c528);
                DAT_104b7200.AddFlagEntry(WINDOW_LIST_PANEL, NAME_PLEASURE, &DAT_1030c5a0);
            }
            else if (((unsigned short)DAT_104c0230 & COSTUME_MASK_BONUS) != 0)
            {
                DAT_104b7200.OpenWindow(WINDOW_LIST_PANEL, 0x20, 0x50, 0xb4, 0x70);
                DAT_104b7200.SetWindowResult(WINDOW_LIST_PANEL, &DAT_1030c524);
                DAT_104b7200.SetWindowScroll(WINDOW_LIST_PANEL, 0x18);
                DAT_104b7200.AddFlagEntry(WINDOW_LIST_PANEL, NAME_NORMAL, &DAT_1030c52c);
                DAT_104b7200.AddFlagEntry(WINDOW_LIST_PANEL, NAME_BEGINNER, &DAT_1030c530);
                DAT_104b7200.AddFlagEntry(WINDOW_LIST_PANEL, NAME_HARD, &DAT_1030c528);
            }
            else
            {
                DAT_104b7200.OpenWindow(WINDOW_LIST_PANEL, 0x20, 0x50, 0xb4, 0x58);
                DAT_104b7200.SetWindowResult(WINDOW_LIST_PANEL, &DAT_1030c524);
                DAT_104b7200.SetWindowScroll(WINDOW_LIST_PANEL, 0x18);
                DAT_104b7200.AddFlagEntry(WINDOW_LIST_PANEL, NAME_NORMAL, &DAT_1030c52c);
                DAT_104b7200.AddFlagEntry(WINDOW_LIST_PANEL, NAME_BEGINNER, &DAT_1030c530);
            }
        }
        DAT_1030c53c = 0;
    }
    if (DAT_1030c540 != 0)
    {
        DAT_10c66d18 = 0;
        DAT_104b7200.CloseWindow(WINDOW_MESSAGE);
        DAT_104b7200.CloseWindow(WINDOW_YESNO_PAGE);
        DAT_104b7200.CloseWindow(WINDOW_YESNO);
        DAT_1020e0f8[0] = 0;
        DAT_104bfd78 = 1;
        DAT_104bfd7c = 1;
        FUN_1013f070(RENDER_DEPTH_TEST, 1, 0);
        DAT_118d0ba8[DAT_108360f8].UpdateImage(0);
        FUN_1013f070(RENDER_FLUSH_SCENE, 0, 0);
        DAT_10da90d0.Set(0.5f, 0.5f, 0.5f, 1.0f);
        DAT_1042da68 = 0;
        DAT_104c0220 = 1;
        if (DAT_10f65484 != 0)
        {
            DAT_10f65484 = 0;
            if (DAT_104bf8c0 == 1)
            {
                Scene_DrawFrame(0);
                FUN_1013f070(RENDER_FLUSH_SCENE, 0, 0);
                DAT_10f65484 = 1;
            }
            else
            {
                Menu_DrawScene();
                FUN_1013f070(RENDER_FLUSH_SCENE, 0, 0);
                DAT_10f65484 = 1;
            }
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
        DAT_10da90d0.Set(1.0f, 1.0f, 1.0f, 1.0f);
        DAT_1042da68 = 0;
        FUN_1014b480();
        FUN_1013f070(RENDER_DEPTH_TEST, 1, 0);
        DAT_104bfd7c = 1;
        DAT_104bfd78 = 1;
        DAT_104b7200.CloseWindow(WINDOW_YESNO_PAGE);
        DAT_104b7200.CloseWindow(WINDOW_YESNO);
        DAT_1020e0f8[0] = 0;
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
            int map;
            int playTime;
            char playerName[40];
            int costume;
            sprintf(path, SAVE_NAME_FORMAT, DAT_104c02c0 * SLOTS_PER_PAGE + i);
            FUN_10010d10(path, &map, &playTime, playerName, &costume);
            if (playerName[0] == 0)
                DAT_104b7200.AddFlagEntry(WINDOW_SLOTS, TEXT_SLOT_EMPTY, &DAT_1030c544);
            else
            {
                FormatSlotLine(path, scratch, playerName, playTime, costume, i);
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
        FUN_1003ad20(SOUND_MENU_SAVE, 0, 1.0f);
        int slot = DAT_104bff04;
        DAT_1030c548 = 0;
        DAT_104b7200.CloseWindow(WINDOW_YESNO_PAGE);
        DAT_104b7200.CloseWindow(WINDOW_YESNO);
        DAT_1020e0f8[0] = 0;
        DAT_1030c540 = 1;
        sprintf(scratch, SAVE_NAME_FORMAT, DAT_104c02c0 * SLOTS_PER_PAGE + slot);
        FUN_1003b260(scratch);
        DAT_104c02e8 = DAT_104c02c0 * SLOTS_PER_PAGE + 1 + slot;
        FUN_1015fe20(DAT_10f65370, "CONFIG", "LSF", DAT_104c02e8);
        return 1;
    }
    if (DAT_1030c54c != 0)
    {
        DAT_1030c54c = 0;
        DAT_104bfd78 = 0;
        DAT_104b7200.CloseWindow(WINDOW_SLOTS);
        DAT_104b7200.CloseWindow(WINDOW_YESNO_PAGE);
        DAT_104b7200.CloseWindow(WINDOW_YESNO);
        DAT_1020e0f8[0] = 0;
        FUN_1000cb30("");
        FUN_1000cb90("");
        DAT_104bf8d4 = 1;
        DAT_10c66d18 = 1;
        return 1;
    }
    if (DAT_1030c550 != 0)
    {
        DAT_104bfd84 = 1;
        DAT_1030c550 = 0;
        DAT_104b7200.CloseWindow(WINDOW_YESNO_PAGE);
        DAT_104b7200.CloseWindow(WINDOW_YESNO);
        DAT_1020e0f8[0] = 0;
        return 1;
    }
    if (DAT_1030c554 != 0)
    {
        MessageWindow_SetTitle(0);
        DAT_1030c554 = 0;
        FUN_100106a0();
    }
    if (DAT_1030c558 != 0)
    {
        FUN_100106a0();
        MessageWindow_SetTitle(0);
        FUN_1004ed60(0);
        DAT_1030c558 = 0;
    }
    if (DAT_1030c55c != 0)
    {
        FUN_100106a0();
        MessageWindow_SetTitle(0);
        FUN_1004ed60(1);
        DAT_1030c55c = 0;
    }
    if (DAT_1030c564 != 0)
    {
        DAT_104bff04 = DAT_104b7200.m_0140[DAT_104b7200.m_0088];
        DAT_1030c564 = 0;
        FUN_100105e0(TEXT_ASK_LEAVE, (int)&DAT_1030c560, (int)&DAT_1030c518);
    }
    if (DAT_1030c560 != 0)
    {
        DAT_10f654b4 = 0;
        DAT_10f654b8 = 0;
        DAT_104bfe1c = 0;
        DAT_104bfe18 = 0;
        DAT_104bf8c0 = 0xf;
        DAT_104bf8bc = 0;
        DAT_104bfd50 = 1;
        DAT_1020d9e0.M_10001550();
        DAT_1065cb28.M_1013edb0();
        DAT_104bf8ec = 0;
        DAT_1032d4b0 = 0;
        DAT_104bf8f0 = FADE_FULL;
        DAT_104bf8e8 = FADE_FULL;
        DAT_104bf8e4 = 0;
        DAT_1101de30 = 1;
        DAT_11018734 = 1;
        DAT_10c6356c = 0;
        Script_ValueCommand(0x271b, 10);
        Script_ValueCommand(0x2774, 0);
        Script_ValueCommand(99999, 0);
        DAT_10c63518 = 0;
        DAT_1030c560 = 0;
        FUN_100106a0();
    }
    if (DAT_1030c568 != 0)
    {
        int offset = 0;
        DAT_1030c568 = 0;
        if (DAT_104bfd20 == SHOP_MODE_BUY_PREVIEW)
            offset = 0x18;
        else if (DAT_104bfd20 == SHOP_MODE_BUY)
            offset = 0x80;
        if (DAT_104bfd20 == SHOP_MODE_UPGRADE)
        {
            DAT_104b7200.OpenWindow(WINDOW_COMMANDS, 0x94, 0x62, 0x100, 0x9c);
            DAT_104b7200.ConfirmWindow(WINDOW_COMMANDS);
            DAT_104b7200.SetWindowScroll(WINDOW_COMMANDS, 0x16);
            DAT_104b7200.SetWindowLastItem(WINDOW_COMMANDS, 10);
        }
        else if (DAT_104bfd20 != SHOP_MODE_LIST)
        {
            DAT_104b7200.OpenWindow(WINDOW_COMMANDS, offset + 0x44, 0x72, 0x100, 0x9c);
            DAT_104b7200.ConfirmWindow(WINDOW_COMMANDS);
            DAT_104b7200.SetWindowScroll(WINDOW_COMMANDS, 0x16);
            if (DAT_104bfd20 == SHOP_MODE_BUY_PREVIEW)
                DAT_104b7200.SetWindowLastItem(WINDOW_COMMANDS, 8);
            else if (DAT_104bfd20 == SHOP_MODE_BUY)
                DAT_104b7200.SetWindowLastItem(WINDOW_COMMANDS, 10);
        }
        else
        {
            DAT_104b7200.OpenWindow(WINDOW_COMMANDS, 0xc4, 0x7a, 0x100, 0x9c);
            DAT_104b7200.ConfirmWindow(WINDOW_COMMANDS);
            DAT_104b7200.SetWindowScroll(WINDOW_COMMANDS, 0x16);
            DAT_104b7200.SetWindowLastItem(WINDOW_COMMANDS, 5);
        }
        DAT_104b7200.SetWindowCallbackValue(WINDOW_COMMANDS, FUN_100106d0);
        DAT_104b7200.SetWindowResult(WINDOW_COMMANDS, &DAT_1030c570);
        for (int i = 0; i < DAT_104bfd24; i++)
            DAT_104b7200.AddRangeEntry(WINDOW_COMMANDS, TEXT_COMMAND_MARK, 0, 0, &DAT_1030c56c);
    }
    if (DAT_1030c56c != 0)
    {
        if (DAT_104bfd20 == SHOP_MODE_LIST)
        {
            if (DAT_1042e2a8[DAT_104bfd30] != -1 && DAT_10202350 <= ITEM_FLAG_LAST && DAT_10202350 != 0)
                DAT_10c62edc[DAT_10202350] = DAT_1042e2a8[DAT_104bfd30];
            DAT_104bfd28 = 3;
            DAT_104bfd10 = 0;
            DAT_104b7200.CloseWindow(WINDOW_COMMANDS);
        }
        else if (DAT_104bfd20 == SHOP_MODE_UPGRADE)
        {
            int item = DAT_1042e2a8[DAT_104bfd30];
            int level = DAT_10874848.TestScriptFlag(DAT_1088e6b0[item].levelFlag + UPGRADE_FLAG_OFFSET);
            int price = UPGRADE_PRICE_NONE;
            if (level < 1)
                price = UPGRADE_PRICE_FIRST;
            if (level == 1)
                price = UPGRADE_PRICE_LEVEL_1;
            else if (level == 2)
                price = UPGRADE_PRICE_LEVEL_2;
            else if (level == 3)
                price = UPGRADE_PRICE_LEVEL_3;
            else if (level >= UPGRADE_LEVEL_MAX)
            {
                DAT_10202350 = 0;
                FUN_1000ed70(TEXT_UPGRADE_LIMIT, 0, MESSAGE_FRAMES);
                goto purchaseDone;
            }
            if (price <= DAT_10c6670c)
            {
                DAT_104c02d0 = item;
                DAT_104bfd5c = 0;
                DAT_10202350 = item;
                DAT_104bfd28 = 3;
                DAT_104bfd10 = 0;
                DAT_104b7200.CloseWindow(WINDOW_COMMANDS);
            }
            else
            {
                DAT_10202350 = 0;
                FUN_1000ed70(TEXT_NO_JUNK_POINTS, 0, MESSAGE_FRAMES);
            }
        }
        else if (DAT_104bfd20 == SHOP_MODE_BUY)
        {
            int item = DAT_1042e2a8[DAT_104bfd30];
            int price = ShopPrice(DAT_1088e6b0[item].price);
            if (DAT_10254590 * price <= DAT_10daaae8[0].coins)
            {
                DAT_10202350 = item;
                DAT_104bfd5c = 0;
                DAT_104bfd28 = 3;
                DAT_104bfd10 = 0;
                DAT_104b7200.CloseWindow(WINDOW_COMMANDS);
            }
            else
            {
                DAT_10202350 = 0;
                FUN_1000ed70(TEXT_NO_CASH, 0, MESSAGE_FRAMES);
            }
        }
        else
        {
            int item = DAT_1042e2a8[DAT_104bfd30];
            int price = ShopPrice(DAT_1088e6b0[item].price);
            int wanted = DAT_10254590;
            int total = wanted * price;
            if (total <= DAT_10daaae8[0].coins)
            {
                int remaining = wanted;
                if (FUN_10012130(&item, &remaining) != 0)
                {
                    DAT_10daaae8[0].coins -= total;
                    FUN_1003ad20(SOUND_BOUGHT, 0, 1.0f);
                    DAT_10202350 = DAT_1042e2a8[DAT_104bfd30];
                    DAT_104bfd28 = 3;
                    DAT_104bfd10 = 0;
                    DAT_104bfd5c = 0;
                    DAT_104b7200.CloseWindow(WINDOW_COMMANDS);
                }
                else if (wanted == remaining)
                    FUN_1000ed70(TEXT_CANNOT_HOLD, 0, MESSAGE_FRAMES);
                else
                {
                    FUN_1003ad20(SOUND_BOUGHT, 0, 1.0f);
                    sprintf(scratch, TEXT_BOUGHT_ONLY, DAT_10254590 - remaining);
                    FUN_1000ed70(TEXT_PACK_FULL, scratch, MESSAGE_FRAMES);
                    DAT_10daaae8[0].coins -= (DAT_10254590 - remaining) * price;
                }
            }
            else
                FUN_1000ed70(TEXT_NO_CASH, 0, MESSAGE_FRAMES);
            FUN_1000e750();
        }
    purchaseDone:
        DAT_104bfd5c = 0;
        DAT_1030c56c = 0;
    }
    if (DAT_1030c570 != 0)
    {
        DAT_1030c570 = 0;
        if (DAT_104bfd20 == SHOP_MODE_LIST)
        {
            if (DAT_10202350 != -1)
            {
                if (DAT_10202350 <= ITEM_FLAG_LAST && DAT_10202350 != 0)
                    DAT_10c62edc[DAT_10202350] = -1;
                DAT_104bfd28 = 3;
                DAT_104bfd10 = 0;
            }
        }
        else
        {
            DAT_10202350 = 0;
            DAT_104bfd28 = 3;
            DAT_104bfd10 = 0;
        }
        DAT_104bfd5c = 0;
        DAT_104b7200.CloseWindow(WINDOW_COMMANDS);
    }
    if (DAT_1030c5a8 != 0)
    {
        MessageWindow_SetTitle(0);
        FUN_1000cb90("");
        DAT_1030c5a8 = 0;
        DAT_1030c5ac = 0;
        DAT_104b7200.OpenWindow(WINDOW_MAP_LIST, 0x20, 0x20, 0x10c, 0x19c);
        DAT_104b7200.SetWindowScroll(WINDOW_MAP_LIST, 0x18);
        DAT_104b7200.SetWindowLastItem(WINDOW_MAP_LIST, 0xf);
        DAT_104b7200.SetWindowResult(WINDOW_MAP_LIST, &DAT_1030c50c);
        for (int i = 0; i < DAT_10874848.m_a4614; i++)
        {
            sprintf(scratch, TEXT_MAP_FORMAT, DAT_10874848.m_95e14[i].map, DAT_10874848.m_95e14[i].name,
                    DAT_10874848.m_95e14[i].file);
            FUN_1015d520(scratch);
            DAT_104b7200.AddFlagEntry(WINDOW_MAP_LIST, scratch, &DAT_1030c51c);
        }
    }
    if (DAT_1030c660 != 0)
    {
        DAT_104bf8c0 = 0;
        DAT_104bf8bc = 0;
        DAT_1030c660 = 0;
        DAT_1030c50c = 1;
    }
    if (DAT_1030c6bc != 0)
    {
        DAT_104bf8c0 = 5;
        DAT_104bf8bc = 0;
        DAT_1030c6bc = 0;
        DAT_104b7200.CloseWindow(WINDOW_MAIN_MENU);
    }
    if (DAT_1030c7e0 != 0 || DAT_1030c7e4 != 0)
    {
        if (DAT_1030c7e0 != 0)
        {
            FUN_1003ad20(SOUND_MENU_PICK, 0, 1.0f);
            FUN_1000cb30(TEXT_CONTINUE);
            FUN_1000cb90(TEXT_PICK_SAVE);
            DAT_1030c7e0 = 0;
        }
        DAT_104bff00 = 1;
        DAT_1030c7e4 = 0;
        DAT_104b7200.CloseWindow(WINDOW_LOAD_FRAME);
        DAT_104b7200.CloseWindow(WINDOW_MAIN_MENU);
        DAT_104b7200.OpenWindow(WINDOW_SLOTS, 0x1c, 0x74, 0x186, 0xcc);
        DAT_104b7200.SetWindowResult(WINDOW_SLOTS, &DAT_1030c50c);
        DAT_104b7200.m_01dc[WINDOW_SLOTS] = 1;
        DAT_104b7200.SetWindowCallbackValue(WINDOW_SLOTS, FUN_1003b180);
        if (DAT_104c02e8 != 0 && DAT_104c02ec == 0)
        {
            DAT_104c02c0 = (DAT_104c02e8 - 1) / SLOTS_PER_PAGE;
            DAT_104b7200.SetWindowCursor(WINDOW_SLOTS, (DAT_104c02e8 - 1) % SLOTS_PER_PAGE);
            DAT_104c02ec = 1;
        }
        DAT_104b7200.SetWindowLayout(WINDOW_SLOTS, 0, 0x13, &DAT_104c02c0, &DAT_1030c7e4);
        DAT_104bff04 = -1;
        for (int i = 0; i < SLOTS_PER_PAGE; i++)
        {
            int map;
            int playTime;
            char playerName[40];
            int costume;
            sprintf(path, SAVE_NAME_FORMAT, DAT_104c02c0 * SLOTS_PER_PAGE + i);
            FUN_10010d10(path, &map, &playTime, playerName, &costume);
            const char *text;
            int *picked;
            if (playerName[0] == 0)
            {
                text = TEXT_SLOT_EMPTY;
                picked = &DAT_1030c770;
            }
            else
            {
                FormatSlotLine(path, scratch, playerName, playTime, costume, i);
                text = path;
                picked = &DAT_1030c7e8;
            }
            DAT_104b7200.AddFlagEntry(WINDOW_SLOTS, text, picked);
        }
    }
    if (DAT_1030c7ec != 0)
    {
        DAT_104b7200.CloseWindow(WINDOW_YESNO_PAGE);
        DAT_104b7200.CloseWindow(WINDOW_YESNO);
        DAT_1020e0f8[0] = 0;
        DAT_104bff00 = 1;
        FUN_1000cb30(TEXT_LOAD);
        DAT_1030c7ec = 0;
        DAT_104b7200.CloseWindow(WINDOW_LOAD_FRAME);
        DAT_104b7200.OpenWindow(WINDOW_SLOTS, 0x7c, 0xf4, 0x186, 0xcc);
        DAT_104b7200.SetWindowResult(WINDOW_SLOTS, &DAT_1030c520);
        DAT_104b7200.m_01dc[WINDOW_SLOTS] = 1;
        int slot = FindWindowSlot(WINDOW_SLOTS);
        if (slot != WINDOW_SLOT_NONE)
            DAT_104b7200.m_7654[slot] = FUN_1003b180;
        if (DAT_104c02e8 != 0 && DAT_104c02ec == 0)
        {
            DAT_104c02c0 = (DAT_104c02e8 - 1) / SLOTS_PER_PAGE;
            DAT_104b7200.SetWindowCursor(WINDOW_SLOTS, (DAT_104c02e8 - 1) % SLOTS_PER_PAGE);
            DAT_104c02ec = 1;
        }
        DAT_104b7200.SetWindowLayout(WINDOW_SLOTS, 0, 0x13, &DAT_104c02c0, &DAT_1030c7ec);
        DAT_104bff04 = -1;
        for (int i = 0; i < SLOTS_PER_PAGE; i++)
        {
            int map;
            int playTime;
            char playerName[40];
            int costume;
            sprintf(path, SAVE_NAME_FORMAT, DAT_104c02c0 * SLOTS_PER_PAGE + i);
            FUN_10010d10(path, &map, &playTime, playerName, &costume);
            const char *text;
            int *picked;
            if (playerName[0] == 0)
            {
                text = TEXT_SLOT_EMPTY;
                picked = &DAT_1030c770;
            }
            else
            {
                FormatSlotLine(path, scratch, playerName, playTime, costume, i);
                text = path;
                picked = &DAT_1030c7e8;
            }
            DAT_104b7200.AddFlagEntry(WINDOW_SLOTS, text, picked);
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
    if (DAT_1030c7f0 == 0)
        return 0;
    CloseWindowInline(WINDOW_YESNO_PAGE);
    CloseWindowInline(WINDOW_YESNO);
    memset(DAT_10c62250, 0, sizeof(DAT_10c62250));
    memset(DAT_10c62650, 0, sizeof(DAT_10c62650));
    DAT_1020e0f8[0] = 0;
    CloseWindowInline(WINDOW_MAIN_MENU);
    CloseWindowInline(WINDOW_SLOTS);
    CloseWindowInline(WINDOW_PAGE_B);
    MessageWindow_SetTitle(0);
    DAT_1030c7f0 = 0;
    sprintf(DAT_1032d6a8, SAVE_NAME_FORMAT, DAT_104c02c0 * SLOTS_PER_PAGE + DAT_104bff04);
    FUN_1005cdc0(DAT_1032d6a8, 0);
    DAT_104c02e8 = DAT_104c02c0 * SLOTS_PER_PAGE + 1 + DAT_104bff04;
    DAT_104bfd18 = 1;
    FUN_1015fe20(DAT_10f65370, "CONFIG", "LSF", DAT_104c02e8);
    DAT_104c02ec = 1;
    rand();
    if (DAT_10c62ed0 == MAP_STAGE)
    {
        DAT_1030c500[0] = 1;
        DAT_104c0308 = 1;
        return 0;
    }
    Script_ValueCommand(0x1868b, 0);
    if (DAT_10c62ed0 == MAP_TITLE || (DAT_10c62ed0 != 2 && DAT_10c62ed0 != 3 && DAT_10c62ed0 != 4 && DAT_10c62ed0 != 5 && DAT_10c62ed0 != 0xc &&
        DAT_10c62ed0 != DEBUG_MAP_SECOND))
    {
        DAT_10c62ed0 = MAP_TITLE;
        DAT_104bf8c0 = 0xf;
        DAT_104bf8bc = 0;
        DAT_104bfd50 = 1;
        DAT_104bf8ec = 0;
        DAT_1032d4b0 = 0;
        DAT_104bf8f0 = FADE_FULL;
        DAT_104bf8e8 = FADE_FULL;
        DAT_104bf8e4 = 0;
        DAT_1020d9e0.M_10001550();
        DAT_1065cb28.M_1013edb0();
        DAT_10c6356c = 0;
        Script_ValueCommand(0x271b, 10);
        Script_ValueCommand(0x2774, 0);
        Script_ValueCommand(99999, 0);
        DAT_10c63518 = 0;
        return 0;
    }
    if (DAT_10c62ed0 == 2 || DAT_10c62ed0 == 4 || DAT_10c62ed0 == 5)
    {
        DAT_10db9240 = 0;
        DAT_10db9244 = 0;
        DAT_10db9248 = 0;
    }
    if (DAT_10c62ed0 == 3)
    {
        DAT_10db9240 = 204.0f;
        DAT_10db9244 = -2.0f;
    }
    else if (DAT_10c62ed0 == 0xc)
    {
        DAT_10db9240 = -240.0f;
        DAT_10db9244 = 1234.0f;
    }
    else if (DAT_10c62ed0 == DEBUG_MAP_SECOND)
    {
        DAT_10db9240 = -123.0f;
        DAT_10db9244 = 1266.0f;
    }
    if (DAT_10c62ed0 == 3 || DAT_10c62ed0 == 0xc || DAT_10c62ed0 == DEBUG_MAP_SECOND)
        DAT_10db9248 = 0;
    DAT_1021cff8 = 1;
    DAT_103086a8 = 0;
    DAT_1030c900 = 0;
    DAT_1032d4b0 = -10.0f / 3.0f;
    DAT_1042de20 = DAT_10c62ed0;
    DAT_104bf8bc = 0;
    DAT_104bf8c0 = 1;
    DAT_104bf8e4 = 0xf;
    DAT_104bf8e8 = 0;
    DAT_104bf8ec = 0;
    DAT_104bf8f0 = FADE_FULL;
    DAT_104bfd50 = 1;
    DAT_104c00d0 = 0;
    return 0;
}
