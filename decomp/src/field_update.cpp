// The field mode update (game mode 15, called by the frame step): the mode that runs the map. The first frame builds
// the scene (water regions, cloud shadows, the player model with helmet and drill, the camera, the movie and the
// reset of every field global); every frame runs the engine ticks that are due (input, menus, the item menu, the
// player, the characters, the effects, the camera and the scene frame) and then draws the menu scene. C++ rather
// than extern "C": the function keeps an exception frame for its Matrix4 locals.

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "vec4.h"

#include "matrix4.h"

#include "node.h"

#include "node_data.h"

#include "mesh.h"

#include "anim_model.h"

#include "model_bank.h"

#include "effect_manager.h"

#include "region_view.h"

#include "char_slot.h"

#include "item_info.h"

#include "game.h"

#include "tracker.h"

#include "dinput_min.h"

#include "demo_recording.h"

#include "mmv_player.h"

#include "texture_object.h"

#include "texture_load.h"

#include "char_vars.h"

#include "camera_vars.h"

#include "map_vars.h"

#include "ui_vars.h"

#include "render_vars.h"

#include "game_vars.h"

#include "script_system.h"

#include "script_flag.h"

#include "scene_command.h"

#include "scene_draw_vars.h"

#include "menu_scene.h"

#include "menu_main.h"

#include "menu_save_load.h"

#include "player_frame.h"

#include "path_walk.h"

#include "char_update.h"

#include "field_item_menu.h"

#include "doppler_loops.h"

#include "effect_holds.h"

#include "map_records.h"

#include "depth_sort.h"

#include "value_command.h"

#include "world.h"

#include "dodge_game.h"

#include "minigame_hud.h"

#include "field_update.h"

#include "hop_spit_enemy.h"

#include "map_setup.h"

#include "engine_memory.h"

#include "win_vars.h"

#include "node_collision_vars.h"

#include "action_select_vars.h"

#include "game_load_data.h"

#include "debris.h"

#include "sound_vars.h"

#include "game_main.h"

#include "switch_slots.h"

#include "char_tables.h"

#include "frame_vars.h"

#include "mole_game.h"

#include "script_vars.h"

#include "hud_frame.h"

extern "C" int FUN_1013fa10(int index, int enable, float a, float b, float c, float d);   // sets light `index`
extern "C" int FUN_10021380(int map);                       // Map_SetEntryFlags
extern "C" int FUN_1000f360(int index);                              // 0x1000f360 (face_anim.cpp): face change of a character
extern "C" int FUN_1001e240(void);                          // 0x1001e240 (Map_IconTexture)
extern "C" int FUN_10015710(int index);                     // 0x10015710 (visibility.cpp): 0 = the character is out of view
extern "C" int FUN_101148b0(int index);                     // 0x101148b0 (char_dispatch.cpp): per-kind update of kinds from 100
extern "C" int FUN_1016ac50(const Vec4 *eye, const Vec4 *target, Matrix4 *view, Matrix4 *projection);   // Camera_LookAt
#include "status_panel.h"
extern "C" int FUN_10199dc0(float red, float green, float blue, float density, float nearZ, float farZ);   // projection.cpp: the fog of the map root
#include "field_hud.h"
extern "C" void FUN_100225b0(void);                         // the tracker's window callback of the field
extern "C" int FUN_10017fc0(void);                          // 0x10017fc0 (node_fade.cpp)
extern "C" int FUN_1004d4b0(void);                          // 0x1004d4b0 (region_view.cpp): per-frame region update
extern "C" int FUN_10021c80(void);                          // 0x10021c80 (Sound listener update)
extern "C" int FUN_10058500(void);                          // 0x10058500 (switch_effects.cpp)
extern "C" int FUN_1008fea0(void);                          // 0x1008fea0 (drill_claws.cpp)
extern "C" int FUN_1000cdf0(void);                          // 0x1000cdf0 (Messages_Update)
extern "C" int FUN_1005c680(void);                          // 0x1005c680 (Bgm_Update)
extern "C" int FUN_1003a940(void);                          // 0x1003a940
extern "C" int FUN_1015f4e0(void);                          // 0x1015f4e0 (TextInput_Update)
#include "field_item_menu.h"

// Game mode of this update.
#define FIELD_SLOT_COUNT 3                 // menu windows the game can have open at once
#define WINDOW_ID_SAVE_MENU 0x22c
#define WINDOW_ID_CHOICE_MENU 0x218
#define WINDOW_ID_YES_NO 0x216
#define SOUND_EVENT_MENU_CLOSE 2
#define HUD_TEXTURE_SLOT_COUNT 15
#define REGION_RECORD_SIZE 0x44
#define NODE_ALLOC_SIZE 0x18a0
#define PLAYBACK_ALLOC_SIZE 0x7c84
#define MAX_CATCH_UP_TICKS 20
#define CLOUD_COUNT 3
#define CLOUD_START_X 5800.0f
#define CLOUD_RESET_X -5000.0f
#define CLOUD_RANGE 6000
#define MAP_NEAR_DISTANCE2 20250000.0f
#define NEAR_CULL_DISTANCE2_PLAYER 40000.0f
#define NEAR_CULL_DISTANCE2_OTHER 62500.0f
#define OBJECT_VISIBLE_X -10000.0f
#define SCENE_GHOME_COMMAND 0x753e
#define DEBRIS_SCENE_FIRST 0x7d1
#define PROMPT_NEAR_DISTANCE2 25000.0f
#define PROMPT_SOUND 0x356
#define VALUE_COMMAND_FIELD_FADE 0x18695
#define STAMP_UNUSED 0
#define SCOPE_EFFECT_OWNER 999
#define CAMERA_BACK_DISTANCE 1700.0f
#define CAMERA_SWING_DIVISOR -5000.0f
#define CAMERA_SWING_ANGLE 0.2618f
#define COSTUME_SCOPE 6
#define CLOUD_HEIGHT 500.0f
#define CLOUD_SPACING 3600.0f
#define CLOUD_MAX_Y 3000.0f
#define CLOUD_SPEED 3.0f
#define HELMET_HIDES_FACE 0x2a
#define PLAYER_BOUNDS_RADIUS 100.0f
#define FADE_STEP -6.6666665f
#define FOG_FAR_DISTANCE 100000.0f
#define BACKGROUND_SCROLL_LAST 0xdc
#define SECOND_MODEL 75
#define MOVIE_LAST_FRAME 0x2f
#define SHAFT_LIMIT 2500.0f
#define SHAFT_FADE 0.05f
#define SHAFT_SPEED 20.0f
#define KEY_PROMPT_SLOTS 124
#define DEBRIS_SCENE_RESET 0x7d0
#define DEBRIS_STATE_FIRST 0x7da
#define DEBRIS_STATE_LAST 0x80c
#define DEBRIS_PICKUP_LAST 0x807
#define HALF_TURN 1.5708f
#define PROMPT_FRAMES 2
#define PROMPT_CELL_MARKER 15
#define PROMPT_CELL_TALK 6
#define PROMPT_MODEL_COUNT 58
#define PROMPT_SKIPPED_MODEL 14
#define ARROW_FRAMES 15
#define GUIDE_SLOT 16
#define CHAR_KIND_SCRIPTED 100
#define DODGE_STATE_BLOCK_SIZE 0xc0
#define WATER_NODE_FLAG 4

// The effect that follows the costume's helmet: `scopeEffect` is the variant used with the scope nodes (costume 6).
static __forceinline int HelmetEffect(int kind, int scopeEffect)
{
    if (kind == 0x1a)
        return scopeEffect ? 0x859a : 0x8598;
    if (kind == 0x1c)
        return 0x85fc;
    if (kind == 0x1d)
        return 0x8660;
    return scopeEffect ? 0x8536 : 0x8534;
}

// Shows one home ghost pair: the "-1" ghost while `state` is 2, otherwise the "-2" one.
static __forceinline void SetHomeGhostPair(int state, const char *first, const char *second)
{
    if (state == 2)
    {
        Scene_RunCommand(SCENE_GHOME_COMMAND, 1, first);
        Scene_RunCommand(SCENE_GHOME_COMMAND, 0, second);
    }
    else
    {
        Scene_RunCommand(SCENE_GHOME_COMMAND, 0, first);
        Scene_RunCommand(SCENE_GHOME_COMMAND, 1, second);
    }
}

// Closes the menu window `id` when it is open.
static __forceinline void CloseWindowIfOpen(int id)
{
    int slot = DAT_104b7200.FindSlot(id);
    if (slot != FIELD_SLOT_COUNT)
    {
        DAT_104be860(SOUND_EVENT_MENU_CLOSE);
        DAT_104b7200.m_0110[slot] = 0;
    }
}

// Shows the key prompt over the player: cell `cell` of the prompt sheet for PROMPT_FRAMES frames.
static __forceinline void ShowPlayerPrompt(int cell)
{
    if (DAT_104bf8f8[0] == 0 || DAT_1021d138[0] != cell)
    {
        DAT_104bf8f8[0] = 1;
        DAT_1021d138[0] = cell;
    }
    DAT_1021f740[0] = PROMPT_FRAMES;
}

// The held item is drawn at the player's left hand while the scene state DAT_104c00d0 is in the debris range, and
// the pick-up effect starts at the hand (before 0x807).
static __forceinline void DrawHeldItem()
{
    ItemInfo &item = DAT_1088e6b0[DAT_104c0198];
    if (item.previewScale == 0)
        item.previewScale = 100;
    float scale = ((float)(item.previewScale / 2 + 0x32) * 1.5f) / 100.0f;
    Matrix4 world;
    Node *hand = DAT_104c039c->Find("Bip01 L Hand", 0);
    world = DAT_1086df38;
    world = (Scaling(scale, scale, scale) * RotationZ((float)DAT_104c00d0 * 0.1f - HALF_TURN)) * world;
    if (hand != 0)
        world.SetTranslation(hand->worldMatrix.GetTranslation());
    world._43 = world._43 + 20.0f;
    DAT_104d2a08[DAT_104c0198].StartProximityPass(world.m, &DAT_108392d8, &DAT_108392d8, 0, 0);
    DAT_1086e0b8[0].endA = world;
    Vec4 origin;
    origin.Fill(0.0f);
    Matrix4 attach;
    attach = DAT_1086df38;
    if (hand != 0)
        attach.SetTranslation(hand->worldMatrix.GetTranslation());
    if (DAT_104c00d0 < DEBRIS_PICKUP_LAST)
        DAT_10daaa80.Start(0x8f02, origin, -1, 0, 0, 0, 0, &attach, 0, 0);
}

// The prompt arrow of the field: it shows while the player stands at a marker of the map or near a character,
// DAT_1020c2f4 counts its frames up to 15 and down to 0.
static __forceinline void UpdatePromptArrow()
{
    int i;
    if (DAT_104bfe1c == 0 && DAT_1032d69c != 0 && DAT_102028dc == 0 && DAT_104bfd28 == 0)
    {
        if (DAT_104bfb08[0] != 0)
            goto clearPromptEffect;
        if (DAT_1020c304 != 0)
            goto clearEffectNeeded;
        if (DAT_10210fc0[DAT_10c64a5c].entry.id > -1)
        {
            ShowPlayerPrompt(PROMPT_CELL_MARKER);
            if (DAT_1020c2ec != 5 && DAT_10c63508 == 0 && DAT_10c66d08 == 0)
                FUN_1003ad20(PROMPT_SOUND, 0, 0);
            DAT_1020c2ec = 5;
            DAT_1020c2f8 = 1;
            DAT_10c63508 = 0;
            goto arrowStep;
        }
        DAT_1020c33c = 0;
        DAT_1020c2f8 = 0;
        for (i = 1; i < PROMPT_MODEL_COUNT; i++)
        {
            if (i == PROMPT_SKIPPED_MODEL)
                continue;
            Vec4 offset = DAT_10db6ee8[i].matrix.GetTranslation() - DAT_10db6ee8[0].matrix.GetTranslation();
            if (offset.LengthSquared3() < PROMPT_NEAR_DISTANCE2)
            {
                ShowPlayerPrompt(PROMPT_CELL_TALK);
                if (DAT_1020c2ec != 5 && DAT_10c63508 == 0)
                    FUN_1003ad20(PROMPT_SOUND, 0, 0);
                DAT_1020c2ec = 5;
                DAT_1020c33c = i;
            }
        }
        if (DAT_1020c33c != 0)
        {
            if (DAT_1020c2f8 != 0)
                goto arrowStep;
            goto arrowCheck;
        }
        if (DAT_104bfb08[0] == 0 && DAT_10c63508 != 0 && DAT_1032d69c != 0)
            DAT_10c63508 = 0;
        DAT_1020c2ec = 0;
        DAT_1020c2f8 = 0;
        goto arrowFade;
    }
    if (DAT_104bfb08[0] == 0)
    {
    clearEffectNeeded:
        if (DAT_10c63508 != 0 && DAT_1032d69c != 0)
            DAT_10c63508 = 0;
    }
    if (DAT_104bfe1c == 0)
    {
    clearPromptEffect:
        DAT_1020c2ec = 0;
    }
    DAT_1020c2f8 = 0;
arrowCheck:
    if (DAT_1020c33c == 0)
        goto arrowFade;
arrowStep:
    if (DAT_1020c2f4 < ARROW_FRAMES)
    {
        DAT_1020c2f4 = DAT_1020c2f4 + 1;
        DAT_1020c340 = DAT_1020c33c != 0;
    }
    else if (DAT_1020c2f8 == 0 && DAT_1020c33c == 0)
        goto arrowFade;
    return;
arrowFade:
    if (DAT_1020c2f4 > 0)
        DAT_1020c2f4 = DAT_1020c2f4 - 1;
}

// The banner and the preview fade: states 1 (fading in) and 2 (fading out) of the two banners.
static __forceinline void UpdateBanners()
{
    if (DAT_104bf8c8 != 0)
    {
        DAT_104bf8d0 = DAT_104bf8d0 + 1;
        if (DAT_104bfd4c < DAT_104bf8d0)
        {
            DAT_104bf8c8 = 2;
            if (DAT_104bf8cc > 0)
                DAT_104bf8cc = DAT_104bf8cc - 1;
        }
        else if (DAT_104bf8c8 == 1)
        {
            if (DAT_104bf8cc < 10)
                DAT_104bf8cc = DAT_104bf8cc + 1;
        }
        else if (DAT_104bf8c8 == 2)
        {
            if (DAT_104bf8cc > 0)
                DAT_104bf8cc = DAT_104bf8cc - 1;
        }
        if (DAT_104bf8cc == 0)
            DAT_104bf8c8 = 0;
    }
    if (DAT_104bf8d4 != 0)
    {
        if (DAT_104bf8d4 == 1)
        {
            if (DAT_104bf8d8 < 0xf)
                DAT_104bf8d8 = DAT_104bf8d8 + 1;
        }
        else if (DAT_104bf8d4 == 2 && DAT_104bf8d8 > 0)
            DAT_104bf8d8 = DAT_104bf8d8 - 1;
        if (DAT_104bf8d8 == 0)
            DAT_104bf8d4 = 0;
    }
}

// Starts the scope effect of the drill at the node `tip` (the effects of costume 6 use three nodes and the options).
static __forceinline void StartScopeEffect(int effect, Node *tip, const Vec4 &position, int withOptions)
{
    Matrix4 attach;
    attach.SetIdentity();
    if (tip != 0)
        attach.SetTranslation(tip->worldMatrix.GetTranslation());
    if (withOptions != 0)
    {
        EffectOptions options;
        options.flags = 1;
        options.m_0004 = SCOPE_EFFECT_OWNER;
        DAT_10daaa80.Start(effect, position, -1, 0, 0, 0, 0, &attach, &options, 0);
    }
    else
        DAT_10daaa80.Start(effect, position, -1, 0, 0, 0, 0, &attach, 0, 0);
}

// Loads one texture, `.iti` files as iti, the others as bitmaps.

// The camera follows the player: it sits behind and above `target` and swings with the player's x.
static __forceinline void PlaceStartCamera(const Vec4 &target, float scale)
{
    Vec4 offset;
    Vec4 sum;
    DAT_11af3e28.Set(scale * 0.0f + target.x, target.y - scale * CAMERA_BACK_DISTANCE,
                     target.z + scale * CAMERA_BACK_DISTANCE, 0);
    DAT_118e6c78.Set(target.x, target.y, target.z, 0);
    offset.Copy(DAT_11af3e28 - DAT_118e6c78);
    offset.RotateZ((target.x / CAMERA_SWING_DIVISOR) * CAMERA_SWING_ANGLE);
    sum.Copy(offset + DAT_118e6c78);
    DAT_11af3e28.Copy(sum);
    DAT_118e69e8.Copy(DAT_11af3e28);
    DAT_118e6cb8.Copy(DAT_118e6c78);
}

// 0x1011a060 (Field_Update). Game mode 15: the field. On the first frame (DAT_104bf8bc = 0) it releases the HUD
// textures, loads the haze of the water regions and the three cloud shadows, builds the water regions of the map,
// the player model with its helmet, drill and motions and the mini map root, places the camera behind the player,
// resets every field global, opens the map movie, queues the first events of a saved game and sets the four home
// ghosts. Every frame it then runs the engine ticks that are due (one when replaying a recording or when more than
// 20 are pending): the input and its recording, the field item menu, the menu windows, the light shafts, the scope
// effects, the animation of the player and of every character in range, the clouds, the sounds, the face changes,
// the prompts and the banners. Last it builds the camera matrices, updates the map, the effects and the lights and
// draws the menu scene. Returns 0.
int Field_Update()
{
    Vec4 startPosition;
    int i;
    int closeOverlay = 0;
    if (DAT_104bfd84 != 0)
        return 0;
    if (DAT_104bf8bc == 0)
    {
        DAT_10c64f10 = 0;
        for (i = 0; i < HUD_TEXTURE_SLOT_COUNT; i++)
        {
            if (DAT_10202808[i] > -1)
                FUN_10174c70(DAT_10202808[i], 0);
            DAT_10202808[i] = -1;
        }
        DAT_102028e4 = LoadTexture("moyabase.iti", 0);
        if (DAT_104c03c0 != 0)
        {
            for (i = 0; i < DAT_102564a0; i++)
            {
                DAT_11b1ba40(DAT_104c03c0[i].vertices);
                DAT_11b1ba40(DAT_104c03c0[i].points);
                DAT_11b1ba40(DAT_104c03c0[i].buffer0004);
                DAT_104c03c0[i].vertices = 0;
                DAT_104c03c0[i].points = 0;
                DAT_104c03c0[i].buffer0004 = 0;
            }
            DAT_11b1ba40(DAT_104c03c0);
            DAT_104c03c0 = 0;
        }
        if (DAT_10255f08 != 0)
        {
            Node *water = DAT_10255f08->Find("moya", 0);
            DAT_104c03c4 = 0;
            if (water != 0)
            {
                water->ClearFlags(WATER_NODE_FLAG);
                DAT_102564a0 = water->packCount;
                DAT_104c03c0 = (MapRegion *)DAT_11b0b938(water->packCount * REGION_RECORD_SIZE, "tsm_moyavpac");
                memset(DAT_104c03c0, 0, water->packCount * REGION_RECORD_SIZE);
                for (i = 0; i < DAT_102564a0; i++)
                {
                    DAT_104c03c0[i].vertices = (RegionVertex *)DAT_11b0b938(water->packs[i].vertexCount * sizeof(RegionVertex), "tsm_moyavertex[i]");
                    DAT_104c03c0[i].points = (RegionPoint *)DAT_11b0b938(water->packs[i].vertexCount * sizeof(RegionPoint), "tsm_moyasenixyz[i]");
                    memset(DAT_104c03c0[i].vertices, 0, water->packs[i].vertexCount * sizeof(RegionVertex));
                    memset(DAT_104c03c0[i].points, 0, water->packs[i].vertexCount * sizeof(RegionPoint));
                    DAT_104c03c0[i].pointCount = water->packs[i].vertexCount;
                    for (int v = 0; v < water->packs[i].vertexCount; v++)
                        memcpy(&DAT_104c03c0[i].vertices[v], &water->packs[i].vertices[v], sizeof(RegionVertex));
                }
                for (i = 0; i < DAT_102564a0; i++)
                {
                    DAT_104c03c0[i].buffer0004 = DAT_11b0b938(water->indexLists[i].count * 2, "tsm_moyaindex[i]");
                    memset(DAT_104c03c0[i].buffer0004, 0, water->indexLists[i].count * 2);
                    DAT_104c03c0[i].indexCount = water->indexLists[i].count;
                    for (int k = 0; k < water->indexLists[i].count; k++)
                        ((short *)DAT_104c03c0[i].buffer0004)[k] = (short)water->indexLists[i].indices[k];
                }
            }
        }
        DAT_104c00c8 = 0;
        if (DAT_10db6ee8[0].root != 0)
        {
            Node *oldScope = DAT_10db6ee8[0].root->Find("scop", 0);
            if (oldScope != 0)
                DAT_10db6ee8[0].root->RemoveChild(oldScope, 0);
            if (DAT_10db6ee8[0].root == 0)
            {
                DAT_104c00f8 = 0;
                DAT_104c00fc = 0;
                DAT_104c0100 = 0;
                DAT_104c0104 = 0;
            }
            else
            {
                DAT_104c00f8 = DAT_10db6ee8[0].root->Find("scop-pt", 0);
                if (DAT_10db6ee8[0].root == 0)
                    DAT_104c00fc = 0;
                else
                    DAT_104c00fc = DAT_10db6ee8[0].root->Find("charbase0", 0);
                DAT_104c0100 = 0;
                DAT_104c0104 = 0;
                if (DAT_10c655f0 == COSTUME_SCOPE)
                {
                    DAT_104c0100 = DAT_10db6ee8[0].root->Find("scop-2pt", 0);
                    DAT_104c0104 = DAT_10db6ee8[0].root->Find("scop-3pt", 0);
                }
            }
        }
        DAT_102028dc = 0;
        memset(&DAT_1020c2b0, 0, DODGE_STATE_BLOCK_SIZE);
        DAT_10e3dec0 = 0;
        DAT_10e3debc = 0;
        DAT_1020c320 = LoadTexture("kumo001a.iti", 0);
        DAT_1020c324 = LoadTexture("kumo002a.iti", 0);
        DAT_1020c328 = LoadTexture("kumo003a.iti", 0);
        DAT_1020c2fc = -1;
        DAT_1020c32c = LoadTexture("waku04.bmp", 0);
        DAT_10803ab8[0].x = -(float)(FUN_1013f880() % CLOUD_RANGE);
        DAT_10803ab8[0].z = CLOUD_HEIGHT;
        DAT_10803ab8[0].y = (float)(FUN_1013f880() % CLOUD_RANGE) - CLOUD_RANGE;
        DAT_10803ab8[1].x = DAT_10803ab8[0].x + CLOUD_SPACING;
        DAT_10803ab8[1].z = CLOUD_HEIGHT;
        DAT_10803ab8[1].y = (float)(FUN_1013f880() % CLOUD_RANGE) - CLOUD_RANGE;
        DAT_10803ab8[4].x = DAT_10803ab8[0].x + CLOUD_SPACING * 2;
        DAT_10803ab8[4].z = CLOUD_HEIGHT;
        DAT_10803ab8[4].y = (float)(FUN_1013f880() % CLOUD_RANGE) - CLOUD_RANGE;
        if (DAT_10255f08 != 0)
        {
            DAT_1021c624 = DAT_10255f08->Find("@kumo1", 0);
            DAT_1021c628 = DAT_10255f08->Find("@kumo2", 0);
            DAT_1021c62c = DAT_10255f08->Find("@kumo3", 0);
        }
        if (DAT_104c039c != 0)
        {
            DAT_104c039c->Release();
            DAT_11b1ba40(DAT_104c039c);
            DAT_104c039c = 0;
            DAT_11b1ba40(DAT_104c03a0);
            DAT_104c03a0 = 0;
            DAT_104c03a8->Release();
            DAT_11b1ba40(DAT_104c03a8);
            DAT_104c03a8 = 0;
            DAT_104c03ac = 0;
        }
        {
            DAT_104c039c = (Node *)DAT_11b0b938(NODE_ALLOC_SIZE, "tsm_lpminiFChar");
            DAT_104c039c->Init();
            char modelName[0x100];
            const char *modelFormat = "%s%d.it3";
            if (DAT_10c655f0 == 0xb)
                modelFormat = "%sa1.it3";
            else if (DAT_10c655f0 == 0xc)
                modelFormat = "%sa2.it3";
            sprintf(modelName, modelFormat, DAT_10daaae8[0].m_0165, DAT_10c655f0);
            DAT_104c039c->LoadIt3(FUN_1013f8a0(modelName, 0), 0, 0);
            DAT_104c039c->Rebuild();
            DAT_104c039c->BuildMesh(1, 0);
            Node *hand = DAT_104c039c->Find("Bip01 R Hand", 0);
            Node *drill;
            if (hand != 0 && (drill = hand->AddChild(0, 0x40)) != 0)
            {
                const char *drillFile;
                if (DAT_10c655f0 == COSTUME_SCOPE)
                    drillFile = "spscop.it3";
                else if (DAT_10c6566c == 1)
                    drillFile = "drill5.it3";
                else if (DAT_10c64ad4 == 0)
                    drillFile = "char02-d.it3";
                else if (DAT_10c64ad4 == 1)
                    drillFile = "drill2.it3";
                else if (DAT_10c64ad4 == 2)
                    drillFile = "drill3.it3";
                else
                    drillFile = "drill4.it3";
                drill->Load(FUN_1013f8a0(drillFile, 0), 0, 0, 0);
                strcpy(drill->name, "scop");
                drill->parent = hand;
                drill->BuildMesh(1, 0);
                DAT_104c039c->PreScanFile(FUN_1013f8a0("motionzD.it3", 0), 0);
                DAT_1021c630 = DAT_104c039c->Find("scop-pt", 0);
                if (DAT_10c655f0 == COSTUME_SCOPE)
                {
                    DAT_1021c638 = DAT_104c039c->Find("scop-2pt", 0);
                    DAT_1021c63c = DAT_104c039c->Find("scop-3pt", 0);
                }
            }
            int helmet = DAT_10daaae8[0].helmet;
            Node *oldHelmet = DAT_104c039c->Find("helmet", 0);
            if (oldHelmet != 0)
                DAT_104c039c->RemoveChild(oldHelmet, 0);
            Node *head = DAT_104c039c->Find("helmet", 0);
            if (head == 0 && (head = DAT_104c039c->Find("Bip01 Head", 0)) != 0)
            {
                Node *helmetNode = head->AddChild(0, 0x40);
                helmetNode->Load(FUN_1013f8a0(DAT_108f678c[helmet], 0), 0, 0, 0);
                strcpy(helmetNode->name, "helmet");
                helmetNode->parent = head;
                helmetNode->Rebuild();
                helmetNode->BuildMesh(1, 0);
            }
            Node *face = DAT_104c039c->Find("face_char02bf.lwo", 0);
            if (helmet == HELMET_HIDES_FACE)
            {
                if (face != 0)
                    face->flags |= NODE_FLAG_HIDDEN;
            }
            else if (face != 0)
                face->flags &= ~NODE_FLAG_HIDDEN;
            DAT_104c039c->bounds.w = PLAYER_BOUNDS_RADIUS;
            DAT_104c039c->PreScanFile(FUN_1013f8a0("motionzP.it3", 0), 0);
            DAT_104c03a0 = (Playback *)DAT_11b0b938(PLAYBACK_ALLOC_SIZE, "tsm_ani");
            DAT_104c03a0->Init();
            DAT_104c03a0->BindNode(DAT_104c039c);
            for (i = 0; i < DAT_10b35ff8; i++)
            {
                if (strncmp(DAT_10b3602c[i], "SET", 3) == 0)
                    DAT_104c03a0->SetLayer(DAT_10b3792c[i][0], DAT_10b3792c[i][1], DAT_10b3792c[i][2], DAT_10b3792c[i][3],
                                           DAT_10b3792c[i][4], DAT_10b3792c[i][5]);
                else if (strncmp(DAT_10b3602c[i], "DIS", 3) == 0)
                    DAT_104c03a0->SetLayerParam(DAT_10b3792c[i][0], DAT_10b3792c[i][1], DAT_10b3792c[i][2]);
                else if (strncmp(DAT_10b3602c[i], "RND", 3) == 0)
                    DAT_104c03a0->SetSlotOffset(DAT_10b3792c[i][0], DAT_10b3792c[i][1], DAT_10b3792c[i][2], DAT_10b3792c[i][3]);
                else if (strncmp(DAT_10b3602c[i], "DAT", 3) == 0)
                {
                    if (DAT_10b3792c[i][0] == 4)
                        DAT_104c03a0->SetSlotSource(&DAT_104c02f0, DAT_10b3792c[i][1], DAT_10b3792c[i][0]);
                    else
                        DAT_104c03a0->SetSlotSource(&DAT_10c652cc, DAT_10b3792c[i][1], DAT_10b3792c[i][0]);
                }
            }
            if (DAT_104c00d0 < DEBRIS_SCENE_FIRST)
            {
                int motion;
                if (DAT_1020c304 == 0 || DAT_10c63388 != 0)
                    motion = 2;
                else
                {
                    motion = 1;
                    if (DAT_104c03a0->layer != 1 && DAT_104c03a0->nextLayer != 1)
                        DAT_104c03a0->Play(1, 1, 5, 1.0f);
                }
                DAT_104c03a0->Play(motion, 0, 5, 1.0f);
            }
            else if (DAT_104c00d0 > DEBRIS_SCENE_FIRST + 9)
            {
                if (DAT_104c03a0->layer != 0x59 && DAT_104c03a0->nextLayer != 0x59)
                    DAT_104c03a0->Play(0x59, 1, 5, 1.0f);
                DAT_104c03a0->Play(0x59, 0, 5, 1.0f);
            }
            DAT_104c039c->Rebuild();
            DAT_104c039c->BuildMesh(1, 0);
            DAT_104c039c->M_101944d0();
            Vec4 origin;
            DAT_1086df38.SetIdentity();
            origin.Fill(0.0f);
            DAT_1086df38.SetTranslation(origin);
            DAT_104c03a8 = (Node *)DAT_11b0b938(NODE_ALLOC_SIZE, "tsm_lpminiroot");
            DAT_104c03a8->Init();
            DAT_104c03a8->LoadIt3(FUN_1013f8a0("miniroot.it3", 0), 0, 0);
            DAT_104c03ac = DAT_104c03a8->Find("@maproot", 0);
            DAT_1021bdf8[0][0] = DAT_104c039c->Find("@face_char02bf", 0);
            DAT_1021bdf8[0][1] = DAT_104c039c->Find("@face_char02bf", 0);
            DAT_10db6ee8[0].m_17e8 = 1;
            DAT_10db6ee8[0].alpha = 0;
            DAT_10db6ee8[0].mode = 0;
        }
        DAT_1021c620 = DAT_104c039c->Find("Bip01 Pelvis", 0);
        if (DAT_102028d0 == -1)
            DAT_102028d0 = LoadTexture("info_04.bmp", 0);
        DAT_10804bf8.Add(DAT_104c039c, (const Vec4 *)&DAT_1086df38.translation);
        {
            Matrix4 pathMatrix = DAT_104c03ac->extra->AnimatedMatrix(DAT_10c64a58);
            startPosition.Copy(pathMatrix.GetTranslation());
        }
        DAT_1086df38.SetTranslation(startPosition);
        DAT_1020c314 = DAT_10c64a58;
        DAT_1020344c = 1.0f;
        PlaceStartCamera(startPosition, 1.0f);
        DAT_104687b0 = DAT_10db6ee8[0].m_238c;
        DAT_1020c348 = DAT_10daaae8[0].helmet;
        DAT_102024bc = 0xffff;
        DAT_102024b8 = 0xffff;
        DAT_102024b0 = -1;
        memset(DAT_102550e0, 0, sizeof(DAT_102550e0));
        DAT_104c00b2 = 0;
        DAT_104c00b0 = 0;
        DAT_104c0092 = 0;
        DAT_104c0093 = 0;
        DAT_104c00a0 = 0;
        DAT_104c0097 = 0;
        DAT_104c0094 = 0;
        DAT_1030c478[0] = 0;
        DAT_1049f1f8[0] = 0;
        DAT_1030c479 = 0;
        DAT_1049f1f9 = 0;
        DAT_1030c47a = 0;
        DAT_1049f1fa = 0;
        DAT_1030c47b = 0;
        DAT_1049f1fb = 0;
        DAT_104c00b7 = 0;
        DAT_104c0128 = 0;
        DAT_10f6553c = Hud_DrawFrame;
        DAT_104c010c = 0;
        DAT_10db6ee8[0].m_0075 = 0;
        DAT_104c0068 = 0;
        DAT_104bff00 = 0;
        DAT_102024a8 = 1;
        DAT_104c009e = 0;
        DAT_104c009d = 0;
        DAT_104c0030 = 0;
        DAT_104bfeb9 = 0;
        DAT_1030b868 = 0;
        DAT_104bfd74 = 4;
        DAT_104bfeb0 = 0;
        DAT_104bfe70 = 0;
        DAT_104bfe74 = 0;
        DAT_104c0028 = 0;
        DAT_104c002c = 0;
        DAT_104c00bd = 0;
        DAT_104bfe5c = 0;
        DAT_104bfe30 = 0;
        DAT_104bfdd0 = 0;
        DAT_104bfdcc = 0;
        DAT_104bfddc = 0;
        DAT_104bfdc8 = 0;
        DAT_1020d430 = 0;
        DAT_10468128 = 0;
        DAT_104bfe20 = 0;
        DAT_104bf8bc = 2;
        DAT_104c009c = 0;
        DAT_104bfd78 = 0;
        DAT_104bfd80 = 0;
        DAT_104bfe3c = 0;
        DAT_10db6ee8[0].mode = 1;
        DAT_104bfe7c = 0;
        DAT_104bfe60 = 0;
        DAT_104bfe4c = 0;
        DAT_104bfe50 = 0;
        DAT_104bfe6c = 0;
        memset(DAT_10253e98, 0, sizeof(DAT_10253e98));
        memset(DAT_104686b0, 0, sizeof(DAT_104686b0));
        memset(DAT_10210d30, 0, sizeof(DAT_10210d30));
        memset(DAT_1030c278, 0, sizeof(DAT_1030c278));
        for (i = 1; i < CHAR_SLOT_COUNT; i++)
            DAT_10daaae8[i].stats[STAT_HEALTH] = DAT_10daaae8[i].stats[STAT_MAX_HEALTH];
        DAT_104b7210 = -1;
        memset(DAT_1030bbf8, 0, sizeof(DAT_1030bbf8));
        FUN_1013fa10(0, 0, 1.0f, 1.0f, 1.0f, 1.0f);
        FUN_1013fa10(1, 0, 0.0f, 0.0f, 0.0f, 1.0f);
        DAT_1101ddf4 = 0;
        DAT_104c0138 = GetTickCount();
        FUN_1000f360(0);
        FUN_1013fa10(0, 0, 1.0f, 1.0f, 1.0f, 1.0f);
        FUN_1013fa10(1, 0, 0.0f, 0.0f, 0.0f, 1.0f);
        if (DAT_10c6337c == 0)
            DAT_10da90d0.Set(1.0f, 1.0f, 1.0f, 1.0f);
        else
            DAT_10da90d0.Set(0.0f, 0.0f, 0.0f, 1.0f);
        DAT_1042da68 = 0;
        DAT_10f6553c = FieldHud_Draw;
        PlaceStartCamera(startPosition, DAT_1020344c);
        DAT_1020c2c0 = DAT_10c64a5c;
        DAT_10f65504 = DAT_104bf8b8 + 1;
        DAT_1020c2d0 = DAT_10210fc0[DAT_10c64a5c].camera.movie;
        DAT_104bf8f0 = 100.0f;
        DAT_104bf8e8 = 0;
        DAT_104bf8ec = 0;
        DAT_1032d4b0 = FADE_STEP;
        DAT_104bf8e4 = 0xf;
        DAT_10c63514 = 0;
        DAT_10c6359c = 2;
        DAT_10c635a0 = 0;
        DAT_10c635a4 = 0x10;
        if (DAT_1020c2d0 > -1 && DAT_1020c2d0 < 0x40)
        {
            DAT_10f08ab0.Open(FUN_1013f8a0(DAT_10875e48[DAT_1020c2d0], 0));
            DAT_10f08ab0.M_10161050();
            DAT_10f08ab0.DrawToTexture(0);
            DAT_104bfe74 = DAT_10f08ab0.size;
            DAT_104bfe70 = 1;
            DAT_1020b228 = FUN_1001e240();
        }
        int started = DAT_10c636dc;
        if (DAT_10c657c4 == 0 && DAT_10c636dc != 0)
        {
            DAT_1020247c[DAT_104bfe88] = 0x41b;
            DAT_104bfe88 = DAT_104bfe88 + 1 & 7;
            DAT_10c657c4 = 1;
        }
        if (DAT_10c66d18 == 0 && started != 0)
        {
            DAT_1020247c[DAT_104bfe88] = 0x3ad;
            DAT_104bfe88 = DAT_104bfe88 + 1 & 7;
            DAT_1032d69c = 0;
            DAT_10c63508 = 1;
            DAT_10c66d18 = 1;
            DAT_10c63514 = 0;
            DAT_10c63518 = 1;
        }
        else if (DAT_10c66d18 == 2)
        {
            DAT_10c66d18 = 1;
            DAT_10c63514 = 0;
            DAT_10c63518 = 1;
        }
        if (DAT_10c64700 == 0)
        {
            Scene_RunCommand(SCENE_GHOME_COMMAND, 1, "Ghome1-1");
            Scene_RunCommand(SCENE_GHOME_COMMAND, 1, "Ghome2-1");
            Scene_RunCommand(SCENE_GHOME_COMMAND, 1, "Ghome3-1");
            Scene_RunCommand(SCENE_GHOME_COMMAND, 1, "Ghome4-1");
            Scene_RunCommand(SCENE_GHOME_COMMAND, 0, "Ghome1-2");
            Scene_RunCommand(SCENE_GHOME_COMMAND, 0, "Ghome2-2");
            Scene_RunCommand(SCENE_GHOME_COMMAND, 0, "Ghome3-2");
            Scene_RunCommand(SCENE_GHOME_COMMAND, 0, "Ghome4-2");
        }
        else
        {
            SetHomeGhostPair(DAT_10c64a34, "Ghome3-1", "Ghome3-2");
            SetHomeGhostPair(DAT_10c64a38, "Ghome4-1", "Ghome4-2");
            SetHomeGhostPair(DAT_10c64a3c, "Ghome2-1", "Ghome2-2");
            SetHomeGhostPair(DAT_10c64a40, "Ghome1-1", "Ghome1-2");
        }
        if (DAT_10c655f4 == 0 && (DAT_104c0230 & 8) != 0)
        {
            DAT_10c655f4 = 1;
            if (DAT_10c655f8 == 0)
                DAT_10c655f8 = 1;
        }
        DAT_10db6f68 = 0;
        DAT_10db6ee8[0].mode = 0;
        DAT_104c0135 = 0;
        DAT_1020d9e0.M_10001010((int)DAT_10f6569c, (int)FUN_100225b0, (int)FUN_1013e6e0);
        if (DAT_10255f08 != 0)
        {
            Node *haze = DAT_10255f08->Find("@moya", 0);
            if (haze != 0)
                haze->ClearFlags(WATER_NODE_FLAG);
        }
        if (DAT_10216cc0[0].count != 0)
        {
            DAT_10daaae8[GUIDE_SLOT].kind = -1;
            DAT_1020b830 = 0;
            DAT_1020b834 = 0;
            DAT_1020b838 = 0;
        }
        FUN_10199dc0(0, 0, 0, 0, 0, FOG_FAR_DISTANCE);
        DAT_1020c354 = 0xf;
    }

    DWORD now = GetTickCount();
    if (now - DAT_104c0138 >= 1000)
    {
        unsigned int seconds = (now - DAT_104c0138) / 1000;
        DAT_104c00dc = DAT_104c00dc + seconds;
        DAT_104c0138 = DAT_104c0138 + seconds * 1000;
    }
    if (DAT_1032d69c == 0)
        DAT_1020c354 = 10;
    if (DAT_1065cb90 == 0)
    {
        if (DAT_10f65504 == DAT_104bf8b8)
            goto drawScene;
    }
    else
    {
        DAT_104c0184 = DAT_104c0184 + 2;
        if (DAT_104bf8b8 == DAT_10f65504)
            return 0;
    }
    int tickCount = DAT_10f65504 - DAT_104bf8b8;
    DAT_104bf8b8 = DAT_10f65504;
    if (DAT_1065cb90 != 0)
        DAT_104c0184 = DAT_104c0184 - tickCount;
    if (tickCount > MAX_CATCH_UP_TICKS || DAT_104bf8a0 != 0 || DAT_104bf880 != 0 || tickCount < 1 || DAT_1065cb90 != 0)
        tickCount = 1;
    for (int tick = 0; tick < tickCount; tick++)
    {
        if (DAT_1032d69c != 0 && DAT_104b7200.IsQueuedEventReady() != 0)
        {
            DAT_104bf8b8 = DAT_104bf8b8 + (tick - tickCount);
            if (DAT_104bfe1c != 0)
            {
                DAT_1030c54c = 0;
                DAT_104bfd78 = 0;
                closeOverlay = 1;
            }
            break;
        }
        DAT_104bf8a4 = DAT_104bf8a4 + 1;
        if (DAT_104bf890 % 5 == 0 && (DAT_104c02cc = DAT_104c02cc + 1, DAT_104c02cd + 10 < (unsigned int)DAT_104c02cc))
            DAT_104c02cc = 0;
        DAT_104bf890 = DAT_104bf890 + 1;
        DAT_1032d67c[0] = DAT_1032d67c[0] + 1;
        if (DAT_1032d67c[0] > BACKGROUND_SCROLL_LAST)
            DAT_1032d67c[0] = 0;
        DAT_1020d9e0.M_10001110(0, 0, 0, 0, 0, 0);
        DAT_10f08008.Update();
        DAT_10210f30 = DAT_10f08008.ReadControls(0);
        DAT_1020d9e0.M_10001550();
        DAT_1065cb28.SetButtons(DAT_10210f30);
        DAT_1065cb28.M_1015caa0(DAT_10f08008.ReadControls(1), 0);
        DAT_1065cb28.M_1015caa0(DAT_10f08008.ReadControls(2), 1);
        DAT_1065cb28.M_1015caa0(DAT_1032d6a0, 2);
        DAT_1065cb28.M_1015caa0(DAT_10f65558, 3);
        DAT_1065cb28.M_1015caa0(DAT_104bf890, 4);
        DAT_1065cb28.M_1015cac0(DAT_10db6ee8[0].matrix._41, 0);
        DAT_1065cb28.M_1015cac0(DAT_10db6ee8[0].matrix._42, 1);
        DAT_1065cb28.M_1015cac0(DAT_10db6ee8[0].matrix._43, 2);
        DAT_1065cb28.M_1015cac0(DAT_10db6ee8[SECOND_MODEL].matrix._41, 3);
        DAT_1065cb28.M_1015cac0(DAT_10db6ee8[SECOND_MODEL].matrix._42, 4);
        DAT_1065cb28.M_1015cac0(DAT_10db6ee8[SECOND_MODEL].matrix._43, 5);
        DAT_1065cb28.M_1015cac0(DAT_118e6cb8.x, 6);
        DAT_1065cb28.M_1015cac0(DAT_118e6cb8.y, 7);
        DAT_1065cb28.M_1015cac0(DAT_118e6cb8.z, 8);
        if (DAT_1065cb94 != 0 && DAT_1065cb90 != 0)
            DAT_1065cb28.RecordFrame();
        if (DAT_1065cb98 != 0 && DAT_1065cb90 != 0)
        {
            DAT_1065cb28.PlayFrame();
            DAT_1032d6a0 = DAT_1065cb28.values_now[2];
            if (DAT_104bf890 != DAT_1065cb28.values_now[4] || DAT_10f65558 != DAT_1065cb28.values_now[3]
                || DAT_10db6ee8[0].matrix._41 != DAT_1065cb28.floats_now[0]
                || DAT_10db6ee8[0].matrix._42 != DAT_1065cb28.floats_now[1]
                || DAT_10db6ee8[0].matrix._43 != DAT_1065cb28.floats_now[2]
                || DAT_10db6ee8[SECOND_MODEL].matrix._41 != DAT_1065cb28.floats_now[3]
                || DAT_10db6ee8[SECOND_MODEL].matrix._42 != DAT_1065cb28.floats_now[4]
                || DAT_10db6ee8[SECOND_MODEL].matrix._43 != DAT_1065cb28.floats_now[5]
                || DAT_118e6cb8.x != DAT_1065cb28.floats_now[6] || DAT_118e6cb8.y != DAT_1065cb28.floats_now[7]
                || DAT_118e6cb8.z != DAT_1065cb28.floats_now[8])
            {
                Sleep(60000);
                fread(0, 1, 1, 0);
                DAT_1065cb90 = 0;
            }
        }
        DAT_104b7200.MoveCursor();
        int itemMenuBusy = 0;
        if (tick == tickCount - 1)
        {
            DAT_1020c2dc = DAT_1065cb28.pressed[13];
            if (DAT_1020c2dc == 0)
                DAT_1020c2e8 = 0;
            else
                DAT_1020c2e8 = DAT_1020c2e8 + 1;
            FUN_1004d4b0();
        }
        if (DAT_104bfe6c == 0 && DAT_1032d69c != 0 && DAT_104bfd08 == 0)
        {
            itemMenuBusy = FieldItemMenu_Update();
            if (DAT_1020c2e8 != 0)
                DAT_1020c2e8 = DAT_1020c2e8 + 1;
        }
        else
            DAT_1020c354 = 10;
        if (DAT_102028dc > 0 && DAT_102028dc != 0x14)
            DAT_102028dc = DAT_102028dc + 1;
        if (DAT_102028dc > 0x28)
        {
            DAT_10c63518 = 0;
            DAT_102028dc = 0;
        }
        if (DAT_104bfdf0 != 0 && DAT_104bfdf0 < 1000)
        {
            DAT_104c03a4 = DAT_104c03a4 + 1;
            DAT_104bfdf0 = DAT_104bfdf0 - 1;
            if (DAT_104bfdf0 == 0)
            {
                DAT_104c0319 = 0;
                DAT_104c039c->PreScanFile(FUN_1013f8a0("motionzP.it3", 0), 0);
                DAT_1020c344 = 0;
                DAT_104c03a4 = 0;
                DAT_104c03a0->Play(2, 1, 0, 1.0f);
                DAT_104c03a0->frame = DAT_104c03a0->m_02ec[2];
                DAT_104c03a0->m_0004 = (float)DAT_104c03a0->m_02ec[2];
                DAT_104c03a0->layer = 2;
                DAT_104c03a0->Play(2, 1, 5, 1.0f);
            }
        }
        if (itemMenuBusy == 0)
            DAT_104b7200.UpdateWindows();
        if (FUN_10064630() == 0 && FUN_10112140() != 0)
            return 0;
        if (DAT_104bfd84 != 0)
            return 0;
        if ((DAT_104bf890 & 1) != 0 && DAT_10255f08 != 0)
            DAT_10255f08->M_10194ee0();
        if ((DAT_102028dc != 0 || DAT_104bfd28 != 0 || DAT_104c03c8 != 0) && DAT_104bfb08[0] != 0)
        {
            DAT_104bfb08[0] = 2;
            DAT_10467c20[0] = 0;
        }
        DAT_10f65548 = 0;
        DAT_10f6554c = 0;

        // The two light shafts slide across the screen and fade in and out.
        for (i = 0; i < 2; i++)
        {
            Vec4 &shaft = DAT_10f65848[i];
            if (shaft.x > SHAFT_LIMIT || shaft.y > SHAFT_LIMIT)
            {
                shaft.w = shaft.w - SHAFT_FADE;
                if (shaft.w <= 0.0f)
                {
                    shaft.w = 0.0f;
                    shaft.x = (float)(-FUN_1013f880() % 1000 - 1500);
                    shaft.y = (float)(-FUN_1013f880() % 1000 - 1500);
                }
            }
            else
            {
                shaft.x = shaft.x + SHAFT_SPEED;
                shaft.y = shaft.y + SHAFT_SPEED;
                shaft.w = shaft.w + SHAFT_FADE;
                if (shaft.w >= 1.0f)
                    shaft.w = 1.0f;
            }
        }
        DAT_10f65368 = (float)sin((double)((float)DAT_104bf890 * 0.1f)) * 0.3f;
        if (DAT_104c0232 != 0)
            DAT_104c0232 = DAT_104c0232 - 1;

        // The scope effects follow the nodes of the drill.
        if (DAT_10c63384 == 0)
        {
            int kind = DAT_1088e6b0[DAT_10daaae8[0].costume].helmetKind;
            Vec4 origin;
            if (DAT_10c655f0 == COSTUME_SCOPE)
            {
                StartScopeEffect(HelmetEffect(kind, 1), DAT_1021c630, origin, 1);
                if (DAT_1021c638 != 0)
                    StartScopeEffect(HelmetEffect(kind, 1), DAT_1021c638, origin, 1);
                if (DAT_1021c63c != 0)
                    StartScopeEffect(HelmetEffect(kind, 1), DAT_1021c63c, origin, 1);
            }
            else
                StartScopeEffect(HelmetEffect(kind, 0), DAT_1021c630, origin, 0);
        }
        if (DAT_104bfe70 != 0)
        {
            if (DAT_104bfe70 != MOVIE_LAST_FRAME)
                DAT_10f08ab0.M_10161050();
            DAT_10f08ab0.DrawToTexture(0);
            int nextFrame = DAT_104bfe70;
            if (nextFrame != MOVIE_LAST_FRAME)
            {
                nextFrame = DAT_104bfe70 + 1;
                if (nextFrame != MOVIE_LAST_FRAME)
                    nextFrame = DAT_104bfe70 + 2;
            }
            DAT_104bfe70 = nextFrame;
            if (DAT_10f08ab0.m_004b == 0)
                DAT_104bfe70 = 0;
        }
        for (i = 0; i < DAT_1020b22c; i++)
        {
            EffectHold &hold = DAT_1020dcf8[i];
            hold.age = hold.age + 1;
            if (hold.m_0010 < hold.age)
            {
                hold.age = 0;
                Vec4 where((float)hold.position[0], (float)hold.position[1], (float)hold.position[2], 0);
                DAT_10daaa80.Start(hold.effect, where, -1, -1, 0, 0, 0, 0, 0, 0);
            }
        }

        // The camera and the matrices of the frame.
        Matrix4 parent;
        Matrix4 viewProjection;
        Matrix4 projection;
        Matrix4 view;
        parent.SetIdentity();
        FUN_1016ac50(&DAT_118e69e8, &DAT_118e6cb8, &view, &projection);
        viewProjection = view * projection;
        DAT_10866da8 = projection;
        DAT_10836ec8 = parent;
        DAT_108392d8 = viewProjection;
        DAT_10836cd8 = view * DAT_104c03f8;
        if (DAT_1020c344 == 0)
        {
            DAT_104c03a0->Advance();
            DAT_104c03a4 = DAT_104c03a0->frame;
        }
        else
        {
            DAT_104c03a0->frame = DAT_104c03a4;
            DAT_104c03a0->m_0004 = (float)DAT_104c03a4;
        }
        Player_UpdateFrame();
        DAT_104c039c->StartProximityPass(DAT_1086df38.m, &DAT_108392d8, 0, DAT_104c03a4, 0);
        DAT_10db6ee8[0].m_0294 = 1;
        if (DAT_104c03a0->blending == 0)
            DAT_10db6ee8[0].motionFrame = DAT_104c03a4;
        int visibleCount = 0;
        float distances[CHAR_SLOT_COUNT];
        DAT_10db6ee8[0].UpdateFrame(&DAT_108392d8, 1, 0, 0, 0);
        if (DAT_10c63384 == 0)
        {
            for (i = 1; i < CHAR_SLOT_COUNT; i++)
            {
                AnimModel &model = DAT_10db6ee8[i];
                if (model.root != 0 && model.matrix._41 > OBJECT_VISIBLE_X)
                {
                    model.UpdateFrame(&DAT_108392d8, 1, 1, DAT_104bf8a4, 0);
                    if (DAT_104bfe6c == 0)
                    {
                        Vec4 at = model.matrix.GetTranslation();
                        Vec4 eyeAt = model.matrix.GetTranslation();
                        float farDistance = i == 0 ? NEAR_CULL_DISTANCE2_PLAYER : NEAR_CULL_DISTANCE2_OTHER;
                        if (at.DistanceSquared(DAT_118e6cb8) <= MAP_NEAR_DISTANCE2
                            && eyeAt.DistanceSquared(DAT_118e69e8) >= farDistance && (i != 0 || FUN_10015710(i) != 0))
                            model.m_2378 = 0;
                        else
                            model.m_2378 = 1;
                    }
                    else
                        model.m_2378 = 0;
                    DAT_10315140[visibleCount] = i;
                    Vec4 position = model.matrix.GetTranslation();
                    distances[visibleCount] = position.DistanceSquared(DAT_118e69e8);
                    visibleCount++;
                    if (DAT_10daaae8[i].kind < CHAR_KIND_SCRIPTED)
                        Char_UpdateByKind(i);
                    else
                        FUN_101148b0(i);
                }
            }
            DAT_104c0054 = visibleCount;
            FUN_10165020(distances, 0, 0, DAT_10315140, visibleCount);
        }
        else
            DAT_104c0054 = 0;

        // The player's place on the screen, and the three clouds.
        Vec4 screen = Project(DAT_1086df38.GetTranslation(), DAT_108392d8);
        screen.x = (screen.x + 1.0f) * 320.0f;
        screen.y = (screen.y - 1.0f) * -240.0f;
        DAT_1020c318 = (int)screen.x;
        DAT_1020c31c = (int)screen.y;
        Node *kumo[CLOUD_COUNT] = { DAT_1021c624, DAT_1021c628, DAT_1021c62c };
        for (i = 0; i < CLOUD_COUNT; i++)
        {
            Vec4 &cloud = DAT_10803ab8[(i & 1) + (i / 2) * 4];
            cloud.x = cloud.x - CLOUD_SPEED;
            if (cloud.x < CLOUD_RESET_X)
            {
                cloud.x = CLOUD_START_X;
                cloud.y = CLOUD_MAX_Y - (float)(FUN_1013f880() % CLOUD_RANGE);
                cloud.z = CLOUD_HEIGHT;
            }
            kumo[i]->matrix.SetTranslation(cloud);
            kumo[i]->matrix._43 = kumo[i]->matrix._43 + 100.0f;
            kumo[i]->matrix._41 = kumo[i]->matrix._41 - 200.0f;
            kumo[i]->matrix._42 = kumo[i]->matrix._42 - 100.0f;
        }
        DAT_10daaa80.Update(0, tick == tickCount - 1, 1);
        FUN_10021c80();
        FUN_1008fea0();
        FUN_10058500();
        for (i = 0; i < KEY_PROMPT_SLOTS; i++)
        {
            if (DAT_104bf8f8[i] != 0)
            {
                DAT_1021f740[i] = DAT_1021f740[i] - 1;
                if (DAT_1021f740[i] < 1)
                    DAT_104bf8f8[i] = 0;
            }
        }
        FUN_1015f4e0();
        if (FUN_1013f880() % 200 < 2)
            FUN_1000f360(0);
        for (i = 0; i < CHAR_SLOT_COUNT; i++)
        {
            if (DAT_1030c278[i] != 0)
            {
                DAT_1030c278[i] = DAT_1030c278[i] - 1;
                if (DAT_1030c278[i] == 0)
                    FUN_1000f360(i);
            }
        }
        if (DAT_104bf8e4 != 0)
            FUN_1003a9d0();
        FUN_10021380(DAT_10210fc0[DAT_1020c2c0].entry.id);
        if (DAT_104bfd28 != 0)
        {
            if (DAT_104bfd28 == 1)
            {
                DAT_104bfd2c = DAT_104bfd2c + 1;
                if (DAT_104bfd2c > 0xf)
                {
                    DAT_104bfd28 = 2;
                    DAT_1030c568 = 1;
                }
            }
            else if (DAT_104bfd28 == 3)
            {
                DAT_104bfd2c = DAT_104bfd2c - 1;
                if (DAT_104bfd2c == 0)
                    DAT_104bfd28 = 0;
            }
        }
        if (DAT_104c00d0 > DEBRIS_SCENE_RESET)
            DAT_104c00d0 = 0;
        else if (DAT_104c00d0 >= DEBRIS_STATE_FIRST && DAT_104c00d0 <= DEBRIS_STATE_LAST)
            DrawHeldItem();
        UpdatePromptArrow();
        UpdateBanners();
        if (DAT_1042da68 != 0)
        {
            DAT_1042da68 = DAT_1042da68 - 1;
            DAT_10da90d0.Copy(DAT_10da90d0 + DAT_10daa800);
            if (DAT_1042da68 == 0)
                DAT_10da90d0.Copy(DAT_10c67f60);
        }
        FUN_1000cdf0();
        FUN_1005c680();
        if (DAT_104bff00 == 0)
        {
            if (DAT_104bff0c > 0)
                DAT_104bff0c = DAT_104bff0c - 1;
        }
        else if (DAT_104bff0c < 0xf)
            DAT_104bff0c = DAT_104bff0c + 1;
        FUN_1003a940();
        DAT_104bf88c = DAT_104bf88c + DAT_10202348;
        if (DAT_104bf88c == 10 || DAT_104bf88c == 0)
            DAT_10202348 = -DAT_10202348;
        FUN_1001e4b0(DAT_10c6402c, ScriptFlag(DAT_10c64028) == 0);
        Scene_Frame();
    }
    if (closeOverlay != 0)
    {
        CloseWindowIfOpen(WINDOW_ID_SAVE_MENU);
        CloseWindowIfOpen(WINDOW_ID_CHOICE_MENU);
        CloseWindowIfOpen(WINDOW_ID_YES_NO);
        DAT_1020e0f8[0] = 0;
        if (DAT_104bf8d4 != 1)
            DAT_104bf8d8 = 0;
        DAT_10210ae8[0] = 0;
        strcpy(DAT_1020e0f8, "");
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
            for (i = 0; i < 0x22; i++)
            {
                if (DAT_1030b870[i] > -1)
                {
                    FUN_10174c70(DAT_1030b870[i], 1);
                    DAT_1030b870[i] = -1;
                }
            }
        }
        CloseWindowIfOpen(WINDOW_ID_CHOICE_MENU);
        CloseWindowIfOpen(WINDOW_ID_YES_NO);
        DAT_1020e0f8[0] = 0;
        DAT_104bfe1c = 0;
        memset(DAT_1030c500, 0, sizeof(DAT_1030c500));
    }

    // The camera of the frame: the map, its extra root and the lights are updated under it.
    {
        Matrix4 parent;
        Matrix4 viewProjection;
        Matrix4 projection;
        Matrix4 view;
        parent.SetIdentity();
        FUN_1016ac50(&DAT_118e69e8, &DAT_118e6cb8, &view, &projection);
        viewProjection = view * projection;
        DAT_10866da8 = projection;
        DAT_10836ec8 = parent;
        DAT_108392d8 = viewProjection;
        DAT_10836cd8 = view * DAT_104c03f8;
        if (DAT_10255f08 != 0)
            DAT_10255f08->StartProximityPass(parent.m, &viewProjection, &view, DAT_104bf8a4 % 6000, 0);
        if (DAT_10255f0c != 0)
            DAT_10255f0c->StartProximityPass(parent.m, &DAT_10836cd8, &view, DAT_104bf8a4 % 6000, 0);
        DAT_108654f8.StartProximityPass(DAT_10836ec8.m, &DAT_108392d8, &view, 0, 0);
        FUN_10017fc0();
        DAT_10804bf8.PrepareFrame(&DAT_10836ec8, &DAT_108392d8, DAT_1032d67c[0], 1);
        if (DAT_104c0394 == 0 && DAT_10c63380 == 0)
        {
            DAT_104bf8ec = 0;
            DAT_1032d4b0 = 0;
            DAT_104bf8f0 = 100.0f;
            DAT_104bf8e8 = 100.0f;
            DAT_104bf8e4 = 0;
            Script_ValueCommand(VALUE_COMMAND_FIELD_FADE, 0);
        }
    }
drawScene:
    Menu_DrawScene();
    if (DAT_104bfd78 == 0)
        ResetMeshSlots();
    return 0;
}
