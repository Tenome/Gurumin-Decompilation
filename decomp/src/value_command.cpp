// The script value commands: one big if-chain over the command number that drives the three text windows and their
// panel lines, the furniture of the item stand, the costume bits, the result card of a time run, the character
// slots and carried items, the player's turning and attacks and many small getters and setters. Arguments and
// results travel in the registers DAT_10c6356c.. (script flags 0x1a4..).

#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "vec4.h"
#include "matrix4.h"
#include "node.h"
#include "anim_model.h"
#include "char_slot.h"
#include "char_state.h"
#include "char_tables.h"
#include "char_vars.h"
#include "game.h"
#include "game_vars.h"
#include "map_vars.h"
#include "map_records.h"
#include "camera_vars.h"
#include "scene_command.h"
#include "script_system.h"
#include "text_window.h"
#include "ui_vars.h"
#include "effect_holds.h"
#include "effect_manager.h"
#include "font_renderer.h"
#include "item_info.h"
#include "zone_roamer.h"
#include "model_bank.h"
#include "bgm_track.h"
#include "player_frame.h"
#include "status_panel.h"
#include "debris.h"
#include "script_vars.h"
#include "script_game_builtin.h"
#include "menu_save_load.h"
#include "shop_menu.h"
#include "map_setup.h"
#include "sound_vars.h"
#include "render_vars.h"
#include "camera_point.h"
#include "value_command.h"

extern "C" int FUN_10174c70(int slot, int force);                          // Texture_Release
extern "C" int FUN_10016580(const char *name);                             // Movie_PlayToTexture
extern "C" int FUN_100120a0(const int *index, int *amount);                // Inventory_Remove
extern "C" int FUN_10012130(int *item, int *count);                        // Inventory_Add
extern "C" int FUN_10056260(int slot, const char *line1, const char *line2, const char *line3, const char *line4);   // Message_Open
extern "C" int FUN_1000ed70(const char *text, const char *subtext, int frames);   // Message_Show
extern "C" int FUN_100f9670(int index);                                    // Char_UpdateByKind
extern "C" int FUN_100165e0(int on);                                       // Player_SetHeadTracking
extern "C" int FUN_1003b670(int reason);                                   // GameOver_Start
extern "C" int FUN_10055ee0(void);                                         // Player_KnockBack
extern "C" int FUN_10057b90(int object);                                   // Switch_Release
extern "C" int FUN_10057c40(int holder, Node *node, int value);            // Switch_Hold
extern "C" int FUN_10011b70(int kind, int power, int unused, Vec4 *from);  // Char_PushAwayFrom
extern "C" int FUN_10011fb0(int frozen);                                   // Water_SetFrozen
extern "C" int FUN_1000dab0(int id, int value, int frames, int mode);      // Bgm_FadeGroup300
extern "C" int FUN_1000db30(int id, int value, int frames);                // Bgm_FadeGroup100
extern "C" int FUN_1000e750(void);                                         // AntiTamper_Store
extern "C" int FUN_1000e810(void);                                         // AntiTamper_StoreArrays
extern "C" long FUN_10001b70(long volume);                                 // SoundArc_Message227
extern "C" int FUN_1013f880(void);                                         // Random
#include "scene_command_ids.h"
extern "C" const char *FUN_1013f8a0(const char *name, int folder);         // Path_Data
extern "C" float FUN_10001f80(float y, float x);                           // atan2f

#define WINDOW_PICTURE_NONE (-1)
#define KAGU_NODE_NAME "kagunode"
#define FIRST_CHEST_FLAG_BASE 0x4e21
#define PI_APPROX 3.1415f
#define DEGREES_HALF_TURN 180.0f

// The text window the registers address (register 0 = its number).
#define CURRENT_WINDOW DAT_10255960[DAT_10c6356c]

// Window fade step: the opening code of VALUE_WINDOW_SET_FADE (and -1) stores the frame count in the window.
#define SET_WINDOW_FADE(value) \
    do \
    { \
        CURRENT_WINDOW.m_0018 = (value); \
        CURRENT_WINDOW.m_001c = (value); \
        CURRENT_WINDOW.m_0020 = 0; \
    } while (0)

// Places the furniture of kagu spot `spot` when the costume mask `bits` holds `bit` (VALUE_KAGU_PLACE_BOUGHT).
#define PLACE_IF_BOUGHT(bit, spot) \
    if ((bits & (bit)) != 0) \
        Script_ValueCommand(VALUE_KAGU_PLACE, spot);

// The costume items (49..78) and the bit each one sets in the costume mask DAT_10c64adc, in the order of the
// original's case bodies. Expanded into the switch of VALUE_COSTUME_SET_BIT and into the one of VALUE_COSTUME_CLEAR_ALL.
#define FOR_EACH_COSTUME_ITEM(X) \
    X(74, 0x1) \
    X(68, 0x2) \
    X(59, 0x4) \
    X(75, 0x8) \
    X(73, 0x10) \
    X(60, 0x100) \
    X(69, 0x200) \
    X(63, 0x400) \
    X(66, 0x800) \
    X(49, 0x1000) \
    X(67, 0x10000) \
    X(65, 0x20000) \
    X(70, 0x40000) \
    X(72, 0x80000) \
    X(71, 0x100000) \
    X(58, 0x1000000) \
    X(64, 0x2000000) \
    X(50, 0x4000000) \
    X(51, 0x8000000) \
    X(78, 0x10000000)

#define COSTUME_CASE_SET(item, bit) \
    case item: \
        costumes |= bit; \
        break;

#define COSTUME_CASE_CLEAR(item, bit) \
    case item: \
        costumes &= ~(bit); \
        break;

// The price after the helmet discounts (half, 0.9 or 0.95 of it) of `price`.
static int DiscountedPrice(int price)
{
    if ((DAT_104c02c8 & 0x300000) == 0x300000)
        return (int)((float)price * 0.5f);
    if ((DAT_104c02c8 & 0x200000) != 0)
        return (int)((float)price * 0.9f);
    if ((DAT_104c02c8 & 0x100000) != 0)
        return (int)((float)price * 0.95f);
    return price;
}

// Starts effect `effect` at the player's position on a fresh placement matrix (result card fanfares).
static __forceinline void StartResultEffect(int effect, int frame)
{
    Matrix4 placement;
    Vec4 position;
    placement.SetIdentity();
    position.Fill(0.0f);
    placement.SetTranslation(DAT_10db6ee8[0].matrix.GetTranslation());
    DAT_10daaa80.Start(effect, position, -1, -1, 0, 0, frame, &placement, 0, 0);
}

// The angle (radians) of the step from `from` to `to`, turned so that 0 looks along -y.
static float HeadingBetween(const Vec4 &from, const Vec4 &to)
{
    return FUN_10001f80(to.x - from.x, from.y - to.y);
}

// 0x101011b0 (Script_ValueCommand). Runs value command `id` with `value` (see ValueCommand in value_command.h). The
// commands from 30000 on are passed to the scene commands, and from 40000 on to Script_SystemCommand first. The
// commands read and write the registers DAT_10c6356c.. directly. Unknown numbers do nothing. Returns 0.
int Script_ValueCommand(int id, int value)
{
    if (id > 39999)
        Script_SystemCommand(id, value);
    if (id > 29999)
    {
        Script_SceneCommand(id, value);
        return 0;
    }
    Vec4 pointA;
    Vec4 pointB;
    if (id == -1)
    {
        SET_WINDOW_FADE(value);
    }
    else if (id == VALUE_WINDOW_CLOSE)
    {
        int picture = DAT_10255960[value].picture;
        DAT_10c6356c = value;
        for (int line = 0; line < 8; line++)
            DAT_1020da50[line][0] = 0;
        if (picture >= 0)
        {
            if (DAT_1049efd8[picture].slot == DAT_102028cc)
            {
                for (int record = 0; record < PICTURE_RECORD_COUNT; record++)
                {
                    if (DAT_1049efd8[record].slot == -1)
                    {
                        DAT_1049efd8[record].slot = DAT_102028cc;
                        DAT_1049efd8[picture].slot = -1;
                        break;
                    }
                }
            }
            else
                FUN_10174c70(DAT_1049efd8[picture].slot, 1);
        }
        DAT_10255960[value].picture = WINDOW_PICTURE_NONE;
        DAT_10c63578 = 0;
    }
    else if (id == VALUE_WINDOW_SET_STYLE)
    {
        if (value == 8)
        {
            CURRENT_WINDOW.style = TEXT_WINDOW_STYLE_PICTURE_FULL;
            DAT_10c63570 = value;
        }
        else if (value < 7)
        {
            if (value < 4)
            {
                CURRENT_WINDOW.style = TEXT_WINDOW_STYLE_PLAIN;
                DAT_10c63570 = value;
            }
            else
            {
                CURRENT_WINDOW.style = 1;
                DAT_10c63570 = value - 4;
            }
        }
        else
        {
            CURRENT_WINDOW.style = TEXT_WINDOW_STYLE_PICTURE_RECT;
            DAT_10c63570 = value;
        }
    }
    else if (id == VALUE_WINDOW_OPEN)
    {
        if (CURRENT_WINDOW.style == TEXT_WINDOW_STYLE_PICTURE_RECT || CURRENT_WINDOW.style == TEXT_WINDOW_STYLE_PICTURE_FULL)
        {
            CURRENT_WINDOW.y = DAT_10c6357c;
            CURRENT_WINDOW.x = DAT_10c63578;
            CURRENT_WINDOW.bottom = DAT_10c63584;
            CURRENT_WINDOW.right = DAT_10c63580;
            CURRENT_WINDOW.tile = DAT_10c63598;
            CURRENT_WINDOW.kind = TEXT_WINDOW_FRAMED;
            CURRENT_WINDOW.progress = 0;
            CURRENT_WINDOW.m_0018 = 0;
            CURRENT_WINDOW.m_001c = value;
            CURRENT_WINDOW.m_0020 = 1;
            PictureRecord &record = DAT_1049efd8[CURRENT_WINDOW.picture];
            record.left = DAT_10c63588;
            record.top = DAT_10c6358c;
            record.right = DAT_10c63590;
            record.bottom = DAT_10c63594;
            DAT_10c63578 = 0;
            DAT_10c63574 = 1;
        }
        else
        {
            int lineCount = 0;
            for (int line = 7; line >= 0; line--)
            {
                if (DAT_1020da50[line][0] != 0)
                {
                    lineCount = line + 1;
                    break;
                }
            }
            if (DAT_10c6356c == 2)
                lineCount--;
            int width = 0;
            for (int line = 0; line < lineCount; line++)
            {
                float scale = (line == 0 && DAT_10c6356c == 0) ? 1.1f : 0.9f;
                int lineWidth = DAT_1020e538.TextWidth(DAT_1020da50[line], scale);
                if (width < lineWidth)
                    width = lineWidth;
            }
            if (DAT_10c63570 == 1 || DAT_10c63570 == 3)
                width += lineCount < 1 ? 0x8e : 0x98;
            else
                width += 0x10;
            int height;
            if (lineCount < 2)
                height = (DAT_10c6356c == 0) * 2 + 0x14;
            else if (DAT_10c6356c == 0)
                height = lineCount * 0x14 + 2;
            else
                height = lineCount * 0x14;
            height += 0x12;
            if (height < 0x90 && (DAT_10c63570 == 1 || DAT_10c63570 == 3))
                height = 0x90;
            CURRENT_WINDOW.x = 0x140 - (width >> 1);
            CURRENT_WINDOW.right = (width >> 1) + 0x140;
            CURRENT_WINDOW.bottom = DAT_10c63578 + 200 + (height >> 1);
            CURRENT_WINDOW.kind = TEXT_WINDOW_FRAMED;
            CURRENT_WINDOW.y = DAT_10c63578 - (height >> 1) + 200;
            CURRENT_WINDOW.progress = 0;
            CURRENT_WINDOW.m_0018 = 0;
            CURRENT_WINDOW.m_001c = value;
            CURRENT_WINDOW.m_0020 = 1;
            DAT_10c63574 = 1;
        }
    }
    else if (id == VALUE_WINDOW_SET_FADE)
    {
        SET_WINDOW_FADE(value);
    }
    else if (id == VALUE_MESSAGE_OPEN_PANEL)
    {
        FUN_10056260(0, DAT_1020da50[0], DAT_1020da50[1], DAT_1020da50[2], DAT_1020da50[3]);
        DAT_1020c334 = -1;
    }
    else if (id == VALUE_MESSAGE_OPEN_LINE)
    {
        FUN_10056260(value, DAT_1020da50[DAT_10c6356c], "", "", "");
    }
    else if (id == VALUE_NOP_2774)
    {
    }
    else if (id == VALUE_KAGU_CLEAR)
    {
        if (DAT_10255f08 != 0)
        {
            Node *old = DAT_10255f08->Find(KAGU_NODE_NAME, 0);
            if (old != 0)
                DAT_10255f08->RemoveChild(old, 0);
            Node *holder = DAT_10255f08->AddChild(0, 0x40);
            strcpy(holder->name, KAGU_NODE_NAME);
            holder->flags = NODE_FLAG_NEVER_CULLED;
        }
    }
    else if (id == VALUE_KAGU_PLACE)
    {
        const KaguSpot &spot = DAT_1090865c[value];
        int item = spot.item;
        if (item != -1 && DAT_103150c0[item] == 0 && DAT_10255f08 != 0)
        {
            Node *holder = DAT_10255f08->Find(KAGU_NODE_NAME, 0);
            if (holder == 0)
            {
                Node *created = DAT_10255f08->AddChild(0, 0x40);
                strcpy(created->name, KAGU_NODE_NAME);
                holder = DAT_10255f08->Find(KAGU_NODE_NAME, 0);
                if (holder == 0)
                    return 0;
            }
            Node *furniture = holder->AddChild(0, 0x40);
            furniture->Clone(&DAT_104d2a08[item], 0, 0);
            furniture->matrix = RotationZ((float)spot.angle * 3.1415f / DEGREES_HALF_TURN) *
                                TranslationMatrix((float)spot.x, (float)spot.y, (float)spot.z + 10.0f);
            sprintf(furniture->name, "Kaguda%02d", item);
            furniture->BuildMesh(1, 0);
        }
    }
    else if (id == VALUE_KAGU_PLACE_BOUGHT)
    {
        unsigned int bits = DAT_10c64adc;
        Script_ValueCommand(VALUE_KAGU_CLEAR, 0);
        PLACE_IF_BOUGHT(0x10000000, 0x26)
        PLACE_IF_BOUGHT(0x8000000, 0x23)
        PLACE_IF_BOUGHT(0x4000000, 0x24)
        PLACE_IF_BOUGHT(0x2000000, 0x27)
        PLACE_IF_BOUGHT(0x1000000, 0x25)
        PLACE_IF_BOUGHT(0x100000, 0x2c)
        PLACE_IF_BOUGHT(0x80000, 0x29)
        PLACE_IF_BOUGHT(0x40000, 0x2a)
        PLACE_IF_BOUGHT(0x20000, 0x2b)
        PLACE_IF_BOUGHT(0x10000, 0x28)
        PLACE_IF_BOUGHT(0x1000, 0xb)
        PLACE_IF_BOUGHT(0x800, 0x9)
        PLACE_IF_BOUGHT(0x400, 0xa)
        PLACE_IF_BOUGHT(0x200, 0x7)
        PLACE_IF_BOUGHT(0x100, 0x8)
        PLACE_IF_BOUGHT(0x10, 0x5)
        PLACE_IF_BOUGHT(0x8, 0x3)
        PLACE_IF_BOUGHT(0x4, 0x2)
        PLACE_IF_BOUGHT(0x2, 0x0)
        PLACE_IF_BOUGHT(0x1, 0x4)
    }
    else if (id == VALUE_HALVE_COUNT)
    {
        DAT_10c64a44 = DAT_10c64a48;
        if (DAT_10c65658 == 0 && DAT_10c64a48 > 1)
            DAT_10c64a44 = 1;
        DAT_10c64a44 = DAT_10c64a44 / 2;
    }
    else if (id == VALUE_COSTUME_SET_BIT)
    {
        unsigned int costumes = DAT_10c64adc;
        switch (value)
        {
        FOR_EACH_COSTUME_ITEM(COSTUME_CASE_SET)
        }
        DAT_10c64adc = costumes;
    }
    else if (id == VALUE_COSTUME_COUNT)
    {
        if (value == 4)
        {
            int total = 0;
            for (int group = 0; group < 4; group++)
            {
                Script_ValueCommand(VALUE_COSTUME_COUNT, group);
                total += DAT_10c6356c;
            }
            DAT_10c6356c = total;
        }
        else
        {
            unsigned char shift = (unsigned char)(value * 8);
            unsigned int group = (int)(0xff << (shift & 0x1f) & DAT_10c64adc) >> (shift & 0x1f);
            int count = (group & 1) != 0;
            if ((group & 2) != 0)
                count++;
            if ((group & 4) != 0)
                count++;
            if ((group & 8) != 0)
                count++;
            if ((group & 0x10) != 0)
                count++;
            DAT_10c6356c = count;
        }
    }
    else if (id == VALUE_COSTUME_CLEAR_ALL)
    {
        unsigned int costumes = DAT_10c64adc;
        for (int item = 0; item < 0x50; item++)
        {
            if (DAT_103150c0[item] == 0)
                continue;
            switch (item)
            {
            FOR_EACH_COSTUME_ITEM(COSTUME_CASE_CLEAR)
            default:
                continue;
            }
            DAT_103150c0[item] = 0;
        }
        DAT_103150c0[0x26] = 0;
        DAT_103150c0[0x27] = 0;
        DAT_103150c0[0x34] = 0;
        DAT_103150c0[0x35] = 0;
        DAT_10c64adc = costumes;
        FUN_1000e810();
    }
    else if (id == VALUE_SHOW_RESULT)
    {
        DAT_1020247c[DAT_104bfe88] = 0x41b;
        DAT_104bfe88 = DAT_104bfe88 + 1 & 7;
        int stage = DAT_10c64028 - 0x8d4;
        if (stage < 1)
            stage = 0;
        else if (stage > 0x3f)
            stage = 0x3f;
        FUN_10016580(DAT_10875e48[stage]);
        DAT_104bfe70 = 0;
        strcpy(DAT_1020da50[0], DAT_10874e48[stage]);
        strcpy(DAT_1020da50[1], "\x93|\x82\xb5\x82\xbd\x93G\x82\xcc\x90\x94");
        strcpy(DAT_1020da50[2], "\x95\xf3\x94\xa0\x82\xe2\x92\xd9\x82\xcc\x90\x94");
        strcpy(DAT_1020da50[4], "\x82\x66\x82\x81\x82\x8d\x82\x85\x82\x6e\x82\x96\x82\x85\x82\x92");
        strcpy(DAT_1020da50[5], "\x8e\x9e\x8a\xd4");
        strcpy(DAT_1020da50[6], "\x91\x8d\x8d\x87\x95\x5d\x89\xbf");
        DAT_1020da50[7][0] = 0;
        char text[0x40];
        Script_ValueCommand(40000, 0);
        int enemyTotal = DAT_10c63edc;
        int enemies = DAT_10c6356c;
        if (DAT_10c63edc < DAT_10c6356c)
            enemies = DAT_10c63edc;
        sprintf(text, "%d", enemies);
        int numberWidth = DAT_1020e538.TextWidth(text, 1.0f);
        sprintf(DAT_1020da50[1], "%ss%03d%ds210\x81^", DAT_1020da50[1], 0xcd - numberWidth, enemies);
        sprintf(text, "\x81^");
        numberWidth = DAT_1020e538.TextWidth(text, 1.0f);
        sprintf(DAT_1020da50[1], "%ss%03d%d", DAT_1020da50[1], numberWidth + 0xd7, enemyTotal);
        Script_ValueCommand(40000, 1);
        int chestTotal = DAT_10c63f3c;
        int chests = DAT_10c6356c;
        Script_ValueCommand(40000, 2);
        int jarTotal = DAT_10c63fcc;
        int jars = DAT_10c6356c;
        if (chestTotal < chests)
            chests = chestTotal;
        if (DAT_10c63fcc < jars)
            jars = DAT_10c63fcc;
        int found = jars + chests;
        sprintf(text, "%d", found);
        numberWidth = DAT_1020e538.TextWidth(text, 1.0f);
        sprintf(DAT_1020da50[2], "%ss%03d%ds210\x81^", DAT_1020da50[2], 0xcd - numberWidth, found);
        sprintf(text, "\x81^");
        numberWidth = DAT_1020e538.TextWidth(text, 1.0f);
        sprintf(DAT_1020da50[2], "%ss%03d%d", DAT_1020da50[2], numberWidth + 0xd7, jarTotal + chestTotal);
        int gameOvers = DAT_10c64000;
        sprintf(text, "%d", gameOvers);
        numberWidth = DAT_1020e538.TextWidth(text, 1.0f);
        int slashWidth = DAT_1020e538.TextWidth("\x81^", 1.0f);
        int slashCentre = (slashWidth >> 1) + 0xd2;
        sprintf(DAT_1020da50[4], "%ss%03d%s", DAT_1020da50[4], slashCentre - (numberWidth >> 1), text);
        int elapsed = (GetTickCount() + (DAT_10c6400c - (int)DAT_10c64004)) / 1000;
        int minutes = elapsed / 60;
        int seconds = elapsed % 60;
        if (minutes > 99)
        {
            minutes = 99;
            seconds = 0;
        }
        sprintf(text, "%02d", minutes);
        numberWidth = DAT_1020e538.TextWidth(text, 1.0f);
        int colonWidth = DAT_1020e538.TextWidth("\x81" "F", 1.0f);
        sprintf(DAT_1020da50[5], "%ss%03d%ss%03d\x81" "F", DAT_1020da50[5], 0xcf - numberWidth, text,
                slashCentre - (colonWidth >> 1));
        sprintf(text, "\x81^");
        numberWidth = DAT_1020e538.TextWidth(text, 1.0f);
        sprintf(DAT_1020da50[5], "%ss%03d%02d", DAT_1020da50[5], numberWidth + 0xd5, seconds);
        int rank = 0;
        if (enemies >= (int)((float)DAT_102169c0.enemies * 0.9f))
            rank = 1;
        int chestLimit = DAT_102169c0.chestsA + DAT_102169c0.chestsB;
        if (found >= (int)((float)chestLimit * 0.9f))
            rank++;
        if (gameOvers <= DAT_102169c0.deaths)
            rank++;
        if (found + enemies >= chestLimit + DAT_102169c0.enemies)
        {
            int attacks = DAT_10c6401c + DAT_10c64018;
            int weighted = DAT_10c6401c + DAT_10c64018 * 2;
            rank++;
            if (attacks == 0)
                attacks = 1;
            if (weighted == 0)
                weighted = 1;
            if (minutes < DAT_102169c0.time / 100 || (minutes == DAT_102169c0.time / 100 && seconds <= DAT_102169c0.time % 100))
                rank++;
            if (DAT_10c64008 <= DAT_10daaae8[0].stats[STAT_MAX_HEALTH] / 5)
                rank++;
            DAT_10c64024 = (DAT_10c6401c * 30 + attacks * 100) / attacks + 100;
            int hitPercent = DAT_10c64020 * 100 / weighted;
            if ((float)DAT_10c64024 / 100.0f * (float)hitPercent >= (float)DAT_102169c0.accuracy)
                rank++;
        }
        if (gameOvers != 0)
            rank = 0;
        switch (rank)
        {
        case 1:
            strcpy(text, "\x82\x61");
            break;
        case 2:
            strcpy(text, "\x82\x60");
            break;
        case 3:
            strcpy(text, "\x82\x60\x81\x7b");
            break;
        case 4:
            strcpy(text, "\x82\x72");
            break;
        case 5:
            strcpy(text, "\x82\x72\x81\x7b");
            break;
        case 6:
            strcpy(text, "\x82\x72\x81\x7b\x81\x7b");
            break;
        case 7:
            strcpy(text, "\x82\x72\x81\x7b\x81\x7b\x81\x7b");
            break;
        default:
            strcpy(text, "\x82\x62");
        }
        int rankWidth = (int)((float)DAT_1020e538.TextWidth(text, 1.2f) * 1.2f);
        sprintf(DAT_1020da50[6], "%ss%03d%s", DAT_1020da50[6], 0x9b - (rankWidth >> 1), text);
        int recordFlag = DAT_10c64028;
        DAT_10255960[0].kind = TEXT_WINDOW_CARD;
        DAT_10255960[0].progress = 0;
        DAT_10255960[0].m_0018 = 0;
        DAT_10255960[0].m_001c = value;
        DAT_10255960[0].m_0020 = 1;
        DAT_10c6356c = 0;
        DAT_104c0128 = 0;
        DAT_10255960[0].tile = 0;
        int rewardGiven = 0;
        if (DAT_10c64028 > 0 && DAT_10874848.TestScriptFlag(DAT_10c64028) < 2 && rank > 3)
        {
            DAT_10874848.SetScriptFlag(recordFlag, 3);
            if (DAT_10c64f8c == 0)
            {
                int item = 0x4f;
                int count = 1;
                FUN_10012130(&item, &count);
            }
            rewardGiven = 1;
            DAT_10255960[0].tile = 0x30;
            StartResultEffect(0x7c3, 0x5a);
        }
        int nextFlag = DAT_10c6402c + 1;
        int bestRank = DAT_10874848.TestScriptFlag(nextFlag) % 100;
        int bestEnemies = DAT_10874848.TestScriptFlag(DAT_10c6402c) / 100000;
        int bestJars = DAT_10874848.TestScriptFlag(DAT_10c6402c) % 1000;
        int bestChests = (DAT_10874848.TestScriptFlag(DAT_10c6402c) / 1000) % 100;
        bool record = false;
        if (DAT_10874848.TestScriptFlag(nextFlag) == 0 || bestRank < rank)
            record = true;
        else if (rank == bestRank)
        {
            int bestSeconds = (DAT_10874848.TestScriptFlag(nextFlag) / 1000000) * 60 +
                              (DAT_10874848.TestScriptFlag(nextFlag) / 10000) % 100;
            if (elapsed < bestSeconds || bestChests + bestJars + bestEnemies < found + enemies)
                record = true;
        }
        if (record)
        {
            int timeAndRank = ((minutes * 100 + seconds) * 100 + gameOvers) * 100 + rank;
            DAT_10874848.SetScriptFlag(DAT_10c6402c, (enemies * 100 + chests) * 1000 + jars);
            DAT_10874848.SetScriptFlag(nextFlag, timeAndRank);
            if (rewardGiven == 0)
            {
                if (rank < 4)
                {
                    if (rank >= 0)
                    {
                        if (rank < 2)
                        {
                            DAT_10255960[0].tile = 0x2b;
                            if (DAT_10c64f8c == 0)
                            {
                                int item = 0x4d;
                                int count = 1;
                                FUN_10012130(&item, &count);
                            }
                            StartResultEffect(0x759, 0x78);
                        }
                        else
                        {
                            DAT_10255960[0].tile = 0x31;
                            if (DAT_10c64f8c == 0)
                            {
                                int item = 0x4c;
                                int count = 1;
                                FUN_10012130(&item, &count);
                            }
                            StartResultEffect(0x7c4, 0x5a);
                        }
                    }
                }
                else
                    StartResultEffect(0x7c3, 0x5a);
            }
        }
        if (DAT_10c66d18 != 0)
            DAT_10c66d18 = 1;
        if (DAT_10c62ed0 != 306 || DAT_10c64768 != 0)
        {
            DAT_104c012c = 36;
            Bgm_Play(0x24, 0);
        }
    }
    else if (id == VALUE_CLAMP_63)
    {
        if (value < 0x3f)
            DAT_10c6356c = value < 1 ? 0 : value;
        else
            DAT_10c6356c = 0x3f;
    }
    else if (id == VALUE_GET_STATE_CODE)
    {
        if (DAT_10c64764 != 0)
            DAT_10c6356c = 6;
        else if (DAT_10c64680 != 0)
            DAT_10c6356c = 5;
        else if (DAT_10c65240 != 0)
            DAT_10c6356c = 4;
        else if (DAT_10c65238 != 0)
            DAT_10c6356c = 3;
        else if (DAT_10c65230 != 0)
            DAT_10c6356c = 2;
        else if (DAT_10c65294 != 0)
            DAT_10c6356c = 1;
        else
            DAT_10c6356c = 0;
    }
    else if (id == VALUE_CHAR_RESET_KIND)
    {
        DAT_1021f3c0[value] = 2;
        DAT_10daaae8[value].kind = -1;
    }
    else if (id == VALUE_CHAR_CLEAR_STATE)
    {
        // written out: the original clears the 16 words with 16 stores
        DAT_1020b430[value].words[0] = 0;
        DAT_1020b430[value].words[1] = 0;
        DAT_1020b430[value].words[2] = 0;
        DAT_1020b430[value].words[3] = 0;
        DAT_1020b430[value].words[4] = 0;
        DAT_1020b430[value].words[5] = 0;
        DAT_1020b430[value].words[6] = 0;
        DAT_1020b430[value].words[7] = 0;
        DAT_1020b430[value].words[8] = 0;
        DAT_1020b430[value].words[9] = 0;
        DAT_1020b430[value].words[10] = 0;
        DAT_1020b430[value].words[11] = 0;
        DAT_1020b430[value].words[12] = 0;
        DAT_1020b430[value].words[13] = 0;
        DAT_1020b430[value].words[14] = 0;
        DAT_1020b430[value].words[15] = 0;
    }
    else if (id == VALUE_CHAR_SET_KIND)
    {
        DAT_10daaae8[value].kind = DAT_10c6356c;
    }
    else if (id == VALUE_CHAR_CARRY_MOTION)
    {
        DAT_10db6ee8[value].PlayMotion(DAT_10c6356c, DAT_10c63570, DAT_10c63574, 1.0f);
        for (int slot = 92; slot < 124; slot++)
        {
            if (DAT_10daaae8[slot].kind == 0x2f)
            {
                CharState &state = DAT_1020b430[slot];
                for (int compartment = 0; compartment < 4; compartment++)
                {
                    if (state.words[compartment * 4] % 10000 == value && state.words[compartment * 4 + 3] > 0)
                        state.words[compartment * 4 + 3] += 10000;
                }
            }
        }
    }
    else if (id == VALUE_CHAR_GET_KIND)
    {
        DAT_10c6356c = DAT_10daaae8[value].kind;
    }
    else if (id == VALUE_CHAR_SET_SLOT_KIND)
    {
        DAT_1021f3c0[value] = (char)DAT_10c6356c;
    }
    else if (id == VALUE_CHAR_UPDATE)
    {
        FUN_100f9670(value);
    }
    else if (id == VALUE_CARRY_SET)
    {
        int heldSlot = -1;
        int heldCompartment = -1;
        int freeSlot = -1;
        int freeCompartment = -1;
        for (int slot = 92; slot < 124; slot++)
        {
            if (DAT_10daaae8[slot].kind == 0x2f)
            {
                for (int compartment = 0; compartment < 4; compartment++)
                {
                    if (DAT_1020b430[slot].words[compartment * 4 + 3] == 0 && freeSlot < 0)
                    {
                        freeCompartment = compartment;
                        freeSlot = slot;
                        break;
                    }
                    if (DAT_1020b430[slot].words[compartment * 4] == -1)
                    {
                        heldSlot = slot;
                        heldCompartment = compartment;
                        break;
                    }
                }
            }
            if (heldSlot >= 0)
                break;
        }
        if (value < 1)
        {
            if (heldSlot >= 0)
            {
                DAT_1020b430[heldSlot].words[heldCompartment * 4] = 0;
                DAT_1020b430[heldSlot].words[heldCompartment * 4 + 3] = 0;
            }
            FUN_100165e0(0);
        }
        else
        {
            int slot;
            int compartment;
            if (heldSlot < 0)
            {
                if (freeSlot < 0)
                    return 0;
                slot = freeSlot;
                compartment = freeCompartment;
            }
            else
            {
                slot = heldSlot;
                compartment = heldCompartment;
            }
            DAT_1020b430[slot].words[compartment * 4] = -1;
            DAT_1020b430[slot].words[compartment * 4 + 1] = value;
            DAT_1020b430[slot].words[compartment * 4 + 3] = 1;
            FUN_100165e0(1);
        }
    }
    else if (id == VALUE_ROUTE_SET_KIND)
    {
        for (int slot = 58; slot < 92; slot++)
        {
            if (DAT_10daaae8[slot].routeRecord == value)
                DAT_10daaae8[slot].kind = DAT_10c6356c;
        }
    }
    else if (id == VALUE_CHAR_SET_TABLE_9)
    {
        DAT_10254090[value] = (char)DAT_10c6356c;
    }
    else if (id == VALUE_CHAR_SET_VARIANT)
    {
        DAT_1020b230[value] = DAT_10c6356c;
    }
    else if (id == VALUE_CHAR_PLACE_SHADOW)
    {
        if (DAT_102022b8 != 0)
        {
            Vec4 start;
            Vec4 hit;
            Vec4 end = DAT_10db6ee8[value].matrix.GetTranslation();
            start.Copy(end);
            end.z += 10.0f;
            Vec4 light = *DAT_10804bf8.LightDirection(&DAT_10db6ee8[value], &DAT_118e69e8, &DAT_118e6cb8);
            light.Normalized3();
            start.Copy(start - light * 1000.0f);
            if (DAT_10255f08 != 0 && DAT_10255f08->CastSegment2(0, 0x4000000, &start, &end, &hit, 0) != 0)
            {
                hit.x = start.x;
                hit.y = start.y;
            }
            else
            {
                hit.x = start.x;
                hit.y = start.y;
                hit.z = -3500.0f;
            }
            DAT_10db6ee8[value].shadowMatrix.SetTranslation(hit);
        }
    }
    else if (id == VALUE_CHAR_SET_MARK)
    {
        DAT_10db6ee8[value].m_0012 = (char)DAT_10c6356c;
    }
    else if (id == VALUE_SET_INVULNERABLE)
    {
        DAT_104c00b8 = value;
    }
    else if (id == VALUE_PANEL_CHECKSUM)
    {
        DAT_10c6356c = 0;
        for (int index = 0; index < (int)strlen(DAT_1020da50[0]); index++)
            DAT_10c6356c += (char)DAT_1020da50[0][index];
        for (int index = 0; index < (int)strlen(DAT_1020da50[0]); index++)
            DAT_10c6356c += (char)DAT_1020da50[0][index];
        if (value == 1 && FUN_1013f880() % 3 == 0)
            DAT_10c6356c = DAT_10874848.TestScriptFlag(FUN_1013f880() % 10 + 0x1a4);
    }
    else if (id == VALUE_MESSAGE_SHOW)
    {
        FUN_1000ed70(DAT_1020da50[0], DAT_1020da50[1], value);
    }
    else if (id == VALUE_GET_STOCK)
    {
        DAT_10c63594 = DAT_103150c0[value];
    }
    else if (id == VALUE_REMOVE_ITEM_100)
    {
        int item = value;
        int count = 100;
        FUN_100120a0(&item, &count);
    }
    else if (id == VALUE_NOP_4E23)
    {
    }
    else if (id == VALUE_CHECK_COLLECTION)
    {
        DAT_10c647d8 = 1;
        for (int flag = 0x672; flag <= 0x68e; flag++)
        {
            if (flag != 0x673)
            {
                if (DAT_103150c0[flag - 0x640] == 0)
                {
                    if (DAT_10874848.TestScriptFlag(flag) == 0)
                        DAT_10c647d8 = 0;
                }
                else
                    DAT_10874848.SetScriptFlag(flag, 1);
            }
        }
        if (DAT_103150c0[0x27] == 0)
        {
            if (DAT_10c64878 == 0)
                DAT_10c647d8 = 0;
        }
        else
            DAT_10c64878 = 1;
    }
    else if (id == VALUE_REMOVE_ITEM)
    {
        int item = value;
        int count = DAT_10c6356c;
        FUN_100120a0(&item, &count);
    }
    else if (id == VALUE_GET_HELMET)
    {
        DAT_10c63594 = DAT_10daaae8[0].helmet;
    }
    else if (id == VALUE_CLEAR_FLAGS)
    {
        for (int flag = 0; flag < 4000; flag++)
        {
            if (flag != 0x1c7 && flag != 0x9c5 && flag != 0xf75)
                DAT_10874848.SetScriptFlag(flag, 0);
        }
        FUN_1000e750();
        FUN_1000e810();
    }
    else if (id == VALUE_GET_MAP_OR_MIRROR)
    {
        if (value == 0)
            DAT_10c6356c = DAT_10c62ed0;
        else if (value == 1)
            DAT_10db6ee8[0].matrix.translation.x = DAT_10db6ee8[0].matrix.translation.x * -1.0f;
    }
    else if (id == VALUE_NOP_4E2B || id == VALUE_NOP_4E2C)
    {
    }
    else if (id == VALUE_SET_HEALTH)
    {
        DAT_10daaae8[0].stats[STAT_HEALTH] = value;
        FUN_1000e750();
    }
    else if (id == VALUE_NEAREST_ZONE)
    {
        int zoneFlag;
        if (value == 13)
            zoneFlag = 0xf96;
        else if (value == 15)
            zoneFlag = 0xf93;
        else if (value == 58)
            zoneFlag = 0xf90;
        else
        {
            DAT_10c63594 = 0;
            return 0;
        }
        int zone = DAT_10874848.TestScriptFlag(zoneFlag);
        if (DAT_10c62ed0 == DAT_101d6330[zone])
        {
            int nextFlag = zoneFlag + 1;
            if (DAT_10c62ed0 != DAT_101d6330[DAT_10874848.TestScriptFlag(nextFlag)])
            {
                DAT_10c63594 = DAT_10874848.TestScriptFlag(zoneFlag);
                return 0;
            }
            pointA.Fill(0.0f);
            pointA.x = (float)DAT_10216dc0[DAT_101d6310[DAT_10874848.TestScriptFlag(zoneFlag)]].x;
            pointA.y = (float)DAT_10216dc0[DAT_101d6310[DAT_10874848.TestScriptFlag(zoneFlag)]].y;
            pointB.Fill(0.0f);
            pointB.x = (float)DAT_10216dc0[DAT_101d6310[DAT_10874848.TestScriptFlag(nextFlag)]].x;
            pointB.y = (float)DAT_10216dc0[DAT_101d6310[DAT_10874848.TestScriptFlag(nextFlag)]].y;
            Matrix4 &roamer = DAT_10db6ee8[value].matrix;
            float distanceA = (pointA - roamer.GetTranslation()).Length();
            float distanceB = (pointB - roamer.GetTranslation()).Length();
            if (distanceB < distanceA)
            {
                DAT_10c63594 = DAT_10874848.TestScriptFlag(nextFlag);
                return 0;
            }
        }
        else
            zoneFlag++;
        DAT_10c63594 = DAT_10874848.TestScriptFlag(zoneFlag);
    }
    else if (id == VALUE_SET_SPEAKER_NODE_ID)
    {
        if (value == 0)
            DAT_10c63580 = 0x3a;
        else if (value == 1)
            DAT_10c63580 = 0x5c;
    }
    else if (id == VALUE_FACE_CAMERA_AXIS)
    {
        if (DAT_104bfd0c != 0)
            DAT_10db6ee8[DAT_104c0024].targetHeading =
                FUN_10001f80((float)DAT_102559f0, (float)DAT_1042d9e4) + PI_APPROX;
    }
    else if (id == VALUE_GAME_OVER)
    {
        FUN_1003b670(value);
    }
    else if (id == VALUE_PLAYER_FACE_POINT)
    {
        Vec4 pointB;
        Vec4 pointA;
        DAT_10c63590 = (int)(DAT_10db6ee8[0].heading * 1000.0f);
        DAT_104c00d0 = 0x41f;
        pointB.Copy(DAT_10db6ee8[0].matrix.GetTranslation());
        pointA.Copy(DAT_118e69e8);
        DAT_10db6ee8[0].targetHeading = -HeadingBetween(pointB, pointA);
        DAT_10db6ee8[0].heading = DAT_10db6ee8[0].targetHeading;
        DAT_10db6ee8[0].PlayMotion(0x58, 1, 5, 1.0f);
    }
    else if (id == VALUE_PLAYER_TURN_RESTORE)
    {
        DAT_10db6ee8[0].targetHeading = (float)DAT_10c63590 / 1000.0f;
    }
    else if (id == VALUE_PLAYER_TURN_RELATIVE)
    {
        Vec4 pointB;
        Vec4 pointA;
        pointB.Copy(DAT_10db6ee8[0].matrix.GetTranslation());
        pointA.Copy(DAT_118e69e8);
        DAT_10db6ee8[0].targetHeading = (float)value * PI_APPROX / DEGREES_HALF_TURN - HeadingBetween(pointB, pointA);
        DAT_10db6ee8[0].heading = DAT_10db6ee8[0].targetHeading;
    }
    else if (id == VALUE_PLAYER_MOTION)
    {
        DAT_10db6ee8[0].PlayMotion(value, 0, 5, 1.0f);
    }
    else if (id == VALUE_PLAYER_ATTACK)
    {
        FUN_1005ce80(value, 0, 0, 0);
    }
    else if (id == VALUE_GREETINGS)
    {
        for (int index = 0; index < 4; index++)
            DAT_104c0158[index] = 0x14;
        DAT_10c6356c = DAT_104c00e0;
        DAT_10c635cc = 1;
    }
    else if (id == VALUE_CHAR_GET_SWITCHES)
    {
        DAT_10c6356c = (unsigned char)DAT_10daaae8[value].switchesHeld;
    }
    else if (id == VALUE_FIND_MARKED_SLOT)
    {
        int count = 0;
        for (int slot = 0x5c; slot < 0x7c; slot++)
        {
            if (DAT_1021f3c0[slot] == '`')
            {
                if (count == value)
                {
                    DAT_10c63570 = slot;
                    break;
                }
                count++;
            }
        }
    }
    else if (id == VALUE_RAISE_MAX_HEALTH)
    {
        if (value == 0)
            DAT_10daaae8[0].stats[STAT_MAX_HEALTH] += 10;
        FUN_1000e750();
    }
    else if (id == VALUE_NOP_4E41)
    {
    }
    else if (id == VALUE_PAY_ITEM)
    {
        int price = DiscountedPrice(DAT_1088e6b0[value].price);
        DAT_10daaae8[0].coins -= DAT_10c66cfc * price / 100;
        FUN_1000e750();
    }
    else if (id == VALUE_GET_PRICE)
    {
        DAT_10c6356c = DAT_10c66cfc * DiscountedPrice(value) / 100;
    }
    else if (id == VALUE_KAGU_ITEM_PLACE)
    {
        if (DAT_10255f08 != 0)
        {
            Node *stand = DAT_10255f08->Find("@kagudai", 0);
            if (stand != 0)
            {
                pointA.Copy(stand->parent->matrix.GetTranslation());
                Node *origin = stand->Find("@kagunemoto", 0);
                if (value > 0 && origin != 0)
                {
                    Node *furniture = origin->AddChild(0, 0x40);
                    if (DAT_10c6356c == 0)
                        furniture->Clone(&DAT_104d2a08[value], 0, 0);
                    else
                        furniture->Load(FUN_1013f8a0(DAT_10894698[value], 0), 0, 0, 0);
                    furniture->BuildMesh(1, 0);
                    strcpy(furniture->name, "itemkagu");
                    furniture->matrix = TranslationMatrix(0.0f, 0.0f, 25.0f);
                }
                EffectHold &hold = DAT_1020dcf8[DAT_1020b22c];
                hold.effect = 0x75f;
                hold.position[0] = (int)pointA.x;
                hold.position[1] = (int)pointA.y;
                hold.position[2] = (int)pointA.z + 0xa5;
                hold.m_0010 = 0x40;
                hold.age = 0x3f;
                DAT_1020b22c++;
                hold.value = 0;
                hold.m_001c = 0;
            }
        }
    }
    else if (id == VALUE_KAGU_STAND_HIDE)
    {
        if (DAT_10255f08 != 0)
        {
            Node *origin = DAT_10255f08->Find("@kagunemoto", 0);
            if (origin != 0)
            {
                origin->parent->SetFlags(NODE_FLAG_HIDDEN);
                origin->parent->ClearFlags(4);
            }
            Node *hit = DAT_10255f08->Find("@kaguatari", 0);
            if (hit != 0)
                hit->ClearFlags(4);
        }
    }
    else if (id == VALUE_PLAYER_ATTACK_NEAR)
    {
        if (DAT_102024ac < 0)
        {
            int nearest = 0;
            int nearestDistance = 0;
            for (int slot = 58; slot < 124; slot++)
            {
                if (nearest == 0 ||
                    (DAT_10db6ee8[0].matrix.GetTranslation() - DAT_10db6ee8[slot].matrix.GetTranslation())
                            .LengthSquared3() < (float)nearestDistance)
                {
                    nearestDistance = (int)(DAT_10db6ee8[0].matrix.GetTranslation() -
                                            DAT_10db6ee8[slot].matrix.GetTranslation())
                                               .LengthSquared3();
                    nearest = slot;
                }
            }
            DAT_102024ac = nearest;
        }
        DAT_1049ef8c = 1;
        DAT_104bfe20 = 1;
        FUN_1005ce80(value, 0, 0, 0);
    }
    else if (id == VALUE_CHAR_FACE_POINT)
    {
        Vec4 pointB;
        Vec4 pointA;
        pointB.Copy(DAT_10db6ee8[value].matrix.GetTranslation());
        pointA.Copy(DAT_118e69e8);
        DAT_10db6ee8[value].targetHeading = -HeadingBetween(pointB, pointA);
        DAT_10db6ee8[value].heading = DAT_10db6ee8[value].targetHeading;
    }
    else if (id == VALUE_REMOVE_EFFECT_HOLD)
    {
        for (int index = 0; index < DAT_1020b22c; index++)
        {
            if (DAT_1020dcf8[index].effect == value)
                DAT_1020dcf8[index].position[0] = -200000;
        }
    }
    else if (id == VALUE_CLEAR_104C0110)
    {
        DAT_104c0110 = 0;
    }
    else if (id == VALUE_PLAYER_TURN_TO_REGISTERS)
    {
        Vec4 pointB;
        Vec4 pointA;
        pointB.Copy(DAT_10db6ee8[0].matrix.GetTranslation());
        pointA.x = (float)DAT_10c6356c;
        pointA.y = (float)DAT_10c63570;
        DAT_10db6ee8[0].targetHeading = (float)value * PI_APPROX / DEGREES_HALF_TURN - HeadingBetween(pointB, pointA);
        DAT_10db6ee8[0].heading = DAT_10db6ee8[0].targetHeading;
    }
    else if (id == VALUE_AND_REGISTER)
    {
        DAT_10c6356c = DAT_10c6356c & value;
    }
    else if (id == VALUE_SET_STATE_WORD)
    {
        DAT_1020b430[DAT_10c6356c].words[DAT_10c63570] = value;
    }
    else if (id == VALUE_SWITCH_HOLD)
    {
        Node *root = DAT_10db6ee8[value].root;
        Node *element = root->Find("@Thunder", 0);
        if (element == 0)
            element = root->Find("@Fire", 0);
        FUN_10057c40(value, element, DAT_10c6356c);
    }
    else if (id == VALUE_KNOCK_BACK)
    {
        FUN_10055ee0();
    }
    else if (id == VALUE_WATER_FREEZE)
    {
        DAT_10c6356c = (unsigned char)DAT_104c02dc;
        if (DAT_104c007c != 0 && DAT_104c0080 != 0)
        {
            if (DAT_104c02dc == 0)
            {
                if (value == 1)
                {
                    FUN_1000db30(0, 100, 0);
                    FUN_10011fb0(1);
                    FUN_1000db30(0, 0, 0xf);
                }
            }
            else if (value == 0)
            {
                FUN_1000dab0(0, 100, 0, 0xff5050);
                FUN_10011fb0(0);
                FUN_1000dab0(0, 0, 0xf, 0xff5050);
            }
        }
    }
    else if (id == VALUE_GET_STATE_WORD)
    {
        DAT_10c6356c = DAT_1020b430[value].words[DAT_10c63570];
    }
    else if (id == VALUE_SET_SCALE_FROM_REGISTERS)
    {
        DAT_10db6ee8[value].scale.Set((float)DAT_10c6356c / 100.0f, (float)DAT_10c63570 / 100.0f,
                                      (float)DAT_10c63574 / 100.0f, 0.0f);
    }
    else if (id == VALUE_SET_SCALE_SMALL)
    {
        DAT_10db6ee8[93].scale.Set(0.3f, 0.3f, 0.3f, 0.0f);
    }
    else if (id == VALUE_STAMINA_UP)
    {
        DAT_10c64ad8 += 100;
        FUN_1000e750();
    }
    else if (id == VALUE_OR_REGISTER)
    {
        DAT_10c6356c = DAT_10c6356c | value;
    }
    else if (id == VALUE_GET_104BFD0C)
    {
        DAT_10c6356c = DAT_104bfd0c;
    }
    else if (id == VALUE_SET_VECTOR)
    {
        DAT_1083631c.x = (float)DAT_10c6356c;
        DAT_1083631c.y = (float)DAT_10c63570;
        DAT_1083631c.z = (float)DAT_10c63574;
        DAT_1083631c.w = (float)DAT_10c63578;
    }
    else if (id == VALUE_NEAREST_CHAR)
    {
        int nearest = 0;
        float nearestDistance = 300.0f;
        Vec4 pointA;
        Vec4 pointB;
        pointA.Copy(DAT_10db6ee8[0].matrix.GetTranslation());
        for (int slot = 1; slot < 92; slot++)
        {
            pointB.Copy(DAT_10db6ee8[slot].matrix.GetTranslation());
            if ((pointA - pointB).Length() < nearestDistance)
            {
                nearestDistance = (pointA - pointB).Length();
                nearest = slot;
            }
        }
        if (nearest == 0)
            DAT_10c6356c = 0;
        else
            DAT_10c6356c = nearest == value ? 0 : nearest;
    }
    else if (id == VALUE_CHECK_SAVE_ALLOWED)
    {
        DAT_10c6356c = 0;
        int map = DAT_10c62ed0;
        bool checkSave;
        if (map == 0x134)
        {
            if (DAT_10c655e0 != 0 && -50000.0f < DAT_10db6ee8[25].matrix.translation.x)
                DAT_10c6356c = 1;
            checkSave = true;
        }
        else if (map > 0x134)
        {
            if (map == 401)
            {
                DAT_10c6356c = 1;
                checkSave = true;
            }
            else
                checkSave = map >= 100;
        }
        else if (map >= 21 && map <= 27)
        {
            DAT_10c6356c = 1;
            checkSave = true;
        }
        else
            checkSave = map >= 100;
        if (!checkSave || DAT_10c646e4 != 0)
            DAT_10c6356c = value == 0;
    }
    else if (id == VALUE_SET_OVERLAY_FLAG)
    {
        DAT_104bfe1c = value;
    }
    else if (id == VALUE_STAMINA_DOWN)
    {
        DAT_10c64ad8 -= 100;
        FUN_1000e750();
    }
    else if (id == VALUE_GET_PICKUPS)
    {
        DAT_10c6356c = DAT_10daaae8[0].stats[STAT_PICKUPS];
    }
    else if (id == VALUE_SWITCH_RELEASE)
    {
        if (value == -1)
        {
            int count = DAT_1021b3c0;
            for (int object = 0; object < count; object++)
                FUN_10057b90(DAT_10254838[object] % 1000);
        }
        else
            FUN_10057b90(DAT_10254838[value] % 1000);
    }
    else if (id == VALUE_SET_104C0110)
    {
        DAT_104c0110 = value;
    }
    else if (id == VALUE_COPY_STATE_WORD)
    {
        DAT_10c63574 = DAT_1020b430[DAT_10c6356c].words[DAT_10c63570];
    }
    else if (id == VALUE_KAGU_SWAP_NODES)
    {
        if (DAT_10255f08 != 0)
        {
            Node *stand = DAT_10255f08->Find("@itemkagu", 0);
            Node *shown = 0;
            if (stand != 0)
                shown = stand->FindByFlags(NODE_FLAG_HIDDEN, 0, 0);
            Node *kagudai = DAT_10255f08->Find("@kagudai", 0);
            if (kagudai != 0)
            {
                kagudai->parent->SetFlags(NODE_FLAG_HIDDEN);
                Node *item = kagudai->parent->Find("@itemkagu", 0);
                if (item != 0)
                    item->ClearFlags(NODE_FLAG_HIDDEN);
            }
            if (shown != 0)
                shown->SetFlags(NODE_FLAG_HIDDEN);
        }
    }
    else if (id == VALUE_GET_OBJECT_FIELD)
    {
        DAT_10c6356c = DAT_1090a700[value][0];
    }
    else if (id == VALUE_SET_104C012C)
    {
        DAT_104c012c = value;
    }
    else if (id == VALUE_NEGATE_104C012C)
    {
        DAT_104c012c = -DAT_104c012c;
    }
    else if (id == VALUE_GET_RESTRICTED)
    {
        DAT_10c6356c = DAT_10db6ee8[0].m_238c;
    }
    else if (id == VALUE_PLAYER_SHOUT)
    {
        if (DAT_104c01b0 == 0)
            FUN_10001b70((100 - (DAT_102027d8 * 5 + 0x32) / 2) * 100);
        DAT_104c0198 = value;
        DAT_104c00d0 = 0x816;
        DAT_10db6ee8[0].PlayMotion(0x59, 1, 5, 1.0f);
    }
    else if (id == VALUE_SET_RECORD_FIELD)
    {
        int record = DAT_10c6356c < 1 ? 0 : (DAT_10c6356c < 0x7f ? DAT_10c6356c : 0x7f);
        int row = DAT_10c63570 < 1 ? 0 : (DAT_10c63570 < 0xf ? DAT_10c63570 : 0xf);
        int column = DAT_10c63574 < 1 ? 0 : (DAT_10c63574 < 3 ? DAT_10c63574 : 3);
        RoutePoint &point = DAT_10210fc0[record].points[row];
        switch (column)
        {
        case 0:
            point.x = value;
            break;
        case 1:
            point.y = value;
            break;
        case 2:
            point.z = value;
            break;
        default:
            point.code = value;
        }
    }
    else if (id == VALUE_SET_COSTUME)
    {
        DAT_10c655f0 = value;
        FUN_1000e750();
    }
    else if (id == VALUE_SET_104C635F8)
    {
        DAT_10c635f8 = value;
        FUN_1000e750();
    }
    else if (id == VALUE_PUSH_AWAY)
    {
        Vec4 pointB;
        pointB.x = (float)DAT_10c6356c;
        pointB.y = (float)DAT_10c63570;
        pointB.z = (float)DAT_10c63574;
        FUN_10011b70(0x18, 5, 0, &pointB);
    }
    else if (id == VALUE_CLEAR_POST_EFFECT)
    {
        DAT_10f654f8 = 0;
        DAT_10f654fc = 0;
    }
    return 0;
}
