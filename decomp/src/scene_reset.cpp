// Resetting the scene when a map is entered (run by the field and event modes 0x100fc4a0/0x100fda40): the per-character
// tables of every slot are cleared, the models of some maps are loaded, the characters of the map are given their
// first state, the beam links of the "O_067" objects are filled in and a few fixed character kinds are set.

#include <math.h>
#include <string.h>
#include <windows.h>

#include "char_slot.h"

#include "char_state.h"

#include "vec4.h"

#include "matrix4.h"

#include "node.h"

#include "anim_model.h"

#include "last_position.h"

#include "texture_load.h"

#include "char_links.h"
#include "char_tables.h"
#include "char_vars.h"
#include "game_vars.h"
#include "map_vars.h"
#include "render_vars.h"
#include "ui_vars.h"
#include "sound_vars.h"
#include "mole_game.h"
#include "engine_memory.h"
#include "scene_reset.h"

#include "value_command.h"
#include "char_equip_models.h"
extern "C" int FUN_1001b5b0(int index);
extern "C" int FUN_1016b230(const char *text);
#include "char_update.h"

#define PI_APPROX 3.1415f                // pi as the game writes it

// Script flags read or set here (DAT_10c62edc).
#define FLAG_ITEM_1 0x7d4                // the nine items that complete costume 6
#define FLAG_ITEM_2 0x7db
#define FLAG_ITEM_3 0x7e0
#define FLAG_ITEM_4 0x7e3
#define FLAG_ITEM_5 0x7e5
#define FLAG_ITEM_6 0x7eb
#define FLAG_ITEM_7 0x7ed
#define FLAG_ITEM_8 0x7ef
#define FLAG_ITEM_9 0x7f1
#define FLAG_COSTUME_6_READY 0x809
#define FLAG_MODE_2_MESSAGES 0xf71       // difficulty 2 with the message words kept
#define FLAG_MODE_OTHER 0xf72            // any difficulty but 1 and 2
#define FLAG_MODE_1 0xf73
#define FLAG_TIMER_CLOCK 0x44a           // GetTickCount() at the start of the scene
#define FLAG_MAP_STAMP 0x449
#define FLAG_MAP_STAMP_SEEN 0x44d
#define FLAG_LAST_MAP 0x447
#define FLAG_VISIT_COUNT 0x448
#define FLAG_ROUTE_TIMER 0x864
#define FLAG_NIGHT_SLOT 0x9f8
#define FLAG_ELEMENT_A 0x8d4
#define FLAG_ELEMENT_B 0x200
#define FLAG_OPEN_ALL 0x7ea

// Costume unlock bits that must all be set for costume 6 (the same as POPON_UNLOCK_BITS in costume_load.cpp).
#define COSTUME_UNLOCK_A 1
#define COSTUME_UNLOCK_B 2
#define COSTUME_UNLOCK_C 4
#define COSTUME_UNLOCK_D 0x10

#define COSTUME_POPON 6
#define FIRST_MAP_SLOT 0x3a              // character slots 58..91 are the map's characters, 92..123 its objects
#define FIRST_OBJECT_SLOT 0x5c
#define END_OBJECT_SLOT 0x7c
#define PENALTY_STAT 4                   // CharSlot::stats entry lowered for costume 6
#define PENALTY_AMOUNT 0x4b
#define PENALTY_KIND 0x50                // CharSlot::kind that takes the penalty
#define SCRIPT_CODE_STAT 9               // CharSlot::stats entry that holds the packed script code of a map character
#define KEPT_SLOT_KIND 'M'               // DAT_1021f3c0 value of a slot whose state words 10 and 11 are kept
#define STATE_WORD_COUNT 0x10
#define KEPT_WORD_A 10
#define KEPT_WORD_B 11
#define NODE_FLAGS_HIDE 0x20
#define NODE_VALUE_HIDE 0x40
#define NODE_MASK_BODY 0x400
#define NODE_MASK_SHADOW 0x200
#define NODE_FLAG_WORDS 10               // words of DAT_1049efb0 cleared (a quarter of the grid)

// Maps.
#define MAP_WATER_TEMPLE 0xce            // hides the water nodes "mizuT1" and "mizuT2" on low effect detail
#define MAP_TOKATYU 0x133                // loads "tokatyu.it3" as the held object
#define MAP_MOSSROOT 0x1f9               // loads "mossroot.it3" as the held object
#define MAP_FACE_PLAYER 0x191            // every character turns towards the player
#define MAP_FIRST_LATE 100
#define OBJECT_LINK_NAME "O_067"
#define OBJECT_SCRIPT_TOTAL 999

// Slot kinds set here.
#define KIND_FOLLOWER 0x16
#define KIND_GUARD 0x3c
#define KIND_WATCHER 0x17
#define KIND_LAST_FIXED 0x57
#define KIND_ARMOUR 0x10
#define KIND_BOSS 0x31
#define KIND_SHADOW_OBJECT '`'
#define SLOT_STATE_ENEMY 3               // DAT_1021f3fa value of a character that gets its first state here

// Heights of the three beam points above an "O_067" object.
#define BEAM_POINT_HEIGHT_1 75.0f
#define BEAM_POINT_HEIGHT_2 150.0f
#define BEAM_POINT_HEIGHT_3 225.0f

// 0x100fb7b0 (Scene_Reset). Clears the per-map state: the carried object, costume 6's energy penalty, the costume 6
// flag, the difficulty flags, the node and matrix tables, the per-character tables of all 0x80 slots, then turns the
// characters of map 0x191 towards the player, loads the held-object model of maps 0x133 and 0x1f9, hides the water
// nodes of map 0xce and the shadow-map nodes, loads the count-down texture, gives map characters 58..91 their first
// state (FUN_100404e0 with the digit pairs of their script code), fills the link table DAT_1049f0c8 with the pairs
// of "O_067" objects and their three beam points, sets the kinds of the fixed slots and hides the body of the
// kind '`' objects of map 0x191. Returns 0.
extern "C" int FUN_100fb7b0(void)
{
    DAT_104bfdd8 = 0;
    DAT_104c0388 = 0;
    DAT_104c0384 = 0;
    if (DAT_10c655f0 == COSTUME_POPON)
    {
        for (int slot = FIRST_MAP_SLOT; slot < FIRST_OBJECT_SLOT; slot += 2)
        {
            if (DAT_10daaae8[slot].kind == PENALTY_KIND)
                DAT_10daaae8[slot].stats[PENALTY_STAT] -= PENALTY_AMOUNT;
            if (DAT_10daaae8[slot + 1].kind == PENALTY_KIND)
                DAT_10daaae8[slot + 1].stats[PENALTY_STAT] -= PENALTY_AMOUNT;
        }
    }
    unsigned char unlocked = DAT_104c0230;
    if ((unlocked & COSTUME_UNLOCK_A) && (unlocked & COSTUME_UNLOCK_B) && (unlocked & COSTUME_UNLOCK_C) &&
        (unlocked & COSTUME_UNLOCK_D) && DAT_10c62edc[FLAG_ITEM_1] != 0 && DAT_10c62edc[FLAG_ITEM_2] != 0 &&
        DAT_10c62edc[FLAG_ITEM_3] != 0 && DAT_10c62edc[FLAG_ITEM_4] != 0 && DAT_10c62edc[FLAG_ITEM_5] != 0 &&
        DAT_10c62edc[FLAG_ITEM_6] != 0 && DAT_10c62edc[FLAG_ITEM_7] != 0 && DAT_10c62edc[FLAG_ITEM_8] != 0 &&
        DAT_10c62edc[FLAG_ITEM_9] != 0)
        DAT_10c62edc[FLAG_COSTUME_6_READY] = 1;
    DAT_10c62edc[FLAG_MODE_2_MESSAGES] = 0;
    DAT_10c62edc[FLAG_MODE_OTHER] = 0;
    DAT_10c62edc[FLAG_MODE_1] = 0;
    if (DAT_10c635f8 == 1)
        DAT_10c62edc[FLAG_MODE_1] = 1;
    else if (DAT_10c635f8 == 2)
    {
        if (DAT_10c66cc0 == 0)
            DAT_10c62edc[FLAG_MODE_2_MESSAGES] = 1;
    }
    else
        DAT_10c62edc[FLAG_MODE_OTHER] = 1;
    memset(DAT_10254ac0, 0, sizeof(Node *) * 0x60);
    DAT_104c02c5 = 0;
    DAT_104c03b0 = 0;
    memset(DAT_1021a1e0, 0, sizeof(Node *) * 0x60);
    DAT_10c62edc[FLAG_TIMER_CLOCK] = GetTickCount();
    memset(DAT_103086b0, 0, sizeof(DAT_103086b0));
    memset(DAT_1030b8f8, 0, sizeof(DAT_1030b8f8));
    memset(DAT_104483e8, 0, sizeof(DAT_104483e8));
    memset(DAT_10254598, 0, sizeof(DAT_10254598));
    memset(DAT_10255160, 0, sizeof(DAT_10255160));
    DAT_10c62edc[FLAG_ROUTE_TIMER] = 0;
    DAT_104c0380 = 0;
    int slot = 0;
    do
    {
        for (int matrix = 0; matrix < 6; matrix++)
            DAT_10868838[slot][matrix].SetIdentity();
        DAT_104c4438[slot].Fill(0);
        memset(DAT_1021bdf8[slot], 0, sizeof(DAT_1021bdf8[slot]));
        DAT_10453008[slot] = 0;
        DAT_1020e138[slot] = -1;
        DAT_10253a58[slot] = 0;
        DAT_1042dba8[slot] = 0;
        DAT_1020d540[slot] = 0;
        DAT_10867e18[slot].Fill(0);
        char kind = DAT_1021f3c0[slot];
        DAT_1021b4c8[slot] = 0;
        DAT_10256420[slot] = 0;
        DAT_10254390[slot] = 0;
        DAT_1021f940[slot] = 0;
        DAT_1020b230[slot] = 0;
        for (int word = 0; word < STATE_WORD_COUNT; word++)
        {
            if (kind != KEPT_SLOT_KIND || (word != KEPT_WORD_A && word != KEPT_WORD_B))
                DAT_1020b430[slot].words[word] = 0;
        }
        for (int history = 0; history < 2; history++)
            (&DAT_10802bf8[slot].position)[history].Fill(0);
        DAT_10864cf8[slot].Fill(0);
        DAT_10467920[slot] = 0;
        slot++;
    } while (slot < CHAR_SLOT_COUNT);
    for (int point = 0; point < 8; point++)
        DAT_10edbd58[point].Fill(0);
    for (int attached = 0; attached < 6; attached++)
    {
        DAT_10f08880[attached].SetIdentity();
        DAT_102108d0[attached] = 0;
    }
    memset(DAT_10447310, 0, sizeof(DAT_10447310));
    memset(DAT_1021a398, 0, sizeof(DAT_1021a398));
    memset(DAT_1021bbf8, 0, sizeof(DAT_1021bbf8));
    memset(DAT_1042dea8, 0, sizeof(DAT_1042dea8));
    memset(DAT_1021b728, 0, sizeof(DAT_1021b728));
    memset(DAT_10255f10, 0, sizeof(DAT_10255f10));
    memset(DAT_102199b8, 0, sizeof(DAT_102199b8));
    if (DAT_10c62ed0 == MAP_FACE_PLAYER)
    {
        Script_ValueCommand(0x27d9, 0);
        Vec4 player;
        Vec4 other;
        player.Copy(DAT_10db6ee8[0].matrix.GetTranslation());
        for (int slot = 1; slot < FIRST_MAP_SLOT; slot++)
        {
            if (DAT_10db6ee8[slot].root != 0 && DAT_10db6ee8[slot].matrix.translation.x > -20000.0f)
            {
                other.Copy(DAT_10db6ee8[slot].matrix.GetTranslation());
                float dy = player.y - other.y;
                float dx = player.x - other.x;
                float angle = (float)atan2((double)dx, (double)dy) + PI_APPROX;
                DAT_10db6ee8[slot].heading = angle;
                DAT_10db6ee8[slot].targetHeading = angle;
            }
        }
    }
    if (DAT_10c62edc[FLAG_MAP_STAMP] != DAT_10c62edc[FLAG_MAP_STAMP_SEEN])
    {
        if (DAT_10c62edc[FLAG_LAST_MAP] == DAT_10c62ed0)
            DAT_10c62edc[FLAG_VISIT_COUNT] -= 2;
        DAT_10c62edc[FLAG_MAP_STAMP_SEEN] = DAT_10c62edc[FLAG_MAP_STAMP];
    }
    DAT_10c62edc[FLAG_LAST_MAP] = 0;
    switch (DAT_10c62ed0)
    {
    case MAP_MOSSROOT:
        if (DAT_104c0390 != 0)
        {
            DAT_104c0390->Release();
            DAT_11b1ba40(DAT_104c0390);
            DAT_104c0390 = 0;
        }
        DAT_104c0390 = (Node *)DAT_11b0b938(0x18a0, "tsm_lprootnode");
        DAT_104c0390->Init();
        DAT_104c0390->LoadIt3(FUN_1013f8a0("mossroot.it3", 0), 0, 0);
        break;
    case MAP_TOKATYU:
        if (DAT_104c0390 != 0)
        {
            DAT_104c0390->Release();
            DAT_11b1ba40(DAT_104c0390);
            DAT_104c0390 = 0;
        }
        DAT_104c0390 = (Node *)DAT_11b0b938(0x18a0, "tsm_lprootnode");
        DAT_104c0390->Init();
        DAT_104c0390->LoadIt3(FUN_1013f8a0("tokatyu.it3", 0), 0, 0);
        break;
    case MAP_WATER_TEMPLE:
        if (DAT_104bfe80 < 2 && DAT_10255f08 != 0)
        {
            Node *water = DAT_10255f08->Find("mizuT1", 0);
            if (water != 0)
                water->SetFlags(NODE_FLAGS_HIDE);
            water = DAT_10255f08->Find("mizuT2", 0);
            if (water != 0)
                water->SetFlags(NODE_FLAGS_HIDE);
        }
        break;
    }
    if (DAT_104bfe80 < 1 &&
        (DAT_10c62ed0 == 0x259 || DAT_10c62ed0 == 0x25c || DAT_10c62ed0 == 0x25d || DAT_10c62ed0 == 0x1f5 ||
         DAT_10c62ed0 == 0x1f6 || DAT_10c62ed0 == 0x1f7 || DAT_10c62ed0 == 0x1f8 || DAT_10c62ed0 == 0x1f9 ||
         DAT_10c62ed0 == 0x1fa || DAT_10c62ed0 == 0x1fb || DAT_10c62ed0 == 0x1fc || DAT_10c62ed0 == 0x1fd ||
         DAT_10c62ed0 == 0x1fe))
    {
        int index = 0;
        Node *shadow;
        do
        {
            shadow = (Node *)DAT_10255f08->M_1019ed10("@shadowmap", index);
            if (shadow != 0)
                shadow->SetFlags(NODE_FLAGS_HIDE);
            index++;
        } while (shadow != 0);
    }
    DAT_102028c8 = LoadTexture("count_d.iti", 0);
    if (DAT_104c0394 != 0)
    {
        DAT_11b1ba40(DAT_104c0394);
        DAT_104c0394 = 0;
    }
    int i = 0;
    do
    {
        int slot = i + FIRST_MAP_SLOT;
        DAT_10468730[slot] = 0;
        if (DAT_1021f3c0[slot] == SLOT_STATE_ENEMY && DAT_10db6ee8[slot].root != 0)
        {
            int code = DAT_10daaae8[slot].stats[SCRIPT_CODE_STAT];
            DAT_1030b668[slot] = 0;
            DAT_102548c0[slot] = 0;
            DAT_10219e40[slot] = 0;
            if (code > OBJECT_SCRIPT_TOTAL || (DAT_10daaae8[slot].kind == KIND_BOSS && code % 100 != 0))
            {
                if (DAT_10daaae8[slot].kind == KIND_ARMOUR)
                {
                    Node *armour = DAT_10db6ee8[slot].root->Find("@EN_016ar.lwo", 0);
                    if (armour != 0)
                        armour->SetFlags(NODE_FLAGS_HIDE);
                }
                int low = (DAT_10daaae8[slot].kind == KIND_BOSS || DAT_1030b668[slot] == KIND_BOSS)
                              ? DAT_10daaae8[slot].stats[SCRIPT_CODE_STAT] % 100
                              : 0;
                int value = DAT_10daaae8[slot].stats[SCRIPT_CODE_STAT];
                FUN_100404e0(slot, value / 10000000 % 100, value / 100000 % 100, value / 1000 % 100, low, 0, 0);
            }
            FUN_1001b5b0(slot);
        }
        i++;
    } while (i + FIRST_MAP_SLOT < FIRST_OBJECT_SLOT);
    DAT_1021b928[0] = 0;
    DAT_1021b928[1] = 0;
    DAT_1021b928[2] = 0;
    DAT_1021b928[3] = 0;
    DAT_1021b928[4] = 0;
    for (int link = 0; link < CHAR_LINK_COUNT; link++)
    {
        DAT_1049f0c8[link].first = 0;
        DAT_1049f0c8[link].second = 0;
    }
    int side = 0;
    int linkBase = 0;
    for (int slot = FIRST_OBJECT_SLOT; slot < END_OBJECT_SLOT; slot++)
    {
        if (DAT_10db6ee8[slot].root != 0 && _stricmp(DAT_10db6ee8[slot].root->folder, OBJECT_LINK_NAME) == 0)
        {
            (&DAT_1049f0c8[0].first)[linkBase + side] = slot;
            DAT_10868838[slot][0].SetIdentity();
            DAT_10868838[slot][0].SetTranslation(DAT_10db6ee8[slot].matrix.GetTranslation());
            DAT_10868838[slot][0].translation.z = DAT_10868838[slot][0].translation.z + BEAM_POINT_HEIGHT_1;
            DAT_10868838[slot][1].SetIdentity();
            DAT_10868838[slot][1].SetTranslation(DAT_10db6ee8[slot].matrix.GetTranslation());
            DAT_10868838[slot][1].translation.z = DAT_10868838[slot][1].translation.z + BEAM_POINT_HEIGHT_2;
            DAT_10868838[slot][2].SetIdentity();
            DAT_10868838[slot][2].SetTranslation(DAT_10db6ee8[slot].matrix.GetTranslation());
            side++;
            DAT_10868838[slot][2].translation.z = DAT_10868838[slot][2].translation.z + BEAM_POINT_HEIGHT_3;
            if (side == 2)
            {
                linkBase += 2;
                side = 0;
                if (linkBase == 2 * CHAR_LINK_COUNT)
                    break;
            }
        }
    }
    if (DAT_10c62ed0 == 12)
    {
        if (DAT_10db6ee8[FIRST_MAP_SLOT].root != 0)
        {
            DAT_10daaae8[FIRST_MAP_SLOT].kind = (DAT_10c62edc[FLAG_NIGHT_SLOT] != 0 ? 0x3a : 0) + 2;
            DAT_102540ca = 2;
            DAT_10daaae8[FIRST_MAP_SLOT].speedScale = 1.0f;
            DAT_10db6ee8[FIRST_MAP_SLOT].speedScale = 1.0f;
        }
    }
    else if (DAT_10c62ed0 <= 6 && 1 < DAT_10c62ed0)
    {
        if (DAT_10db6ee8[FIRST_MAP_SLOT].root != 0)
        {
            DAT_10daaae8[FIRST_MAP_SLOT].kind = (DAT_10c62edc[FLAG_NIGHT_SLOT] != 0 ? 0x3a : 0) + 2;
            DAT_102540ca = 2;
            DAT_10daaae8[FIRST_MAP_SLOT].speedScale = 1.0f;
            DAT_10db6ee8[FIRST_MAP_SLOT].speedScale = 1.0f;
        }
    }
    memset(DAT_1049efb0, 0, NODE_FLAG_WORDS * sizeof(int));
    if (DAT_104bfe6c == 0)
        FUN_1016b230("");
    else
        DAT_1032d868 = 0;
    if (DAT_1032d69c != 0)
    {
        for (int slot = FIRST_OBJECT_SLOT; slot < END_OBJECT_SLOT; slot++)
        {
            int kind = DAT_10daaae8[slot].kind;
            if (kind == 0x20 || kind == 0x23 || kind == 0x24 || kind == 0x2f || kind == 0x58)
                Char_UpdateByKind(slot);
        }
    }
    if (DAT_10daaae8[3].kind != KIND_FOLLOWER)
        DAT_10daaae8[3].kind = KIND_FOLLOWER;
    if (DAT_10daaae8[6].kind != KIND_FOLLOWER)
        DAT_10daaae8[6].kind = KIND_FOLLOWER;
    if (DAT_10daaae8[7].kind != KIND_FOLLOWER)
        DAT_10daaae8[7].kind = KIND_FOLLOWER;
    if (DAT_10daaae8[11].kind != KIND_FOLLOWER)
        DAT_10daaae8[11].kind = KIND_FOLLOWER;
    if (DAT_10daaae8[13].kind != KIND_GUARD)
        DAT_10daaae8[13].kind = KIND_GUARD;
    if (DAT_10daaae8[15].kind != KIND_GUARD)
        DAT_10daaae8[15].kind = KIND_GUARD;
    if (DAT_10daaae8[1].kind != KIND_FOLLOWER)
        DAT_10daaae8[1].kind = KIND_FOLLOWER;
    if (DAT_10daaae8[2].kind != KIND_FOLLOWER)
        DAT_10daaae8[2].kind = KIND_FOLLOWER;
    if (DAT_10c62ed0 > MAP_FIRST_LATE - 1)
    {
        if ((DAT_10c62edc[FLAG_ELEMENT_A] != 0 || DAT_10c62edc[FLAG_ELEMENT_B] == 0) &&
            DAT_10c62edc[FLAG_OPEN_ALL] == 0)
            DAT_10daaae8[16].kind = KIND_WATCHER;
        else
            DAT_10daaae8[16].kind = KIND_FOLLOWER;
    }
    // Not an else-if: the original tests the follower kind and then the map again (cl keeps the second test)
    if (DAT_10daaae8[16].kind != KIND_FOLLOWER && DAT_10c62ed0 < MAP_FIRST_LATE)
        DAT_10daaae8[16].kind = KIND_FOLLOWER;
    if (DAT_10daaae8[18].kind != KIND_FOLLOWER)
        DAT_10daaae8[18].kind = KIND_FOLLOWER;
    if (DAT_10daaae8[20].kind != KIND_FOLLOWER)
        DAT_10daaae8[20].kind = KIND_FOLLOWER;
    if (DAT_10daaae8[56].kind != KIND_LAST_FIXED)
        DAT_10daaae8[56].kind = KIND_LAST_FIXED;
    if (DAT_10255f08 != 0 && DAT_10c62edc[FLAG_OPEN_ALL] != 0 && DAT_10c62ed0 == MAP_FACE_PLAYER)
    {
        for (int slot = FIRST_OBJECT_SLOT; slot < END_OBJECT_SLOT; slot++)
        {
            if (DAT_1021f3c0[slot] == KIND_SHADOW_OBJECT)
            {
                DAT_10db6ee8[slot].alpha = 0;
                DAT_10db6ee8[slot].root->M_10194730(NODE_VALUE_HIDE, NODE_MASK_BODY);
                DAT_10db6ee8[slot].root->M_10194730(NODE_VALUE_HIDE, NODE_MASK_SHADOW);
            }
        }
    }
    return 0;
}
