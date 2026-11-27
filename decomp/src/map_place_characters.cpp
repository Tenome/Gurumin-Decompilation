// Populating the map when it is entered (run by the field and event modes 0x100fc4a0/0x100fda40): the player is
// reloaded, every model slot is parked far away, then the map's records are applied. A character record clones the
// model of its template slot into the next free map slot (58 and up) and fills the slot's tables, an actor record
// loads its own model file into the slot it names.

#include <string.h>

#include "vec4.h"

#include "matrix4.h"

#include "node.h"

#include "mesh.h"

#include "anim_model.h"

#include "char_slot.h"

#include "char_state.h"

#include "last_position.h"

#include "model_path.h"

#include "char_tables.h"
#include "action_select_vars.h"
#include "char_vars.h"
#include "game_vars.h"
#include "map_vars.h"
#include "render_vars.h"
#include "world.h"
#include "script_flag.h"
#include "texture_load.h"
#include "map_placement.h"

extern "C" void FUN_1003b6e0(int index, int variant);      // Costume_LoadPlayerModel
extern "C" int FUN_100154c0(int item);                     // Char_WearHelmet
extern "C" int FUN_10012510(int mode, int index);          // attach/remove the drill tool
extern "C" int FUN_10012220(int index, int value);         // Model_ApplyCommands
extern "C" int FUN_100143f0(int index, const char *name);  // swaps the item model of a character
extern "C" int FUN_10161920(const char *name);             // texture slot holding `name`

#define MODEL_TABLE_COUNT 0x100
#define MODEL_TABLE_UNROLL 8
#define FIRST_CHARACTER_SLOT 0x3a           // slots 58.. are the map's characters; below are the fixed cast
#define END_MODEL_SLOT 0x7c
#define FAR_AWAY 200000.0f                  // parked models stand this far out
#define PARK_SPACING 1000.0f
#define OBJECT_SLOT_LIMIT 0x5c              // more map characters than there are slots set the "too many enemies" text
#define PLACEMENT_LIFT 5                    // placed models stand this high above the record's position
#define SLOT_STATE_ENEMY 3                  // DAT_1021f3c0 of a map character
#define SLOT_STATE_ACTOR 2                  // DAT_1021f3c0 of an actor
#define DEFAULT_PERCENT 100                 // a percentage of zero or less is read as 100
#define PERCENT 100
#define PI_APPROX 3.1415f                   // pi as the game writes it
#define HEADING_STEP_DEGREES 30
#define SHARES_MODEL_NONE 0
#define ROUTE_CAMERA_FOLLOW 100             // CharSlot::m_00ac of a character the camera keeps in view
#define MODEL_ALPHA_OPAQUE 1.0f
#define MODEL_SPEED_SCALE 100.0f
#define MODEL_MOTION_BLEND 5
#define MODEL_MOTION_SPAWN 3
#define MODEL_COLLISION_LIMIT 2
#define STAT_SCRIPT_CODE 9                  // CharSlot::stats entry that holds the packed script code
#define STAT_VARIANT 15                     // CharSlot::stats entry that holds the costume variant
#define NODE_MASK_ALL (-1)
#define NODE_MASK_BODY 0x400
#define NODE_MASK_SHADOW 0x200
#define NODE_VALUE_HIDE_BODY 0xfe
#define NODE_VALUE_SPECIAL 0x2f
#define NODE_VALUE_FF 0xff
#define NODE_VALUE_ACTOR 0x8f
#define NODE_VALUE_PLAYER 0x1f
#define COSTUME_POPON 6
#define SLOT_ARMOUR 0x10                    // the fixed cast slots whose helmet is replaced by an item model
#define SLOT_NECKLACE 7
#define SLOT_HAT 0xb
#define HELMET_NODE "helmet"
#define MESSAGE_TOO_MANY_ENEMIES "\x93G\x90\x94\x83I\x81[\x83o\x81[\x82\xbe\x82\xbc\x82\xe6"
#define MESSAGE_FRAMES 0x50

// Tints of the map characters by CharSlot::m_0044[0] (1..6), applied to the model's colour vector.
#define TINT_1_X 0.8f
#define TINT_1_Y 0.8f
#define TINT_1_Z 0.6f
#define TINT_2_X 0.2f
#define TINT_2_Y 0.9f
#define TINT_2_Z 1.0f
#define TINT_3_X 0.3f
#define TINT_3_Y 0.9f
#define TINT_3_Z 0.5f
#define TINT_4_X 1.0f
#define TINT_4_Y 1.0f
#define TINT_4_Z 0.2f
#define TINT_5_X 0.5f
#define TINT_5_Y 0.6f
#define TINT_5_Z 0.2f
#define TINT_6_X 1.0f
#define TINT_6_Y 0.2f
#define TINT_6_Z 0.2f

// True when all four conditions of the record hold.
static __forceinline int ConditionsHold(const MapPlacement &record)
{
    return ScriptFlag(record.conditions[0]) && ScriptFlag(record.conditions[1]) && ScriptFlag(record.conditions[2]) &&
           ScriptFlag(record.conditions[3]);
}

// Takes the helmet off the model of `slot` (if it has one).
static __forceinline void RemoveHelmet(int slot)
{
    Node *helmet = DAT_10db6ee8[slot].root->Find(HELMET_NODE, 0);
    if (helmet != 0)
        helmet->parent->RemoveChild(helmet, 0);
}

// 0x1005def0 (Map_PlaceCharacters). Reloads the player (and puts on the helmet carried in slot 0), parks every
// model slot far away, clears the collision flag of every loaded model, then applies the map's records.
// A map character record (DAT_1093e2ec) that applies on this map clones the model of its template slot into the
// next free character slot starting at 58, copies the template's record, places the model at the record's
// position, applies the tint, the heading and the difficulty scaling to its health and values and resets all of its
// per-slot tables; characters that share a model mark the previous slot. An actor record (DAT_1092eee8) loads the
// fixed cast's models (the player's costume or the model file of the record) and places them. When more characters
// were placed than there are slots the "too many enemies" message is set. Returns 1.
extern "C" int FUN_1005def0(void)
{
    FUN_1003b6e0(0, 0);
    if (DAT_10c655f0 == COSTUME_POPON)
        DAT_10daaae8[0].helmet = 0;
    else
        FUN_100154c0(DAT_10daaae8[0].helmet);
    if (DAT_10db6ee8[0].root != 0)
    {
        DAT_10db6ee8[0].root->M_101946b0(NODE_VALUE_PLAYER, NODE_MASK_ALL);
        DAT_10db6ee8[0].root->M_10194730(NODE_VALUE_PLAYER, NODE_MASK_ALL);
    }
    DAT_10db6ee8[0].m_0080 = 1;
    FUN_10012510(DAT_10c652cc != 0 ? 1 : 0, 0);
    for (int i = 0; i < MODEL_TABLE_COUNT; i += MODEL_TABLE_UNROLL)
    {
        if (DAT_10faaa88[i] != 0)
            DAT_10faaa88[i]->hasCollision = 0;
        if (DAT_10faaa88[i + 1] != 0)
            DAT_10faaa88[i + 1]->hasCollision = 0;
        if (DAT_10faaa88[i + 2] != 0)
            DAT_10faaa88[i + 2]->hasCollision = 0;
        if (DAT_10faaa88[i + 3] != 0)
            DAT_10faaa88[i + 3]->hasCollision = 0;
        if (DAT_10faaa88[i + 4] != 0)
            DAT_10faaa88[i + 4]->hasCollision = 0;
        if (DAT_10faaa88[i + 5] != 0)
            DAT_10faaa88[i + 5]->hasCollision = 0;
        if (DAT_10faaa88[i + 6] != 0)
            DAT_10faaa88[i + 6]->hasCollision = 0;
        if (DAT_10faaa88[i + 7] != 0)
            DAT_10faaa88[i + 7]->hasCollision = 0;
    }
    for (int parked = 1; parked < END_MODEL_SLOT; parked++)
        DAT_10db6ee8[parked].Teleport((float)parked * PARK_SPACING - FAR_AWAY, FAR_AWAY, FAR_AWAY);
    memset(DAT_10c62e50, 0, sizeof(DAT_10c62e50));
    memset(DAT_1021f3fa, 0, 0x22);
    int slot = FIRST_CHARACTER_SLOT;
    for (int index = 0; index < DAT_10993f6c; index++)
    {
        const MapPlacement &record = DAT_1093e2ec[index];
        if (record.map != PLACEMENT_ANY_MAP && record.map != DAT_10c62ed0)
            continue;
        if (!ConditionsHold(record))
            continue;
        int variant = record.look % 1000 / 100;
        FUN_1003b6e0(record.slot, variant);
        DAT_10db6ee8[slot].CloneModel(&DAT_10db6ee8[record.slot], 1);
        Node *carrier = DAT_10db6ee8[slot].root->Find(DAT_1021a3c0[record.slot], 0);
        if (carrier != 0)
            DAT_1020e338[slot] = &carrier->worldMatrix;
        else
            DAT_1020e338[slot] = &DAT_10db6ee8[slot].matrix;
        DAT_102192c0[slot] = DAT_102192c0[record.slot];
        DAT_10db6ee8[slot].m_0294 = 0;
        DAT_10db6ee8[slot].M_10161a60();
        DAT_10db6ee8[slot].M_10161b60();
        DAT_10db6ee8[slot].Teleport((float)record.x, (float)record.y, (float)(record.z + PLACEMENT_LIFT));
        DAT_1021f3c0[slot] = SLOT_STATE_ENEMY;
        DAT_10daaae8[slot] = DAT_10daaae8[record.slot];
        FUN_10012220(slot, -1);
        DAT_10daaae8[slot].routeRecord = record.routeRecord;
        DAT_10daaae8[slot].m_00d8 = record.score;
        int percentHealth = record.healthPercent;
        int percentStat3 = record.stat3Percent;
        int percentStat4 = record.stat4Percent;
        int percentCoins = record.coinsPercent;
        if (percentHealth <= 0)
            percentHealth = DEFAULT_PERCENT;
        if (percentStat4 <= 0)
            percentStat4 = DEFAULT_PERCENT;
        if (percentStat3 <= 0)
            percentStat3 = DEFAULT_PERCENT;
        if (percentCoins <= 0)
            percentCoins = DEFAULT_PERCENT;
        int tint = record.look % 100000 / 10000;
        DAT_10daaae8[slot].m_0044[0] = tint;
        if (tint == 1)
            DAT_10db6ee8[slot].scale.Set(TINT_1_X, TINT_1_Y, TINT_1_Z, 0);
        else if (tint == 2)
            DAT_10db6ee8[slot].scale.Set(TINT_2_X, TINT_2_Y, TINT_2_Z, 0);
        else if (tint == 3)
            DAT_10db6ee8[slot].scale.Set(TINT_3_X, TINT_3_Y, TINT_3_Z, 0);
        else if (tint == 4)
            DAT_10db6ee8[slot].scale.Set(TINT_4_X, TINT_4_Y, TINT_4_Z, 0);
        else if (tint == 5)
            DAT_10db6ee8[slot].scale.Set(TINT_5_X, TINT_5_Y, TINT_5_Z, 0);
        else if (tint == 6)
            DAT_10db6ee8[slot].scale.Set(TINT_6_X, TINT_6_Y, TINT_6_Z, 0);
        DAT_10db6ee8[slot].m_2390 = MODEL_COLLISION_LIMIT;
        for (int material = 0; material < DAT_10db6ee8[slot].loadedRoot->materialCount; material++)
        {
            if (DAT_10db6ee8[slot].loadedRoot->materials[material].textures[0][0] != '\0')
            {
                DAT_10467288[slot] = FUN_10161920(DAT_10db6ee8[slot].loadedRoot->materials[material].textures[0]);
                break;
            }
        }
        int scriptCode = record.scriptCode;
        int difficulty = DAT_10c635f8;
        if (difficulty == 1)
        {
            percentHealth = percentHealth * 0x4b / PERCENT;
            percentStat3 = percentStat3 * 0x4b / PERCENT;
        }
        if (difficulty == 2)
        {
            percentHealth = percentHealth * 0x7d / PERCENT;
            percentStat3 = percentStat3 * 0x96 / PERCENT;
            if (DAT_10c66cc0 == 1)
            {
                percentHealth = percentHealth * 0x96;
                percentStat3 = percentStat3 * 0xbe;
                percentHealth = percentHealth / PERCENT;
                percentStat3 = percentStat3 / PERCENT;
            }
            else if (DAT_10c66cc0 != 0)
            {
                percentHealth = percentHealth * 700;
                percentStat3 = percentStat3 * 0x32a;
                percentHealth = percentHealth / PERCENT;
                percentStat3 = percentStat3 / PERCENT;
            }
        }
        int health = DAT_10daaae8[slot].stats[STAT_MAX_HEALTH] * percentHealth / PERCENT;
        if (health <= 1)
            health = 1;
        DAT_10daaae8[slot].stats[STAT_MAX_HEALTH] = health;
        DAT_10daaae8[slot].stats[STAT_HEALTH] = health;
        DAT_10daaae8[slot].stats[STAT_EFFECT_PARAM] = DAT_10daaae8[slot].stats[STAT_EFFECT_PARAM] * percentStat3 / PERCENT;
        DAT_10daaae8[slot].stats[STAT_VALUE_4] = DAT_10daaae8[slot].stats[STAT_VALUE_4] * percentStat4 / PERCENT;
        DAT_10daaae8[slot].stats[STAT_SCRIPT_CODE] = scriptCode;
        memset(&DAT_1020b430[slot], 0, sizeof(CharState));
        DAT_10210d30[slot] = 0;
        DAT_104686b0[slot] = 0;
        DAT_1020d4c0[slot] = 0;
        DAT_102550e0[slot] = 0;
        DAT_10219770[slot] = 0;
        DAT_103153b0[slot] = 0;
        DAT_10218fc0[slot] = 0;
        DAT_10210b30[slot] = 0;
        float zeroPosition = 0;
        DAT_10daaae8[slot].coins = DAT_10daaae8[slot].coins * percentCoins / PERCENT;
        DAT_10daaae8[slot].stats[STAT_VARIANT] = variant;
        DAT_10467ba0[slot] = 0;
        DAT_102239d0[slot] = 0;
        DAT_10254090[slot] = 1;
        DAT_10802bf8[slot].position.Fill(zeroPosition);
        float zeroStep = 0;
        DAT_10802bf8[slot].step.Fill(zeroStep);
        DAT_10315350[slot] = 0;
        DAT_1042d9e8[slot] = 0;
        DAT_1021f6c0[slot] = 0;
        DAT_10254310[slot] = 0;
        DAT_10db6ee8[slot].root->M_10194630(slot, NODE_MASK_BODY);
        DAT_10db6ee8[slot].root->M_101946b0(NODE_VALUE_HIDE_BODY, NODE_MASK_BODY);
        DAT_10db6ee8[slot].root->M_10194730(NODE_VALUE_SPECIAL, NODE_MASK_BODY);
        DAT_10db6ee8[slot].root->M_10194630(0, NODE_MASK_SHADOW);
        DAT_10db6ee8[slot].root->M_101946b0(NODE_VALUE_FF, NODE_MASK_SHADOW);
        DAT_10db6ee8[slot].root->M_10194730(NODE_VALUE_SPECIAL, NODE_MASK_SHADOW);
        DAT_10db6ee8[slot].root->M_101a8760();
        DAT_10db6ee8[slot].locked = 0;
        if (DAT_10daaae8[slot].m_00ac == ROUTE_CAMERA_FOLLOW)
        {
            DAT_1032d5b8[DAT_1049f0f0] = &DAT_10db6ee8[slot].matrix.translation;
            DAT_1021bbc0[DAT_1049f0f0] = 0;
            DAT_1049f0f0 = DAT_1049f0f0 + 1;
        }
        DAT_1021f9c0[slot] = 1;
        DAT_10255a88[slot] = 1;
        DAT_10daaae8[slot].m_00a8 = 0;
        float heading = (float)(record.look % 100 * HEADING_STEP_DEGREES) * PI_APPROX / 180.0f;
        DAT_10db6ee8[slot].targetHeading = heading;
        DAT_10db6ee8[slot].heading = heading;
        if (DAT_10daaae8[slot].m_00a0 != SHARES_MODEL_NONE)
        {
            DAT_10db6ee8[slot].m_17e8 = 1;
            DAT_10db6ee8[slot].root->M_101946b0(0, NODE_MASK_ALL);
            DAT_10db6ee8[slot].root->M_10194730(0, NODE_MASK_ALL);
            DAT_10daaae8[slot - 1].m_00a4 = slot;
            DAT_10daaae8[slot - 1].m_00a8 = 1;
        }
        slot++;
    }
    if (slot > OBJECT_SLOT_LIMIT)
    {
        DAT_104bfd4c = MESSAGE_FRAMES;
        DAT_104c01fc = 0;
        DAT_104bf8c8 = 1;
        DAT_104bf8cc = 0;
        DAT_104bf8d0 = 0;
        DAT_10467ea0[0] = 0;
        int character = 0;
        char value;
        do
        {
            value = MESSAGE_TOO_MANY_ENEMIES[character];
            DAT_10219bc0[character] = value;
            character++;
        } while (value != '\0');
    }
    for (int index = 0; index < DAT_1093e2e8; index++)
    {
        const MapPlacement &record = DAT_1092eee8[index];
        if (record.map != PLACEMENT_ANY_MAP && record.map != DAT_10c62ed0)
            continue;
        if (!ConditionsHold(record))
            continue;
        if (record.slot < FIRST_CHARACTER_SLOT)
        {
            FUN_1003b6e0(record.slot, 0);
            if (record.slot == SLOT_ARMOUR)
            {
                RemoveHelmet(record.slot);
                if (DAT_10c66cc4 != 0)
                    FUN_100143f0(record.slot, "G_003E.it3");
            }
            if (record.slot == COSTUME_POPON)
            {
                RemoveHelmet(record.slot);
                if (DAT_10c66cc8 != 0)
                    FUN_100143f0(record.slot, "G_008E.it3");
            }
            if (record.slot == SLOT_NECKLACE)
            {
                RemoveHelmet(record.slot);
                if (DAT_10c66ccc != 0)
                    FUN_100143f0(record.slot, "G_009E.it3");
            }
            if (record.slot == SLOT_HAT)
            {
                RemoveHelmet(record.slot);
                if (DAT_10c66cd0 != 0)
                    FUN_100143f0(record.slot, "G_013E.it3");
            }
        }
        else
        {
            DAT_10db6ee8[record.slot].LoadFile(FUN_1013f8a0(record.modelFile, 0));
            DAT_10db6ee8[record.slot].root->ReleaseUniqueMeshData();
            DAT_10db6ee8[record.slot].m_022c.Fill(0);
            DAT_10db6ee8[record.slot].alpha = MODEL_ALPHA_OPAQUE;
            DAT_10db6ee8[record.slot].SetDropCollider(DAT_10255f08);
            DAT_10db6ee8[record.slot].speedScale = MODEL_SPEED_SCALE;
            DAT_10db6ee8[record.slot].LoadModel(FUN_1013f8a0(DAT_10b314c8[record.modelDefinition].path, 0));
            FUN_10012220(record.modelDefinition, record.slot);
            if (record.slot != 0)
                DAT_10db6ee8[record.slot].PlayMotion(MODEL_MOTION_SPAWN, 0, MODEL_MOTION_BLEND, 1.0f);
            DAT_1021f3c0[record.slot] = SLOT_STATE_ACTOR;
            int character = 0;
            char value;
            do
            {
                value = record.name[character];
                DAT_10daaae8[record.slot].name[character] = value;
                character++;
            } while (value != '\0');
        }
        DAT_10daaae8[record.slot].m_00a8 = 0;
        DAT_10db6ee8[record.slot].Teleport((float)record.x, (float)record.y, (float)(record.z + PLACEMENT_LIFT));
        DAT_10db6ee8[record.slot].root->M_10194630(record.slot, NODE_MASK_BODY);
        DAT_10db6ee8[record.slot].root->M_10194630(0, NODE_MASK_SHADOW);
        DAT_10db6ee8[record.slot].root->M_101946b0(NODE_VALUE_FF, NODE_MASK_ALL);
        DAT_10db6ee8[record.slot].root->M_10194730(NODE_VALUE_ACTOR, NODE_MASK_ALL);
        memset(&DAT_1020b430[record.slot], 0, sizeof(CharState));
        DAT_10210b30[record.slot] = 0;
        DAT_10254090[record.slot] = 1;
    }
    return 1;
}
