// The HUD of the field (game mode 15): the scene callback DAT_10f6553c that Field_Update installs. Draws the margin
// bars of a wide back buffer, the scene passes, then the stamina gauge, the health and the coin and gem counts along
// the bottom of the screen, the strength bars of the overlay mode, the held item frames, the shop and field menus,
// the windows, the "enter" and "talk" prompt over the pointer, the map movie, the status panel and the overlays.
// Coordinates are 640x480 screen pixels; the 2D parts are shifted by (DAT_10f654b4, DAT_10f654b8), the margins of a
// wider back buffer. Hud_DrawFrame (hud_frame.cpp) is the same screen of the other game modes with the HUD along the
// top.

#include <math.h>
#include <stdlib.h>

#include "anim_model.h"
#include "char_slot.h"
#include "char_tables.h"
#include "char_vars.h"
#include "script_vars.h"
#include "debris.h"
#include "render_device.h"
#include "scene_callback.h"
#include "shaga.h"
#include "text_window.h"
#include "tracker.h"
#include "world_renderer.h"
#include "dodge_game.h"
#include "field_item_menu.h"
#include "font_renderer.h"
#include "game.h"
#include "game_load_data.h"
#include "game_vars.h"
#include "hud_digits.h"
#include "hud_frame.h"
#include "field_hud.h"
#include "message_labels.h"
#include "menu_scene.h"
#include "item_info.h"
#include "matrix4.h"
#include "model_bank.h"
#include "node.h"
#include "render_state.h"
#include "action_select_vars.h"
#include "effect_manager.h"
#include "shop_menu.h"
#include "field_item_menu_draw.h"
#include "field_update.h"
#include "render_vars.h"
#include "scene_draw_vars.h"
#include "script_system.h"
#include "status_panel.h"
#include "texture_load.h"
#include "texture_object.h"
#include "ui_vars.h"
#include "vec4.h"

// The texture slots of the HUD sheets, all fields of the Game object.
#define HUD_TEXTURE(field) DAT_118d0ba8[DAT_104b7200.field]

// Capacity of the stamina gauge: 300 normally, 400 once the bank holds 400 (DAT_10c64ad0).
#define GAUGE_SMALL_CAPACITY 300
#define GAUGE_LARGE_CAPACITY 400

// The gauge and the pickup counter sit this far right of the layout of Hud_DrawFrame (which draws them at the top).
#define GAUGE_SHIFT_X 0x52

// Length in pixels of the health bar, and the number of frames the arrow-prompt fades over.
#define HEALTH_BAR_LENGTH 110
#define PROMPT_FADE_FRAMES 15.0f

// Bars in each of the two rows of the key-held gauges.
#define BAR_COUNT 25

// Number of window slots of the Game object.
#define WINDOW_SLOT_COUNT 3
// A window counts as "in the way" of the map movie once it has been opening for more than this many frames.
#define WINDOW_BUSY_FRAMES 16

// Shown by the prompt over the pointer: "enter" and "talk" in Shift-JIS.
#define PROMPT_TEXT_ENTER "\x93\xfc\x82\xe9"
#define PROMPT_TEXT_TALK "\x82\xa9\x82\xa2\x82\xed"

// The status panel is fully open in this state of DAT_1020c330 (status_panel_text.cpp).
#define STATUS_PANEL_OPEN 15

// 0x100bb5e0 (FieldHud_Draw; C++ linkage: the original has an EH frame for the scratch matrix). The whole frame of
// the field HUD, see the file comment. Returns 0.
int FieldHud_Draw(void)
{
    FUN_1013f070(RENDER_CLEAR_DEPTH, 0xffff, 0);
    if (DAT_10f654b4 != 0)
    {
        int marginX = DAT_10f654b4 * 640 / DAT_10202b2c;
        int marginY = DAT_10f654b8 * 480 / DAT_10202b30;
        FUN_1013f070(RENDER_CLEAR_DEPTH, 0xffff, 0);
        HUD_TEXTURE(texMenu2).DrawTinted(marginX + 640, 0, 640, 480, 0.0f, 0.0f, 0.0f, 0.0f, 0.7f, 1.0f, 0.0f, 0.0f);
        HUD_TEXTURE(texMenu2).DrawTinted(0, marginY + 480, marginX + 640, 480, 0.0f, 0.0f, 0.0f, 0.0f, 0.7f, 1.0f,
                                         0.0f, 0.0f);
    }
    if (DAT_10c63384 == 0 && DAT_10f65484 != 0)
    {
        FUN_1000d6b0(&DAT_1065ca08[1]);
        FUN_1000cf80(&DAT_1065ca08[1]);
    }
    Matrix4 scratch;
    FUN_1013f070(RENDER_CULL_MODE, 0, 0);
    if (DAT_10c63384 == 0 && DAT_104c02e6 == 0)
    {
        // Coins (left) and gems.
        HUD_TEXTURE(texWaku02).DrawTinted(6, 0x1c2, 0x36, 0x1da, 0.25f, 0.875f, 0.5f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f);
        FUN_100131b0(0x2e, 0x1c9, DAT_10daaae8[0].coins, 1.0f, 0, 0.0f, 0.0f);
        HUD_TEXTURE(texWaku02).DrawTinted(0x86, 0x1c2, 0xb6, 0x1da, 0.0f, 0.875f, 0.25f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f);
        FUN_100131b0(0xae, 0x1c9, DAT_10c6670c, 1.0f, 0, 0.0f, 0.0f);

        // The health bar with its two labels, and the digits of the health under it.
        float grey = (float)DAT_104bf88c * 0.049999997f;
        HUD_TEXTURE(texWaku03).DrawTinted(6, 0x1b5, 0x86, 0x1be, 0.314453125f, 0.255859375f, 0.873046875f,
                                          0.287109375f, 0.7f, 0.8f, 0.0f, 0.0f);
        HUD_TEXTURE(texWaku03).DrawTinted(8, 0x1b8,
                                          DAT_10daaae8[0].stats[STAT_HEALTH] * HEALTH_BAR_LENGTH /
                                                  DAT_10daaae8[0].stats[STAT_MAX_HEALTH] + 12,
                                          0x1bb, 0.01953125f, 0.28125f, 0.01953125f, 0.28125f, 0.7f, 0.8f, 0.0f,
                                          grey);
        HUD_TEXTURE(texWaku02).DrawTinted(8, 0x1a6, 0x24, 0x1b6, 0.0f, 0.0f, 0.125f, 0.0625f, 0.7f, 1.0f, 0.0f, 0.0f);
        HUD_TEXTURE(texWaku02).DrawTinted(0x49, 0x1a6, 0x59, 0x1b6, 0.25f, 0.0f, 0.3125f, 0.0625f, 0.7f, 1.0f, 0.0f,
                                          0.0f);
        // One digit of the maximum health per iteration, drawn twice (columns 77 and 124, both stepping left by 12).
        int place = 10;
        int digitX = 0x4d;
        int digitX2 = 0x7c;
        while (DAT_10daaae8[0].stats[STAT_MAX_HEALTH] / (place / 10 > 1 ? place / 10 : 1) != 0)
        {
            FUN_10012d90(digitX, 0x1a7,
                         DAT_10daaae8[0].stats[STAT_MAX_HEALTH] % place / (place / 10 > 1 ? place / 10 : 1), 1.0f, 0,
                         -0.1f, 0.0f);
            FUN_10012d90(digitX2, 0x1a7,
                         DAT_10daaae8[0].stats[STAT_MAX_HEALTH] % place / (place / 10 > 1 ? place / 10 : 1), 1.0f, 0,
                         -0.1f, 0.0f);
            place *= 10;
            digitX -= 12;
            digitX2 -= 12;
        }
    }
    DAT_10202b34 += DAT_10f654b4;
    DAT_10202b38 += DAT_10f654b8;
    if (DAT_10c63384 == 0 && DAT_10f65484 == 0)
    {
        FUN_1000d6b0(&DAT_1065ca08[1]);
        FUN_1000cf80(&DAT_1065ca08[1]);
    }
    if (DAT_104bfd78 == 0)
    {
        FUN_101999a0(&DAT_1065ca08[0], &DAT_1065ca08[1], DAT_10c67ef0, 2);
        DAT_10daaa80.Draw(DAT_1065ca08[1], &DAT_1065ca08[2], 4);
    }
    DAT_10202b34 -= DAT_10f654b4;
    DAT_10202b38 -= DAT_10f654b8;
    if (DAT_104bfe6c == 0 && DAT_104c02e6 == 0 && DAT_10c63384 == 0)
    {
        int bank = DAT_10c64ad0;
        int drillLevel = DAT_10c64ad4;
        int stamina = DAT_10c64ad8;
        int rows = (bank != 0) + 2;
        if (rows == 2)
        {
            if (stamina >= GAUGE_SMALL_CAPACITY)
                stamina = GAUGE_SMALL_CAPACITY;
        }
        else if (rows == 3 && stamina >= GAUGE_LARGE_CAPACITY)
            stamina = GAUGE_LARGE_CAPACITY;
        int frameX = -329;
        if (rows == 3)
            frameX = -349;
        frameX += GAUGE_SHIFT_X;
        HUD_TEXTURE(texWaku03).DrawTinted(frameX + 0x1fe, 0x1c7, frameX + 0x216, 0x1dd, 0.625f, 0.0625f, 0.75f,
                                          0.15625f, 0.7f, 0.7f, 0.0f, 0.0f);
        HUD_TEXTURE(texWaku03).DrawTinted(frameX + 0x202, 0x1c9, frameX + 0x212, 0x1d9,
                                          (float)((drillLevel + 10) * 16) / 256.0f, 0.1875f,
                                          (float)((drillLevel + 11) * 16) / 256.0f, 0.25f, 0.7f, 0.9f, 0.0f, 0.0f);
        int barX = -319;
        if (rows == 3)
            barX = -343;
        barX += GAUGE_SHIFT_X;
        int barLeft = barX + 0x21c;
        HUD_TEXTURE(texWaku03).DrawTinted(barX + 0x1fc, 0x1cb, barLeft, 0x1da, 0.125f, 0.12890625f, 0.25f, 0.1875f,
                                          0.68f, 0.7f, 0.0f, 0.0f);
        HUD_TEXTURE(texWaku03).DrawTinted(barLeft, 0x1cb, barX + 0x24e, 0x1da, 0.25f, 0.12890625f, 0.37109375f,
                                          0.1875f, 0.68f, 0.7f, 0.0f, 0.0f);
        HUD_TEXTURE(texWaku03).DrawTinted(barX + 0x24e, 0x1cb, barX + 0x280, 0x1da, 0.25f, 0.12890625f, 0.37109375f,
                                          0.1875f, 0.68f, 0.7f, 0.0f, 0.0f);
        HUD_TEXTURE(texWaku03).DrawTinted(barX + 0x280, 0x1cb, barX + 0x2b2, 0x1da, 0.25f, 0.12890625f, 0.37109375f,
                                          0.1875f, 0.68f, 0.7f, 0.0f, 0.0f);
        if (rows == 2)
        {
            HUD_TEXTURE(texWaku03).DrawTinted(barX + 0x2b2, 0x1cb, barX + 0x2d2, 0x1da, 0.375f, 0.12890625f, 0.5f,
                                              0.1875f, 0.68f, 0.7f, 0.0f, 0.0f);
            if (stamina >= GAUGE_SMALL_CAPACITY)
            {
                HUD_TEXTURE(texWaku03).DrawTinted(
                    barLeft, 0x1cd, (int)((float)((drillLevel + 1) * 5000) / 100.0f + (float)barLeft), 0x1d5,
                    0.00390625f, 0.6328125f, 0.00390625f, 0.63671875f, 0.69f, 0.7f, 0.0f,
                    (float)DAT_104bf88c * 0.049999997f);
            }
            else
            {
                int filledRight = (int)((float)(drillLevel * 5000) / 100.0f + (float)barLeft);
                HUD_TEXTURE(texWaku03).DrawTinted(barLeft, 0x1cd, filledRight, 0x1d5, 0.00390625f, 0.6328125f,
                                                  0.00390625f, 0.63671875f, 0.69f, 0.7f, 0.0f,
                                                  (float)DAT_104bf88c * 0.049999997f);
                HUD_TEXTURE(texWaku03).DrawTinted(filledRight, 0x1cd,
                                                  (int)((float)(stamina * 50) / 100.0f + (float)barLeft), 0x1d5,
                                                  0.46875f, 0.9453125f, 0.46875f, 0.9453125f, 0.69f, 0.7f, 0.0f,
                                                  0.0f);
            }
        }
        else
        {
            HUD_TEXTURE(texWaku03).DrawTinted(barX + 0x2b2, 0x1cb, barX + 0x2e4, 0x1da, 0.25f, 0.12890625f,
                                              0.37109375f, 0.1875f, 0.68f, 0.7f, 0.0f, 0.0f);
            HUD_TEXTURE(texWaku03).DrawTinted(barX + 0x2e4, 0x1cb, barX + 0x304, 0x1da, 0.375f, 0.12890625f, 0.5f,
                                              0.1875f, 0.68f, 0.7f, 0.0f, 0.0f);
            if (rows == 3 && stamina >= GAUGE_LARGE_CAPACITY)
            {
                HUD_TEXTURE(texWaku03).DrawTinted(
                    barLeft, 0x1cd, (int)((float)((drillLevel + 1) * 5000) / 100.0f + (float)barLeft), 0x1d5,
                    0.00390625f, 0.6328125f, 0.00390625f, 0.63671875f, 0.69f, 0.7f, 0.0f,
                    (float)DAT_104bf88c * 0.049999997f);
            }
            else
            {
                int filledRight = (int)((float)(drillLevel * 5000) / 100.0f + (float)barLeft);
                HUD_TEXTURE(texWaku03).DrawTinted(barLeft, 0x1cd, filledRight, 0x1d5, 0.00390625f, 0.6328125f,
                                                  0.00390625f, 0.63671875f, 0.69f, 0.7f, 0.0f,
                                                  (float)DAT_104bf88c * 0.049999997f);
                HUD_TEXTURE(texWaku03).DrawTinted(filledRight, 0x1cd,
                                                  (int)((float)(stamina * 50) / 100.0f + (float)barLeft), 0x1d5,
                                                  0.46875f, 0.9453125f, 0.46875f, 0.9453125f, 0.69f, 0.7f, 0.0f,
                                                  0.0f);
            }
        }

        // The pickup counter right of the gauge.
        int pickupX = 0;
        if (DAT_10c64ad0 != 0)
            pickupX = 0x10;
        HUD_TEXTURE(texWaku01).DrawTinted(pickupX + GAUGE_SHIFT_X + 0x190, 0x1ca, pickupX + GAUGE_SHIFT_X + 0x1b0,
                                          0x1da, 0.1875f, 0.4375f, 0.3125f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f);
        float pickupShake = (float)DAT_104c0119 / 10.0f;
        FUN_10012990(pickupX + GAUGE_SHIFT_X + 0x1b0, 0x1ca, DAT_10daaae8[0].stats[STAT_PICKUPS], 1.0f, 3,
                     pickupShake, pickupShake);
    }
    if (DAT_104bfe6c == 0)
        FieldItemMenu_Draw();
    if (DAT_104bfd78 != 0)
        FUN_1000f4d0();
    DAT_104b7200.DrawWindows(DAT_104bf88c);

    // The bars of the key-held gauges of the overlay mode: two rows of 25 bars, growing towards the middle.
    if (DAT_104bfe1c != 0 && DAT_104bfe6c == 0)
    {
        FUN_1013f070(RENDER_BLEND_MODE, BLEND_ADD, 0);
        float scale = 1.0f;
        int barX = 0x104;
        for (int bar = 0; bar < BAR_COUNT; bar++)
        {
            float height = scale * DAT_104bff24[bar] * 0.2f;
            if (height >= 0.0f)
            {
                if (64.0f < height)
                    height = 64.0f;
            }
            else
                height = 0.0f;
            int steps = (int)(height / 3.0f);
            int lift = -(steps * 3);
            HUD_TEXTURE(texWaku03).DrawTinted(barX, lift + 0x160, barX + 8, 0x160, 0.50390625f,
                                              (float)(lift + 0x40) / 256.0f, 0.52734375f, 0.25f, 0.8f, 1.0f,
                                              0.0f, 0.0f);
            scale += 0.13f;
            barX += 8;
        }
        scale = 1.0f;
        barX = 0xe4;
        for (int bar = 0; bar < BAR_COUNT; bar++)
        {
            float height = scale * DAT_104bffa4[bar] * 0.2f;
            if (height >= 0.0f)
            {
                if (64.0f < height)
                    height = 64.0f;
            }
            else
                height = 0.0f;
            int steps = (int)(height / 3.0f);
            int lift = -(steps * 3);
            HUD_TEXTURE(texWaku03).DrawTinted(barX, lift + 0x160, barX + 8, 0x160, 0.50390625f,
                                              (float)(lift + 0x40) / 256.0f, 0.52734375f, 0.25f, 0.8f, 1.0f,
                                              0.0f, 0.0f);
            scale += 0.13f;
            barX -= 8;
        }
        FUN_1013f070(RENDER_BLEND_MODE, BLEND_ALPHA, 0);
    }

    // The two held item frames at the top right with the icons of the helmet and the costume.
    if (DAT_104bfe6c == 0 && DAT_10c63384 == 0 && DAT_104c02e6 == 0)
    {
        HUD_TEXTURE(texWaku01).DrawFrameAltPlain(0x22a, 5, 0x25e, 0x39, 0.2f, 0.5f, 1, 0, 0);
        HUD_TEXTURE(texWaku01).DrawFrameAltPlain(0x1fa, 5, 0x22e, 0x39, 0.2f, 0.5f, 1, 0, 0);
        HUD_TEXTURE(texWaku01).DrawTinted(0x202, 5, 0x222, 0x15, 0.375f, 0.4375f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f);
        HUD_TEXTURE(texWaku01).DrawTinted(0x232, 5, 0x252, 0x15, 0.5f, 0.4375f, 0.625f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f);
        if (DAT_10daabb4 > 0)
        {
            int icon = DAT_1088e6b0[DAT_10daabb4].icon;
            float cellU = (float)(icon % 8) * 0.125f;
            float cellV = (float)(icon / 8) * 0.125f;
            HUD_TEXTURE(texIcon01).DrawTinted(0x234, 0xf, 0x254, 0x2f, cellU + 0.002f, cellV + 0.002f,
                                              cellU + 0.125f - 0.002f, cellV + 0.125f - 0.002f, 0.0f, 1.0f, 0.0f,
                                              0.0f);
        }
        if (DAT_10daabb8 > 0)
        {
            int icon = DAT_1088e6b0[DAT_10daabb8].icon;
            float cellU = (float)(icon % 8) * 0.125f;
            float cellV = (float)(icon / 8) * 0.125f;
            HUD_TEXTURE(texIcon01).DrawTinted(0x204, 0xf, 0x224, 0x2f, cellU + 0.002f, cellV + 0.002f,
                                              cellU + 0.125f - 0.002f, cellV + 0.125f - 0.002f, 0.0f, 1.0f, 0.0f,
                                              0.0f);
        }
    }
    if (DAT_104bfd28 != 0)
        ShopMenu_Draw();

    // The prompt over the pointer: a panel with a blinking arrow and "enter" next to a door, "talk" next to a
    // character. It fades in over 15 frames (DAT_1020c2f4).
    if (DAT_104bfe1c == 0)
    {
        if (DAT_1020c2f8 != 0 && DAT_1020c2f4 > 0 && DAT_10c63384 == 0 && DAT_104c02e6 == 0)
        {
            float fade = (float)DAT_1020c2f4 / PROMPT_FADE_FRAMES;
            DAT_104b7200.DrawPanelAlt(0x200, 0xf5, 0x84, 0x40, 0.0f, fade);
            unsigned int blink = DAT_104bf890 >> 4 & 1;
            HUD_TEXTURE(texWaku01).DrawTinted(0x20c, 0x105, 0x224, 0x11d, (float)((blink + 11) << 4) / 256.0f, 0.3125f,
                                              (float)((blink + 12) << 4) / 256.0f, 0.375f, 0.0f, fade, 0.0f, 0.0f);
            DAT_1020e538.DrawOutlined(0x238, 0x109, PROMPT_TEXT_ENTER, 0xffffff, 0.0f, fade, 1.0f);
        }
        else if (DAT_1020c33c > 0)
        {
            float fade = (float)DAT_1020c2f4 / PROMPT_FADE_FRAMES;
            DAT_104b7200.DrawPanelAlt(0x200, 0xf5, 0x84, 0x40, 0.0f, fade);
            unsigned int blink = DAT_104bf890 >> 4 & 1;
            HUD_TEXTURE(texWaku01).DrawTinted(0x20c, 0x105, 0x224, 0x11d, (float)((blink + 11) << 4) / 256.0f, 0.3125f,
                                              (float)((blink + 12) << 4) / 256.0f, 0.375f, 0.0f, fade, 0.0f, 0.0f);
            DAT_1020e538.DrawOutlined(0x238, 0x109, PROMPT_TEXT_TALK, 0xffffff, 0.0f, fade, 1.0f);
        }
        else if (DAT_1020c2f4 != 0)
        {
            float fade = (float)DAT_1020c2f4 / PROMPT_FADE_FRAMES;
            DAT_104b7200.DrawPanelAlt(0x200, 0xf5, 0x84, 0x40, 0.0f, fade);
            unsigned int blink = DAT_104bf890 >> 4 & 1;
            HUD_TEXTURE(texWaku01).DrawTinted(0x20c, 0x105, 0x224, 0x11d, (float)((blink + 11) << 4) / 256.0f, 0.3125f,
                                              (float)((blink + 12) << 4) / 256.0f, 0.375f, 0.0f, fade, 0.0f, 0.0f);
            if (DAT_1020c340 != 0)
                DAT_1020e538.DrawOutlined(0x238, 0x109, PROMPT_TEXT_TALK, 0xffffff, 0.0f, fade, 1.0f);
            else
                DAT_1020e538.DrawOutlined(0x238, 0x109, PROMPT_TEXT_ENTER, 0xffffff, 0.0f, fade, 1.0f);
        }
    }

    // The map movie, drawn only while no menu window is open or opening.
    DAT_10202b34 += DAT_10f654b4;
    DAT_10202b38 += DAT_10f654b8;
    if (DAT_10c63384 == 0 && DAT_104c02e6 == 0)
    {
        int slot;
        for (slot = 0; slot < WINDOW_SLOT_COUNT; slot++)
        {
            if (DAT_104b7200.m_0110[slot] != 0)
                break;
            if (DAT_104b7200.m_008c[slot] != 0 && DAT_104b7200.m_0098[slot] > WINDOW_BUSY_FRAMES)
                break;
        }
        if (slot == WINDOW_SLOT_COUNT)
            FUN_1001ed80(DAT_10c64a5c);
    }
    DAT_10202b34 -= DAT_10f654b4;
    DAT_10202b38 -= DAT_10f654b8;

    // The corner marker at the lower right.
    if (DAT_104bfe6c == 0 && DAT_104bfe78 == 0 && DAT_10c63384 == 0 && DAT_104c02e6 == 0)
    {
        if (DAT_104c0232 != 0)
            HUD_TEXTURE(texWaku03).DrawTinted(0x251, 0x1b1, 0x281, 0x1e1, 0.8125f, 0.3125f, 0.99609375f, 0.49609375f,
                                              0.0f, 1.0f, 0.0f, 0.0f);
        else
            HUD_TEXTURE(texWaku03).DrawTinted(0x250, 0x1b0, 0x280, 0x1e0, 0.8125f, 0.3125f, 0.99609375f, 0.49609375f,
                                              0.0f, 1.0f, 0.0f, 0.0f);
    }
    FUN_100236c0();
    if (DAT_104bf8d4 != 0)
        FUN_1000cc30();
    if (DAT_104bf8c8 != 0)
        FUN_1000eed0();

    // The status panel fades in with the pointer prompt counter and in and out with its own state.
    if (DAT_1020c330 != 0 && DAT_104bfe1c == 0)
    {
        float arrival = (float)DAT_1020c2f4 / PROMPT_FADE_FRAMES;
        float opening = 1.0f - (float)abs(STATUS_PANEL_OPEN - DAT_1020c330) / PROMPT_FADE_FRAMES;
        FUN_1001e340(arrival <= opening ? arrival : opening);
    }
    if (0.1f < DAT_104bf8f0)
        FUN_1000dba0();
    FUN_1007d0a0();
    DAT_1020d9e0.M_100014b0();
    if (DAT_10c63384 != 0 || DAT_104c02e6 != 0)
    {
        FUN_10143e00(0, 0, 640, 0x22, 0, 0.0f, 1.0f);
        FUN_10143e00(0, 0x1be, 640, 480, 0, 0.0f, 1.0f);
    }
    return 0;
}
