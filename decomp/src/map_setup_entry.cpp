// Setting up the map named by an entry of the map table (the second map request of Game_Startup, DAT_1021cff8 = 2;
// its sibling 0x100fc4a0 takes the map number from the script). The per-map globals are cleared, the entry's two
// model files are loaded (a model with a water surface gets the ice nodes), the map's marker nodes (fish, "ksk"
// levers, lights, camera jump points) are collected, and the characters are placed (Map_PlaceCharacters).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vec4.h"
#include "matrix4.h"
#include "node.h"
#include "mesh.h"
#include "anim_model.h"
#include "game.h"
#include "gimmick_slot.h"
#include "model_bank.h"
#include "tri_batch.h"
#include "effect_manager.h"
#include "texture_load.h"
#include "attack_start.h"

#include "camera_vars.h"
#include "char_tables.h"
#include "char_vars.h"
#include "game_vars.h"
#include "map_vars.h"
#include "render_vars.h"
#include "ui_vars.h"
#include "world.h"
#include "switch_slots.h"
#include "ground_height.h"
#include "game_load_data.h"
#include "map_placement.h"
#include "action_select_vars.h"
#include "npc_lookout.h"
#include "sound_vars.h"
#include "char_hikari.h"
#include "script_vars.h"
#include "map_setup.h"

extern "C" int FUN_1000eb90(void);                // AntiDebug_Check
extern "C" int FUN_1000d9d0(void);
extern "C" int FUN_100114a0(void);
extern "C" int FUN_10011fb0(int frozen);          // Water_SetFrozen
extern "C" int FUN_100135b0(void);
extern "C" int FUN_10023900(void);                // Map_ResetOnEnter
extern "C" int FUN_100fb7b0(void);                // Scene_Reset
extern "C" int FUN_10057c40(int holder, Node *light, int kind);
#include "map_trigger_setup.h"
extern "C" int FUN_1013f880(void);                // the game's random number
extern "C" int FUN_1013faf0(void);                // FrameTimer_ResetNext2
extern "C" int FUN_101619a0(void);
extern "C" int FUN_1016a8e0(void);
extern "C" int FUN_1016a8f0(void);
extern "C" int FUN_1016af50(Node *mapRoot);       // starts a camera move (camera.cpp declares the argument as an int)
extern "C" int FUN_1016b010(int mode);
extern "C" int FUN_10161920(const char *name);    // Texture_FindByName
extern "C" int FUN_10001b40(void);                // SoundArc_Message221
extern "C" void FUN_10147760(void);               // BufferPool_ReleaseDynamic
extern "C" int FUN_10148350(void);                // TexCache_ReleaseAll
int Camera_Refresh(void);                         // camera_refresh.cpp
int FUN_1014b1c0(short value);                    // type_parse.cpp
int FUN_100106e0(const char *path);               // Save_Write (C++ linkage)
int FUN_1000c720(Matrix4 *view, const Matrix4 &projection, Vec4 *defaultLight);   // Characters_Draw (character_draw.cpp)
extern "C" int FUN_1000c620(Matrix4 *world, const Matrix4 *view, int a3, const Matrix4 *shadowMatrix, float shadowX,
                             float shadowY, float shadowZ, int shadowTexture, float shadowScale, int shadowSize,
                             const Vec4 *lightDirection, float modelSize, AnimModel *model, float alpha);

#define MAP_MODEL_COUNT 2                  // the entry's name model and file model
#define MAP_MODEL_NAME_SIZE 0x80
#define MAP_MODEL_FLAG_WATER 0x20000       // node flags of a water surface
#define NODE_NAME_CAPACITY 0x40            // second argument of AddChild
#define ICE_RIPPLE_DRAW_MODE 0xc
#define ICE_SURFACE_DRAW_MODE 0x18
#define ICE_RIPPLE_FLAGS 0x22
#define ICE_SURFACE_FLAGS 0x24
#define ICE_RIPPLE_UV_SCALE 0.003f
#define ICE_FIRST_TEXTURE 0                // Material::textures index of the ripple texture
#define ICE_ENVIRONMENT_TEXTURE 3          // and of its environment texture
#define FANNEL_COUNT 4
#define FANNEL_HIDDEN_X -200000.0f         // the "fannel" nodes wait far away
#define FANNEL_NAME_DIGIT '0'
#define KSK_MAX 0x40
#define KSK_PART_MAX 6
#define KSK_BODY_STATE 4
#define KSK_SWAY_BASE 10
#define KSK_SWAY_RANGE 6
#define KSK_PART_FLAGS_CLEARED 4
#define KSK_PART_FLAGS_SET 0x20
#define FISH_MAX 0x20
#define LIGHT_MAX 0x40
#define FIRELIGHT_ID_BASE 0x80
#define CANDLE_ID_BASE 0x100
#define JUMP_MAX 0x20
#define JUMP_KIND_Z_BASE 3
#define MINIMAP_FLAGS 0x4000004            // FindByFlags mask when the map has no "MiniMap" node
#define MAP_MODEL_RENDER_MODE 8
#define STATIC_COLLECTOR_NONE 0
#define GIMMICK_SLOT_COUNT 0x40
#define PLAYER_MODEL_COUNT 0x80
#define CHARACTER_MODEL_COUNT 124
#define MAP_SHORT_START 0x25d              // the map with the shorter starting distance
#define START_DISTANCE 16.0f
#define START_DISTANCE_SHORT 8.0f
#define TITLE_ICE 'i'                      // first character of the entry's title: icy floor
#define TITLE_STORY 'S'
#define TITLE_UNDERGROUND 'U'
#define TITLE_WATER 'I'
#define TITLE_PLAIN '1'
#define ICE_SETTLE_TIMER 0x1e
#define WINDOW_SLOTS_CLEARED 3
#define NOISE_SLOTS 16
#define OPEN_WINDOW_MODE 15
#define ANTI_TAMPER_FLAG_STORY 1

// Copies the zero-terminated `source` to `dest` the way the game does (one byte at a time).
static __forceinline void CopyString(char *dest, const char *source)
{
    const char *src = source;
    char c;
    do
    {
        c = *src;
        dest[src - source] = c;
        src++;
    } while (c != 0);
}

// Node::M_1019ed10 returns the node it finds.
static __forceinline Node *FindStripped(Node *root, const char *name, int index)
{
    return (Node *)root->M_1019ed10(name, index);
}

// 0x100fda40 (Map_SetupFromEntry). Sets up the map of entry `map` of the map table: clears the per-map globals and
// tables, loads the entry's two .it3 models (adding the "ICERIP2"/"ICERIP"/"ICERIPA" ice nodes to a model with a
// water surface), installs the draw callbacks on the first, finds the map's marker nodes by name ("fannel" effect
// nodes, "MiniMap", "@yotsu", "@BurankoRoot", "swan", "sakana01", "@kskNN_" levers with their "+" parts,
// "@firelight", "@CandleNull" and the "@Jump"/"@VJump"/"@YJump"/"@ZJump" camera points), starts the camera move,
// collects the model bank's lights, places the characters (Map_PlaceCharacters) and runs Scene_Reset. While a demo
// or recording starts, the player's world matrix is kept across the unloading of the models. Returns 0. C++ linkage on purpose: the Matrix4 local gives the function an exception
// frame, which an extern "C" definition would lose.
int FUN_100fda40(int map)
{
    int keepPlayerMatrix;
    int i;
    int startValue;
    char names[MAP_MODEL_COUNT][MAP_MODEL_NAME_SIZE];
    char kskName[0x40];
    char partName[0x40];
    int modelIndex[MAP_MODEL_COUNT];

    DAT_10db6ee8[0].m_0088 = 0;
    DAT_104c0148 = 0;
    DAT_102024ac = 0;
    FUN_1000eb90();
    DAT_104bf888 = 0;
    if (DAT_10c66cc0 != 0)
        DAT_1101d870 = 1;
    else
        DAT_1101d870 = (unsigned char)DAT_104c0332;
    DAT_104c0334 = 0;
    DAT_10f654f0 = 0;
    DAT_104bfe30 = 0;
    DAT_104bfe34 = 0;
    DAT_104bfe38 = 0;
    DAT_104bfe3c = 0;
    DAT_104bfe40 = 0;
    DAT_104bfe44 = 0;
    if (DAT_104bfdc8 != 0)
    {
        DAT_10db6ee8[DAT_104bfdd8].m_0075 = 0;
        DAT_10db6ee8[0].m_0075 = 0;
        DAT_104bfdc8 = 0;
        DAT_104bfdcc = 0;
        DAT_104c0090 = 1;
    }
    DAT_104bfdd0 = 0;
    DAT_10219bb8 = 0;
    DAT_104c0330 = 0;
    DAT_104c0331 = 0;
    DAT_10203420 = 1.0f;
    DAT_104c0318 = 0;
    DAT_11b044e0 = 0;
    DAT_104c02f0 = 0;
    DAT_104c0304 = 0;
    DAT_104c0300 = 0;
    DAT_104c0301 = 0;
    DAT_104c030a = 0;
    DAT_104c030c = 0;
    if (DAT_10db6ee8[0].root != 0)
        DAT_10db6ee8[0].root->children[1]->m_0298.y = 0;
    memset(DAT_1020aefc, 0, sizeof(DAT_1020aefc));
    memset(DAT_102550c0, 0, sizeof(DAT_102550c0));
    memset(DAT_1049ef90, 0, sizeof(DAT_1049ef90));
    memset(DAT_1020d438, 0, sizeof(DAT_1020d438));
    memset(DAT_1030bbf8, 0, 0x80 * sizeof(DAT_1030bbf8[0]));
    DAT_10db6ee8[0].m_0013 = 0;
    DAT_104c02ed = 0;
    DAT_104c02e7 = 0;
    DAT_104687b4 = 0;
    DAT_1021b3c0 = 0;
    DAT_1101d87c = 0;
    keepPlayerMatrix = 0;
    DAT_102027f8 = -1;
    Matrix4 savedPlayerMatrix;
    memset(DAT_10254310, 0, sizeof(DAT_10254310));
    memset(DAT_10447bd0, 0, sizeof(DAT_10447bd0));
    memset(DAT_104507e8, 0, sizeof(DAT_104507e8));
    memset(DAT_10219770, 0, 0x40 * sizeof(DAT_10219770[0]));
    memset(DAT_103153b0, 0, 0x40 * sizeof(int));
    memset(DAT_1020af20, 0, sizeof(DAT_1020af20));
    memset(DAT_1021fa40, 0, sizeof(DAT_1021fa40));
    memset(DAT_10467718, 0, sizeof(DAT_10467718));
    memset(DAT_104678a0, 0, sizeof(DAT_104678a0));
    DAT_104c025c = 0;
    DAT_104c025d = 0;
    DAT_104c023d = 0;
    DAT_104c023e = 0;
    DAT_10467798 = 0;
    DAT_1021a368 = 0;
    DAT_104c02b4 = 0;
    DAT_104c02b8 = 0;
    DAT_104c02dc = 0;
    DAT_104c02e4 = 0;
    DAT_1101de30 = 1;
    DAT_10b314c0 = 0;
    DAT_10db6ee8[0].important = 1;
    DAT_10210f38[0] = 0;
    DAT_1042d958[0] = 0;
    DAT_104c0260 = 0;
    DAT_104c0264 = 0;
    DAT_104c0268 = 0;
    DAT_104c0240 = 0;
    DAT_104c0244 = 0;
    DAT_104c0248 = 0;
    DAT_10db6ee8[0].speedScale = 1.0f;
    DAT_102027ec = 1.0f;
    DAT_10866de8.Fill(0.0f);
    FUN_1000d9d0();
    for (i = 0; i < 0x40; i++)
        DAT_10edbe0c[i].mesh = 0;
    DAT_104c013c = 0;
    DAT_104c0140 = FUN_10161920("lens05");
    DAT_1021cffc.SetBuffer(DAT_10223a58, 0x400);
    memset(DAT_10254c40, 0, sizeof(DAT_10254c40));
    DAT_104c021c = 0;
    DAT_10db6ee8[0].m_006c = 0;
    DAT_10db6ee8[0].m_0076 = 0;
    DAT_104c0210 = 0;
    DAT_104c0214 = 0;
    DAT_104c0218 = 0;
    DAT_104c0228 = 0;
    DAT_104c022c = 0;
    DAT_104c022d = 0;
    DAT_104687b0 = 0;
    DAT_102027e4 = 180.0f;
    DAT_104c0204 = 0;
    DAT_104c0224 = 0;
    DAT_1083631c.Fill(0.0f);
    DAT_104c01dc = 0;
    DAT_104c01e0 = 0;
    DAT_104c01e4 = 0;
    DAT_104c01e8 = 0;
    DAT_10db6ee8[0].m_0074 = 0;
    DAT_104c01f8 = 0;
    DAT_104c01d0 = 0;
    DAT_104c01cc = 0;
    DAT_104c01c8 = 0;
    DAT_104c01d8 = 0;
    DAT_104bfe50 = 0;
    DAT_104bf890 = 0;
    DAT_104c0128 = 0;
    DAT_104c01f4 = 0;
    DAT_104c019c = 0;
    DAT_104c0190 = 0;
    if (DAT_104bf8c0 != 0)
    {
        for (i = 0; i < WINDOW_SLOTS_CLEARED; i++)
        {
            if (DAT_104b7200.m_0110[i] != 0 || DAT_104b7200.m_008c[i] != 0 || DAT_104b7200.m_0098[i] != 0)
            {
                DAT_104b7200.m_0110[i] = 0;
                DAT_104b7200.m_008c[i] = 0;
                DAT_104b7200.m_0098[i] = 0;
            }
        }
    }
    DAT_10f65554 = 2;
    FUN_1016a8e0();
    if (DAT_1065cb94 != 0)
    {
        if (DAT_1065cb90 == 0)
        {
            DAT_1065cba4 = 0;
            DAT_1065cb90 = 1;
            DAT_1065cb9c = 0;
            FUN_1016a8f0();
            savedPlayerMatrix = DAT_10db6ee8[0].matrix;
            keepPlayerMatrix = 1;
            DAT_10db6ee8[0].Unload();
            FUN_100106e0("demo.sav");
        }
        FUN_1014b1c0(0);
        DAT_1065cb28.M_1013edb0();
    }
    if (DAT_1065cb98 != 0)
    {
        if (DAT_1065cb90 == 0)
        {
            savedPlayerMatrix = DAT_10db6ee8[0].matrix;
            DAT_1065cba4 = 0;
            DAT_1065cb90 = 1;
            DAT_1065cb9c = 0;
            keepPlayerMatrix = 1;
            DAT_10db6ee8[0].Unload();
            FUN_1016a8f0();
        }
        FUN_1014b1c0(0);
        DAT_1065cb28.M_1013edb0();
    }
    FUN_10023900();
    DAT_104c0134 = 1;
    DAT_104c0194 = 0;
    DAT_104c016c = 0;
    FUN_100114a0();
    DAT_11b04510 = 0;
    DAT_104c0133 = 0xf;
    DAT_104c0154 = 0;
    DAT_104c0155 = 0;
    DAT_104c00d0 = 0;
    DAT_104c00d4 = 0;
    DAT_104c00d5 = 0;
    DAT_104c0068 = 0;
    DAT_104c010c = 0;
    DAT_104c0124 = 0;
    DAT_102024c4 = 0;
    DAT_104c00e0 = 0;
    DAT_10db6ee8[0].alpha = 1.0f;
    DAT_1101d898 = 0;
    DAT_104c0110 = 0;
    DAT_104c0095 = 0;
    DAT_104c0094 = 0;
    DAT_104c0108 = 0;
    DAT_104c00c8 = 0;
    DAT_104c00b7 = 0;
    DAT_104bfe28 = 0;
    DAT_104c00cc = 0;
    DAT_10db6ee8[0].m_02a1 = 0;
    FUN_10148350();
    FUN_10147760();
    DAT_1021f338 = 0x5c;
    DAT_1020b22c = 0;
    DAT_104c003c = 0;
    DAT_1101d8b0 = 0;
    DAT_1101d890 = 0;
    DAT_1101dddc = 0;
    DAT_10db6ee8[0].mode = 1;
    DAT_10db6ee8[0].verticalSpeed = 0;
    DAT_10db6ee8[0].m_237c = 1.0f;
    DAT_10db6ee8[0].alpha = 1.0f;
    DAT_104c0094 = 0;
    DAT_104c00bf = 0;
    DAT_1032d868 = 0;
    DAT_104bf87c = 0;
    DAT_104c0091 = 0;
    DAT_102024b4 = 1;
    if (DAT_104c01b0 == 0)
    {
        for (i = 0; i < 0x44c; i++)
            DAT_10447338[i] = -11;
        if (FUN_10001b40() != 0)
            FUN_10001b40();
    }
    memset(DAT_1030bdf8, 0, sizeof(DAT_1030bdf8));
    memset(DAT_1020d5c8, 0, sizeof(DAT_1020d5c8));
    DAT_10c635ec = 0;
    memset(DAT_10255040, 0, sizeof(DAT_10255040));
    ActionEvent *event = DAT_10254110;
    do
    {
        event->key = -1;
        event++;
    } while (event < DAT_10254110 + ACTION_EVENT_COUNT);
    memset(DAT_1020d660, 0, sizeof(DAT_1020d660));
    memset(DAT_1020b1e0, 0, sizeof(DAT_1020b1e0));
    memset(DAT_10219f78, 0, sizeof(DAT_10219f78));
    memset(DAT_10210d30, 0, sizeof(DAT_10210d30));
    memset(DAT_102550e0, 0, 0x20 * sizeof(int));
    memset(DAT_104bfb08, 0, sizeof(DAT_104bfb08));
    memset(DAT_104481d8, 0, sizeof(DAT_104481d8));
    memset(DAT_10468430, 0, 0x20 * sizeof(int));
    memset(DAT_1020d860, 0, 0x60 * sizeof(int));
    memset(DAT_10c62ee0, 0, sizeof(DAT_10c62ee0));
    for (i = 1; i < PLAYER_MODEL_COUNT; i++)
        DAT_10db6ee8[i].Unload();
    memset(DAT_1030bbf8, 0, 0x40 * sizeof(DAT_1030bbf8[0]));
    DAT_10daaa80.Reset();
    DAT_1101de28 = 0;
    for (i = 0; i < DAT_104bfdc4; i++)
        DAT_10467e20[i]->RemoveChild(DAT_1021b548[i], 0);
    DAT_104bfdc4 = 0;
    DAT_104bf87c = 0;
    for (i = 0; i < 2; i++)
    {
        DAT_10f65848[i].x = (float)(-FUN_1013f880() % 1000 - 500);
        DAT_10f65848[i].y = (float)(-FUN_1013f880() % 1000 - 500);
        DAT_10f65848[i].z = 0;
        DAT_10f65848[i].w = 0;
    }
    DAT_10c62ed0 = DAT_10874848.m_95e14[map].map;
    startValue = DAT_10874848.m_95e14[map].value;
    CopyString(names[0], DAT_10874848.m_95e14[map].name);
    CopyString(names[1], DAT_10874848.m_95e14[map].file);
    FUN_1016b010(0);
    DAT_1101ddf0 = DAT_10874848.m_95e14[map].title[0] != TITLE_PLAIN;
    DAT_104bfdac = 0;
    modelIndex[0] = -1;
    modelIndex[1] = -1;
    DAT_104c007c = 0;
    DAT_104c0080 = 0;
    DAT_104c0084 = 0;
    char *name = names[0];
    char *record = DAT_1030baf8[0];
    for (i = 0; i < MAP_MODEL_COUNT; i++, name += MAP_MODEL_NAME_SIZE, record += sizeof(DAT_1030baf8[0]))
    {
        modelIndex[i] = i;
        CopyString(record, name);
        DAT_104bfd88[0]++;
        DAT_104bfd88[1]++;
        DAT_104bfd88[i] = 0;
        strcat(name, ".it3");
        DAT_104c4c38[i].LoadIt3(FUN_1013f8a0(name, 0), 0, 0);
        Node *water = DAT_104c4c38[i].FindByFlags(MAP_MODEL_FLAG_WATER, 0, 0);
        if (water != 0)
        {
            if (DAT_10874848.m_95e14[map].title[0] == TITLE_ICE)
                DAT_104c02e7 = ICE_SETTLE_TIMER;
            Node *ripple2 = water->parent->AddChild("ICERIP2", NODE_NAME_CAPACITY);
            Node *ripple = ripple2->AddChild("ICERIP", NODE_NAME_CAPACITY);
            ripple2->CopyMesh(water, 1);
            ripple2->drawMode = ICE_RIPPLE_DRAW_MODE;
            ripple2->SetFlags(ICE_RIPPLE_FLAGS);
            ripple2->ResetVertexColours(0);
            ripple->CopyMesh(water, 1);
            ripple->ResetVertexColours(ICE_RIPPLE_UV_SCALE);
            ripple->matrix = water->matrix;
            ripple->drawMode = ICE_SURFACE_DRAW_MODE;
            ripple->SetFlags(ICE_RIPPLE_FLAGS);
            strcpy(ripple->materials[ripple->packs[0].material].textures[ICE_FIRST_TEXTURE], "mp_609");
            FUN_10174ce0(ripple->materials[ripple->packs[0].material].textures[ICE_FIRST_TEXTURE], 0, 0);
            strcpy(ripple->materials[ripple->packs[0].material].textures[ICE_ENVIRONMENT_TEXTURE], "env2");
            FUN_10174ce0(ripple->materials[ripple->packs[0].material].textures[ICE_ENVIRONMENT_TEXTURE], 0, 0);
            DAT_104c0080 = ripple2;
            Node *iceSurface = water->parent->AddChild("ICERIPA", NODE_NAME_CAPACITY);
            iceSurface->CopyMesh(water, 1);
            iceSurface->matrix = water->matrix;
            iceSurface->SetFlags(ICE_SURFACE_FLAGS);
            iceSurface->ComputeBounds();
            DAT_104c003c |= 2;
            DAT_104c0084 = iceSurface;
            DAT_104c007c = water;
            DAT_104bfe28 = 1;
            water->CreateChildCopies();
            DAT_1101de28 = 1;
        }
        DAT_104c4c38[i].Rebuild();
        DAT_104c4c38[i].BuildMesh(1, 0);
        DAT_104c4c38[i].BuildCollisionSpheres();
        DAT_104c4c38[i].CopyPlugData();
    }
    DAT_10255f08 = &DAT_104c4c38[modelIndex[0]];
    DAT_10255f0c = &DAT_104c4c38[modelIndex[1]];
    DAT_10255f08->queueCallback = (NodeQueueCallback)FUN_1000c720;
    DAT_10255f08->drawCallback = (NodeDrawCallback)FUN_1000c620;
    DAT_104c0144 = 0;
    DAT_1032d678 = 0;
    DAT_10255f08->AddChild(0, NODE_NAME_CAPACITY)->SetUniqueName("fannel");
    DAT_10256210 = 0;
    for (i = 0; i < FANNEL_COUNT; i++)
    {
        DAT_1020aeec[i] = 0;
        DAT_1020d64c[i] = 0;
        Node *fannel = DAT_10255f08->AddChild(0, NODE_NAME_CAPACITY);
        fannel->Load(FUN_1013f8a0("drillef.it3", 0), 0, 0, 0);
        strcpy(fannel->name, "0fannel");
        fannel->name[0] = FANNEL_NAME_DIGIT + i;
        DAT_102559f4[i] = fannel;
        fannel->matrix.translation.x = FANNEL_HIDDEN_X;
        fannel->BuildMesh(1, 0);
    }
    if (DAT_10f654b0 != 0)
    {
        DAT_10255f08->M_1019b050(MAP_MODEL_RENDER_MODE);
        DAT_10255f0c->M_1019b050(MAP_MODEL_RENDER_MODE);
    }
    DAT_104c008c = 0;
    DAT_104c008c = DAT_10255f08->Find("MiniMap", 0);
    if (DAT_104c008c == 0)
        DAT_104c008c = DAT_10255f08->FindByFlags(MINIMAP_FLAGS, 0, 0);
    DAT_104c0304 = FindStripped(DAT_10255f08, "@yotsu", 0);
    Node *buranko = DAT_10255f08->Find("@BurankoRoot", 0);
    if (buranko != 0)
    {
        DAT_104c0240 = buranko;
        DAT_104c0244 = buranko->Find("@b1", 0);
        DAT_104c0248 = DAT_104c0240->Find("@b2", 0);
        if (DAT_104c0244 == 0 || DAT_104c0248 == 0)
            DAT_104c0240 = 0;
    }
    Node *swan = DAT_10255f08->Find("swan", 0);
    if (swan != 0)
    {
        DAT_104c0268 = swan;
        DAT_104c0264 = swan->Find("swanat", 0);
        if (DAT_104c0264 == 0)
            DAT_104c0268 = 0;
    }
    if (DAT_10255f08->Find("sakana01", 0) != 0)
    {
        for (i = 0; i < FISH_MAX; i++)
        {
            sprintf(partName, "sakana%02d", i + 1);
            Node *fish = DAT_10255f08->Find("sakana01", 0);
            if (fish == 0)
            {
                DAT_10210f38[0] = 0;
                break;
            }
            DAT_10210f38[i] = fish;
            DAT_1021a1c0[i] = 0;
        }
    }
    else
        DAT_10210f38[0] = 0;
    Node *horiyuka = DAT_10255f08->Find("horiyuka", 0);
    if (horiyuka != 0)
        DAT_10255f08->RemoveChild(horiyuka, 0);
    for (i = 0; i < KSK_MAX; i++)
    {
        sprintf(kskName, "@ksk%02d_", i);
        Node *ksk = FindStripped(DAT_10255f08, kskName, 0);
        if (ksk != 0)
        {
            DAT_1020af20[i] = ksk;
            sprintf(kskName, "ksk%02da", i);
            int prefix = 0;
            if (ksk->name[0] == 'C')
                prefix = 6;
            if (ksk->name[0] == 'O')
                prefix = 7;
            DAT_10253e58[i] = ksk->name[prefix] == 'K';
            DAT_1021d0b8[i] = (short)atol(&ksk->name[prefix + 6]);
            Node *body = FindStripped(DAT_10255f08, kskName, 0);
            DAT_1042da88[i] = (short)(FUN_1013f880() % KSK_SWAY_RANGE) + KSK_SWAY_BASE;
            if (body == 0)
                DAT_1020af20[i] = 0;
            else
            {
                if (body->childCount != 0)
                    body = body->children[0];
                DAT_1021fa40[i] = body;
                DAT_10255a48[i] = KSK_BODY_STATE;
                DAT_10219970[i] = 0;
                sprintf(partName, "@ksk%02d+", i);
                for (int part = 0; part < KSK_PART_MAX; part++)
                {
                    Node *node = FindStripped(DAT_10255f08, partName, part);
                    if (node == 0)
                        break;
                    node->M_10191d50();
                    node->ClearFlags(KSK_PART_FLAGS_CLEARED);
                    node->SetFlags(KSK_PART_FLAGS_SET);
                }
            }
        }
    }
    DAT_1021b3c0 = 0;
    for (i = 0; i < LIGHT_MAX; i++)
    {
        Node *light = FindStripped(DAT_10255f08, "@firelight", i);
        if (light == 0)
            break;
        int slot = DAT_1021b3c0;
        DAT_1049f0f8[slot] = light;
        DAT_10254838[slot] = DAT_1021b3c0 + FIRELIGHT_ID_BASE;
        FUN_10057c40(slot + FIRELIGHT_ID_BASE, light, 0);
        DAT_1021b3c0++;
    }
    for (i = 0; i < LIGHT_MAX; i++)
    {
        Node *light = FindStripped(DAT_10255f08, "@CandleNull", i);
        if (light == 0)
            break;
        int slot = DAT_1021b3c0;
        DAT_1049f0f8[slot] = light;
        DAT_10254838[slot] = DAT_1021b3c0 + CANDLE_ID_BASE;
        FUN_10057c40(slot + CANDLE_ID_BASE, light, 1);
        DAT_1021b3c0++;
    }
    FUN_1016af50(DAT_10255f08);
    if (DAT_10874848.m_95e14[map].title[0] == TITLE_STORY && DAT_10874848.m_95e14[map].title[1] == '1')
        DAT_104c003c |= 1;
    DAT_1049f0f0 = 0;
    for (i = 0; i < JUMP_MAX; i++)
    {
        Node *jump = DAT_10255f08->Find("@Jump", i);
        if (jump == 0)
            break;
        DAT_1032d5b8[DAT_1049f0f0] = &jump->worldMatrix.translation;
        DAT_1021bbc0[DAT_1049f0f0] = 0;
        DAT_1049f0f0++;
    }
    for (i = 0; i < JUMP_MAX; i++)
    {
        Node *jump = DAT_10255f08->Find("@VJump", i);
        if (jump == 0)
            break;
        DAT_1032d5b8[DAT_1049f0f0] = &jump->worldMatrix.translation;
        DAT_1021bbc0[DAT_1049f0f0] = 1;
        DAT_1049f0f0++;
    }
    for (i = 0; i < JUMP_MAX; i++)
    {
        Node *jump = DAT_10255f08->Find("@YJump", i);
        if (jump == 0)
            break;
        DAT_1032d5b8[DAT_1049f0f0] = &jump->worldMatrix.translation;
        DAT_1021bbc0[DAT_1049f0f0] = 2;
        DAT_1049f0f0++;
    }
    for (i = 0; i < JUMP_MAX; i++)
    {
        Node *jump = DAT_10255f08->Find("@ZJump", i);
        if (jump == 0)
            break;
        DAT_1032d5b8[DAT_1049f0f0] = &jump->worldMatrix.translation;
        DAT_1021bbc0[DAT_1049f0f0] = (char)atol(&jump->name[5]) + JUMP_KIND_Z_BASE;
        DAT_1049f0f0++;
    }
    DAT_104bf8a4 = 0;
    DAT_10804bf8.Reset();
    DAT_10804bf8.CollectLights(DAT_10255f08);
    FUN_101619a0();
    FUN_1005def0();
    if (keepPlayerMatrix != 0)
        DAT_10db6ee8[0].matrix = savedPlayerMatrix;
    DAT_104bf87c = 0;
    for (i = 1; i < GIMMICK_SLOT_COUNT; i++)
    {
        DAT_10860cec[i].node = 0;
        DAT_10860cec[i].active = 0;
        DAT_10860cec[i].frame = 0;
        DAT_10860cec[i].targetFrame = 0;
    }
    DAT_10874848.ForEachActiveTrigger(Map_SetUpTrigger);
    for (i = 0; i < CHARACTER_MODEL_COUNT; i++)
    {
        DAT_10db6ee8[i].SetDropCollider(DAT_10255f08);
        DAT_10804bf8.AddLitModel(&DAT_10db6ee8[i]);
    }
    if (DAT_104c0096 == 0)
        DAT_104c012c = startValue;
    DAT_1101d8a8 = 0;
    DAT_108029b8.Copy(DAT_10db6ee8[0].matrix.GetTranslation());
    DAT_108029b8.w = DAT_10db6ee8[0].heading;
    DAT_1030b660 = DAT_10c62ed0;
    DAT_10203450 = START_DISTANCE;
    if (DAT_10c62ed0 == MAP_SHORT_START)
        DAT_10203450 = START_DISTANCE_SHORT;
    FUN_100fb7b0();
    DAT_10255f08->ReleaseUniqueMeshData();
    DAT_10255f0c->ReleaseUniqueMeshData();
    DAT_10255f08->CollectStatic(STATIC_COLLECTOR_NONE);
    if (DAT_10874848.m_95e14[map].title[0] == TITLE_UNDERGROUND)
    {
        FUN_100135b0();
        DAT_104c00bf = 1;
    }
    if (DAT_104c0084 != 0)
        DAT_104c0084->ClearFlags(4);
    if (DAT_10874848.m_95e14[map].title[0] == TITLE_WATER)
        FUN_10011fb0(1);
    FUN_1013faf0();
    DAT_104bf8b8 = DAT_10f65504 - 1;
    if (DAT_11b044f0 != 0)
    {
        int noise = rand();
        DAT_104c0270[rand() % NOISE_SLOTS] = noise;
    }
    if (DAT_10c64ad4 == 3)
    {
        DAT_102559f4[0]->matrix = DAT_10db6ee8[0].matrix;
        DAT_102559f4[1]->matrix = DAT_10db6ee8[0].matrix;
        DAT_102559f4[2]->matrix = DAT_10db6ee8[0].matrix;
        DAT_102559f4[3]->matrix = DAT_10db6ee8[0].matrix;
    }
    if (DAT_10255f08 != 0)
        DAT_10255f08->StartProximityPass(DAT_10836ec8.m, &DAT_108392d8, &DAT_10836330, DAT_104bf8a4, 0);
    Camera_Refresh();
    Camera_Refresh();
    Camera_Refresh();
    return 0;
}
