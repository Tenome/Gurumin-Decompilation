// Sound slots, menus, character helpers and option flags.

#include <stdlib.h>
#include <string.h>
#include "world.h"
#include "game.h"
#include "anim_model.h"
#include "char_state.h"
#include "char_slot.h"

#include "model_path.h"

#include "char_tables.h"
#include "char_vars.h"
#include "game_vars.h"
#include "render_vars.h"
#include "sound_vars.h"
#include "ui_vars.h"

extern "C" int FUN_10001b40(void);
extern "C" const char *FUN_1013f8a0(const char *format, ...);

// Marks all 0x44c looping-sound slots free (-11) and stops the sound process' loops (command 0x221, sent
// twice when the first call succeeds) unless sound is muted.
extern "C" void FUN_100102e0(void)
{
    if (DAT_104c01b0 == 0)
    {
        for (int i = 0; i < 0x44c; i++)
            DAT_10447338[i] = -11;
        if (FUN_10001b40())
            FUN_10001b40();
    }
}

// Closes the yes/no dialog (menu 0x216) and menu 0x218 and clears the message text. Returns 1.
extern "C" int FUN_100106a0(void)
{
    DAT_104b7200.CloseWindow(0x218);
    DAT_104b7200.CloseWindow(0x216);
    DAT_1020e0f8[0] = 0;
    return 1;
}

// 0x100106d0. Callback of the command list window (Game::m_7654): remembers the command under the cursor
// (DAT_104bfd30). `caption` is not used. Always returns 0.
extern "C" int FUN_100106d0(int cursor, char *caption)
{
    DAT_104bfd30 = cursor;
    return 0;
}

// Marks character `index` as defeated (m_00a5) unless it is busy; returns 1 when busy.
extern "C" int FUN_10011f80(int index)
{
    if (DAT_10210d30[index] != 0)
        return 1;
    DAT_10daaae8[index].m_0141 = 1;
    return 0;
}

// Like the game-over start in engine_base.cpp without the sound: stores `value`, sets the scene timer and plays
// the player's defeat motion.
extern "C" int FUN_10012100(int value)
{
    DAT_104c0198 = value;
    DAT_104c00d0 = 0x816;
    DAT_10db6ee8[0].PlayMotion(0x59, 1, 5, 1.0f);
    rand();
    return 0;
}

// Reloads character `index`'s model from the model path table.
extern "C" int FUN_100121e0(int index)
{
    return DAT_10db6ee8[index].LoadModel(FUN_1013f8a0(DAT_10b314c8[DAT_10daaae8[index].params[PARAM_MODEL]].path, 0));
}

// Adds character `index`'s position to the list of positions the camera keeps in view.
extern "C" int FUN_100143c0(int index)
{
    // the camera list holds Vec3 pointers; the matrix's translation row (_41, _42, _43) is the character's position
    DAT_1032d5b8[DAT_1049f0f0] = &DAT_10db6ee8[index].matrix.translation;
    DAT_1021bbc0[DAT_1049f0f0] = 0;
    DAT_1049f0f0++;
    return 1;
}

// Copies two option settings into their runtime flags: DAT_10c66590 (0..2) to DAT_104c0131 and DAT_10c66594
// (on/off) to DAT_104c0130. Not byte-identical yet (see open.tsv).
extern "C" void FUN_10015910(void)
{
    DAT_104c0131 = 0;
    if (DAT_10c66590 == 0)
        DAT_104c0131 = 0;
    if (DAT_10c66590 == 1)
        DAT_104c0131 = 1;
    if (DAT_10c66590 == 2)
        DAT_104c0131 = 2;
    DAT_104c0130 = 1;
    if (DAT_10c66594 == 0)
        DAT_104c0130 = 0;
}

// Returns the NPC slot (0x5c..0x7f) whose script id is `id`, or -1.
extern "C" int FUN_10017aa0(int id)
{
    for (int i = 0x5c; i < 0x80; i++)
    {
        if (DAT_10219ec0[i] == id)
            return i;
    }
    return -1;
}

// Returns the NPC slot of kind 'F', 'M' or 'N' (shop and town people) whose character id is `id`, or -1.
extern "C" int FUN_10017ad0(int id)
{
    for (int i = 0x5c; i < 0x80; i++)
    {
        char kind = DAT_1021f3c0[i];
        if ((kind == 'F' || kind == 'M' || kind == 'N') && DAT_10db6ee8[i - 1].nextSlotId == id)
            return i;
    }
    return -1;
}

#include "node.h"

#include "engine_memory.h"
extern "C" int FUN_1000cb30(const char *text);
extern "C" int FUN_1000cb90(const char *text);      // Caption_Set
extern "C" int FUN_10174c70(int slot, int force);   // releases a texture slot

// 0x1003a9d0. Advances the fade (DAT_104bf8f0 by DAT_1032d4b0 per frame, the target on the last frame). Once the
// fade is under way while the overlay mode is open, the mode is closed: its menus (0x22c, 0x218, 0x216) and caption,
// its model, texture slots and state are released.
extern "C" void FUN_1003a9d0(void)
{
    if (DAT_104bf8e4 > 0)
    {
        DAT_104bf8e4--;
        DAT_104bf8f0 = DAT_1032d4b0 + DAT_104bf8f0;
        if (DAT_104bf8e4 == 0)
            DAT_104bf8f0 = DAT_104bf8e8;
    }
    if (DAT_104bf8f0 != 0.0f && DAT_104bfe1c != 0)
    {
        DAT_1030c500[0x13] = 0;
        DAT_104bfd78 = 0;
        DAT_104b7200.CloseWindow(0x22c);
        DAT_104b7200.CloseWindow(0x218);
        DAT_104b7200.CloseWindow(0x216);
        DAT_1020e0f8[0] = 0;
        FUN_1000cb30("");
        FUN_1000cb90("");
        DAT_104bf8d4 = 1;
        if (DAT_104c03c8 != 0)
        {
            DAT_104c03dc = 0;
            DAT_104c03c8->Release();
            DAT_11b1ba40(DAT_104c03c8);
            DAT_104c03c8 = 0;
            DAT_104c03cc = 0;
            DAT_104c03d0 = 0;
            DAT_104c03d4 = 0;
            DAT_104c03d8 = 0;
            DAT_104c03e0 = 0;
            DAT_104c03e4 = 0;
            for (int i = 0; i < 0x22; i++)
            {
                if (DAT_1030b870[i] > -1)
                {
                    FUN_10174c70(DAT_1030b870[i], 1);
                    DAT_1030b870[i] = -1;
                }
            }
        }
        DAT_104b7200.CloseWindow(0x218);
        DAT_104b7200.CloseWindow(0x216);
        memset(DAT_1030c500, 0, sizeof(DAT_1030c500));
        DAT_1020e0f8[0] = 0;
        DAT_104bfe1c = 0;
    }
}
