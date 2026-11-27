// The 3D scene behind the menus (called by Menu_Main, Menu_SaveLoad and the mode 15 update): the map, three shadows,
// the player, the effects and the haze of the water regions, drawn in one frame. C++ rather than extern "C": the
// function keeps an exception frame for its Matrix4 locals.

#include <stdlib.h>
#include <string.h>

#include "vec4.h"

#include "matrix4.h"

#include "node.h"

#include "anim_model.h"

#include "model_bank.h"

#include "effect_manager.h"

#include "debris.h"
#include "region_view.h"
#include "texture_object.h"

#include "camera_vars.h"
#include "char_tables.h"
#include "char_vars.h"
#include "game_vars.h"
#include "map_vars.h"
#include "menu_scene.h"
#include "dodge_game.h"            // DAT_1086e0b8: the debris placement is the first dodge obstacle matrix
#include "minigame_hud.h"
#include "render_vars.h"
#include "ui_vars.h"

#include "mesh_shader.h"
#include "render_state.h"

extern "C" int FUN_1016ac50(const Vec4 *eye, const Vec4 *target, Matrix4 *view, Matrix4 *projection);   // Camera_LookAt
extern "C" void FUN_1014b480(void);                    // Render_ResetSwapTarget
extern "C" int FUN_101439e0(void);                     // Render_ResetTarget
void FUN_1000f7f0(int *slots, int count);              // Map_DrawTopView (map_view.cpp): the arguments are unused
extern "C" void FUN_10199860(void);                    // RenderQueue_Clear
extern "C" int FUN_101999a0(const Matrix4 *unused, const Matrix4 *view, Vec4 *lights, unsigned int pass);   // DrawList_DrawPass
extern "C" void FUN_10149100(void);                    // SwapTarget_DrawPreview
extern "C" void FUN_10166520(void);                    // ModelBank::DrawLensFlares

// Fog colour components are floats from 0 to 1.
#define FOG_COLOUR_SCALE 255.0f
// Name of the root node that carries a fog colour.
#define FOG_ROOT_NAME "fogset"
// The overhead pass is on when this bit of the game state DAT_104c003c is set.
#define STATE_OVERHEAD_PASS 2
#define CLEAR_DEPTH_FULL 0xffff
#define ALPHA_TEST_REFERENCE 0xf
#define OPAQUE_BLACK 0xff000000
// The shadows: three casters lit from a light far above and behind the scene.
#define SHADOW_CASTERS 3
#define SHADOW_LIGHT_Y 10977.0f
#define SHADOW_LIGHT_Z 20407.0f
#define SHADOW_SCALE 0.2f
#define SHADOW_SIZE 1
#define SHADOW_CASTER_SIZE 125.0f
// The debris model is drawn while DAT_104c00d0 is in 0x7da..0x80c; its opacity is the distance to 0x7f3 + 0x19.
#define DEBRIS_STATE_FIRST 0x7da
#define DEBRIS_STATE_LAST 0x80c
#define DEBRIS_STATE_CENTRE 0x7f3
#define DEBRIS_STATE_OFFSET 0x19
#define DEBRIS_OPACITY_DIVISOR 10.0f
// The haze of the water regions: drawn in three layers; a layer's texture offset moves one texture width in
// (x period) frames and one height in (y period) frames.
#define HAZE_X1_FRAMES 0x870
#define HAZE_Y1_FRAMES 0xf50
#define HAZE_X2_FRAMES 0x8dc
#define HAZE_Y2_FRAMES 0x1162
#define HAZE_X3_FRAMES 0xb32
#define HAZE_Y3_FRAMES 0xfd8
#define HAZE_X1_PERIOD 2160.0f
#define HAZE_Y1_PERIOD 3920.0f
#define HAZE_X2_PERIOD 2268.0f
#define HAZE_Y2_PERIOD 4450.0f
#define HAZE_X3_PERIOD -2866.0f
#define HAZE_Y3_PERIOD -4056.0f
#define HAZE_ALPHA_SECOND 0.75f
#define HAZE_ALPHA_THIRD 0.35f

// The opaque ARGB colour of a fog colour vector.
static __forceinline unsigned int FogColourArgb(const Vec4 &fog)
{
    return ((((unsigned char)(fog.x * FOG_COLOUR_SCALE) | 0xffffff00) << 8 | (unsigned char)(fog.y * FOG_COLOUR_SCALE)) << 8) |
           (unsigned char)(fog.z * FOG_COLOUR_SCALE);
}

// 0x10022830 (Menu_DrawScene). Draws the map behind the menus into the current target. Unless a menu has set up the
// target (DAT_104c0220), it first resets the target and, when the overhead pass is on (DAT_104c003c bit 1), draws
// the map's top view (clearing to the fog colour of a "fogset" root). Then builds the camera and projection
// matrices from the camera position and target, clears the target to the fog colour (black without a map) and draws
// the fading collision regions, the map with its sky hidden, its shadows, the three shadows of the casters, the
// player (not while DAT_10c63384 or DAT_10c6336c is set), the effects, the debris model while DAT_104c00d0 is
// within 0x19 of 0x7f3, the queued nodes and the lens flares, and the haze of the water regions three times with
// a scrolling offset. The matrices end up in DAT_1065ca08..DAT_1065ca88 for the next pass. Returns 0.
int Menu_DrawScene()
{
    Vec4 tint;
    Vec4 scroll;
    if (DAT_10255f08 == 0)
        return 0;
    if (DAT_104c0220 == 0)
    {
        if (DAT_10f65484 != 0)
            FUN_101439e0();
        else
            FUN_1014b480();
        if (DAT_104c0220 == 0 && DAT_104c003c != 0 && (DAT_104c003c & STATE_OVERHEAD_PASS) != 0)
        {
            DAT_104bfe28 = 1;
            DAT_10203418 = DAT_102022c4;
            if (DAT_102022c4 == 0 || DAT_10f654cc == 0)
                FUN_1000f7f0(DAT_10315140, DAT_104c0054);
            else
            {
                DAT_104c007c->flags &= ~NODE_FLAG_HIDDEN;
                DAT_118d0ba8[DAT_108360f4].UpdateImage(0);
                FUN_1013f070(RENDER_FLUSH_SCENE, 0, 0);
                FUN_1013f070(RENDER_BLEND_MODE, 0, 0);
                FUN_1013f070(RENDER_CULL_MODE, 0, 0);
                FUN_1013f070(RENDER_FOG, 0, 0);
                if (DAT_10255f08 != 0 && stricmp(DAT_10255f08->name, FOG_ROOT_NAME) == 0)
                    FUN_1013f070(RENDER_CLEAR, FogColourArgb(DAT_10255f08->fogColour), 0);
                else
                    FUN_1013f070(RENDER_CLEAR, 0, 0);
                FUN_1000f7f0(DAT_10315140, DAT_104c0054);
                FUN_1013f070(RENDER_FLUSH_SCENE, 0, 0);
                if (DAT_10f65484 != 0)
                    FUN_101439e0();
                else
                    FUN_1014b480();
            }
        }
    }
    if ((DAT_104c003c & STATE_OVERHEAD_PASS) == 0)
        DAT_104bfe28 = 0;
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
    parent.SetIdentity();
    if (DAT_10255f08 != 0)
        FUN_1013f070(RENDER_CLEAR, FogColourArgb(DAT_10255f08->fogColour), 0);
    else
        FUN_1013f070(RENDER_CLEAR, OPAQUE_BLACK, 0);
    FUN_1013f070(RENDER_BLEND_MODE, 0, 0);
    FUN_1013f070(RENDER_TEXTURE_ADDRESS, 0, 0);
    FUN_1013f070(RENDER_TEXTURE_FILTER, 1, 0);
    FUN_1013f070(RENDER_ALPHA_TEST, 1, ALPHA_TEST_REFERENCE);
    FUN_10199860();
    FUN_1013f070(RENDER_BLEND_MODE, 0, 0);
    FUN_1013f070(RENDER_DEPTH_FUNC, 0, 0);
    FUN_1013f070(RENDER_CULL_MODE, 0, 0);
    if (DAT_10255f0c != 0)
        DAT_10255f0c->QueueDrawFading(&DAT_10836ec8, &DAT_10836cd8,
                                      DAT_10804bf8.LightForRoot(DAT_104c039c, &DAT_118e69e8, &DAT_118e6cb8), -1,
                                      &DAT_10da90d0, 0);
    FUN_1013f070(RENDER_CLEAR_DEPTH, CLEAR_DEPTH_FULL, 0);
    int skyWasHidden = 0;
    if (DAT_1021c634 == 0)
    {
        if (DAT_10255f08 != 0)
            DAT_1021c634 = DAT_10255f08->Find("ClearORoot", 0);
    }
    else
    {
        if ((DAT_1021c634->flags & NODE_FLAG_HIDDEN) != 0)
            skyWasHidden = 1;
        DAT_1021c634->SetFlags(NODE_FLAG_HIDDEN);
    }
    if (DAT_10255f08 != 0)
        DAT_10255f08->QueueDrawFading(&DAT_10836ec8, &DAT_108392d8,
                                      DAT_10804bf8.LightForRoot(DAT_104c039c, &DAT_118e69e8, &DAT_118e6cb8), -1,
                                      &DAT_10da90d0, 0);
    if (DAT_1021c634 != 0 && !skyWasHidden)
        DAT_1021c634->ClearFlags(NODE_FLAG_HIDDEN);
    FUN_1013f070(RENDER_DEPTH_WRITE, 0, 0);
    if (DAT_102022b8 != 0)
        DAT_10804bf8.DrawShadowsInto(DAT_108654f8.FindByFlags(NODE_FLAG_DRAWN, 0, 0), &DAT_10836ec8, &DAT_108392d8, 0);
    else if (DAT_10255f08 != 0)
        DAT_10804bf8.DrawShadows(DAT_10255f08, &DAT_10836ec8, &DAT_108392d8, 0);
    {
        Vec4 lightPosition;
        lightPosition.x = 0;
        lightPosition.y = SHADOW_LIGHT_Y;
        lightPosition.z = SHADOW_LIGHT_Z;
        if (DAT_10255f08 != 0)
        {
            for (int caster = 0; caster < SHADOW_CASTERS; caster++)
            {
                if (caster & 1)
                    DAT_10255f08->DrawShadow(&DAT_10836ec8, &DAT_108392d8, DAT_104bf8a4,
                                             &DAT_10803ab8[caster % 2 + caster / 2 * 4], &lightPosition, DAT_1020c324,
                                             SHADOW_SCALE, SHADOW_SIZE, SHADOW_CASTER_SIZE, 0, 1.0f);
                else
                    DAT_10255f08->DrawShadow(&DAT_10836ec8, &DAT_108392d8, DAT_104bf8a4,
                                             &DAT_10803ab8[caster % 2 + caster / 2 * 4], &lightPosition, DAT_1020c320,
                                             SHADOW_SCALE, SHADOW_SIZE, SHADOW_CASTER_SIZE, 0, 1.0f);
            }
        }
    }
    FUN_1013f070(RENDER_DEPTH_WRITE, 1, 0);
    Vec4 colour;
    colour.Fill(1.0f);
    if (DAT_10c63384 == 0 && DAT_10c6336c == 0)
        DAT_104c039c->QueueDraw(&DAT_1086df38, &DAT_108392d8,
                                DAT_10804bf8.LightForRoot(DAT_104c039c, &DAT_118e69e8, &DAT_118e6cb8), -1, &colour, 0);
    DAT_10daaa80.Draw(DAT_108392d8, &DAT_10836330, 1);
    if (DAT_104c00d0 >= DEBRIS_STATE_FIRST && DAT_104c00d0 <= DEBRIS_STATE_LAST)
    {
        Vec4 fade;
        fade.Fill(1.0f);
        fade.w = (float)abs(abs(DAT_104c00d0 - DEBRIS_STATE_CENTRE) - DEBRIS_STATE_OFFSET) / DEBRIS_OPACITY_DIVISOR;
        DAT_104d2a08[DAT_104c0198].QueueDraw(&DAT_1086e0b8[0].endA, &DAT_108392d8,
                                             DAT_10804bf8.LightForRoot(DAT_104c039c, &DAT_118e69e8, &DAT_118e69e8), -1,
                                             &fade, 0);
    }
    FUN_1013f070(RENDER_ALPHA_TEST, 1, ALPHA_TEST_REFERENCE);
    FUN_101999a0(&DAT_10836ec8, &DAT_108392d8, &DAT_118e69e8, 0);
    DAT_1065ca08[0] = DAT_10836ec8;
    DAT_1065ca08[1] = DAT_108392d8;
    DAT_1065ca08[2] = DAT_10836330;
    DAT_10c67ef0[0].Copy(DAT_118e69e8);
    DAT_10daaa80.Draw(DAT_108392d8, &DAT_10836330, 0);
    DAT_10daaa80.Draw(DAT_108392d8, &DAT_10836330, 2);
    FUN_1013f070(RENDER_FLUSH_SCENE, 0, 0);
    DAT_10804bf8.DrawLensFlares();
    if (DAT_104c0220 == 0 && DAT_102027f0 != 0)
    {
        DAT_118d0ba8[DAT_108360f4].UpdateImage(0);
        FUN_1013f070(RENDER_FLUSH_SCENE, 0, 0);
        FUN_1013f070(RENDER_DEPTH_TEST, 1, 0);
        FUN_1013f070(RENDER_DEPTH_WRITE, 1, 0);
        FUN_1013f070(RENDER_DEPTH_TEST, 0, 0);
        FUN_1013f070(RENDER_DEPTH_WRITE, 0, 0);
        FUN_10149100();
        FUN_1013f070(RENDER_FLUSH_SCENE, 0, 0);
        if (DAT_10f65484 != 0)
            FUN_101439e0();
        else
            FUN_1014b480();
        FUN_1013f070(RENDER_DEPTH_WRITE, 1, 0);
        FUN_1013f070(RENDER_DEPTH_TEST, 1, 0);
    }
    if (DAT_104c03c0 != 0 && DAT_102028e0 > -1)
    {
        Matrix4 world;
        FUN_1013f070(RENDER_ALPHA_TEST, 1, 0);
        FUN_1013f070(RENDER_BLEND_MODE, 0, 0);
        FUN_1013f070(RENDER_TEXTURE_ADDRESS, 0, 0);
        FUN_1013f070(RENDER_DEPTH_TEST, 0, 0);
        tint.Fill(1.0f);
        scroll.Fill(0.0f);
        scroll.x = (float)(DAT_104bf8a4 % HAZE_X1_FRAMES) / HAZE_X1_PERIOD;
        scroll.y = (float)(DAT_104bf8a4 % HAZE_Y1_FRAMES) / HAZE_Y1_PERIOD;
        world = DAT_10836ec8 * Scaling(1.0f, 1.0f, 1.0f);
        for (int region = 0; region < DAT_102564a0; region++)
        {
            if (DAT_104c03c0[region].visible != 0)
                DAT_118d0ba8[DAT_102028e4].DrawMesh(&DAT_10836ec8, &DAT_108392d8, DAT_104c03c0[region].vertices,
                                                    DAT_104c03c0[region].pointCount, DAT_104c03c0[region].buffer0004,
                                                    DAT_104c03c0[region].indexCount, MESH_SHADER_SHADED, &tint,
                                                    &scroll);
        }
        FUN_1013f070(RENDER_DEPTH_WRITE, 0, 0);
        FUN_1013f070(RENDER_BLEND_MODE, 1, 0);
        tint.Fill(1.0f);
        tint.w = HAZE_ALPHA_SECOND;
        scroll.Fill(0.0f);
        scroll.x = (float)(DAT_104bf8a4 % HAZE_X2_FRAMES) / HAZE_X2_PERIOD;
        scroll.y = (float)(DAT_104bf8a4 % HAZE_Y2_FRAMES) / HAZE_Y2_PERIOD;
        world = DAT_10836ec8 * TranslationMatrix(0, 0, 0);
        for (int region = 0; region < DAT_102564a0; region++)
        {
            if (DAT_104c03c0[region].visible != 0)
                DAT_118d0ba8[DAT_102028e0].DrawMesh(&world, &DAT_108392d8, DAT_104c03c0[region].vertices,
                                                    DAT_104c03c0[region].pointCount, DAT_104c03c0[region].buffer0004,
                                                    DAT_104c03c0[region].indexCount, MESH_SHADER_SHADED, &tint,
                                                    &scroll);
        }
        scroll.x = (float)(DAT_104bf8a4 % HAZE_X3_FRAMES) / HAZE_X3_PERIOD;
        scroll.y = (float)(DAT_104bf8a4 % HAZE_Y3_FRAMES) / HAZE_Y3_PERIOD;
        tint.w = HAZE_ALPHA_THIRD;
        world = DAT_10836ec8 * Scaling(1.0f, 1.0f, 1.0f) * TranslationMatrix(0, 0, 0);
        for (int region = 0; region < DAT_102564a0; region++)
        {
            if (DAT_104c03c0[region].visible != 0)
                DAT_118d0ba8[DAT_102028e0].DrawMesh(&world, &DAT_108392d8, DAT_104c03c0[region].vertices,
                                                    DAT_104c03c0[region].pointCount, DAT_104c03c0[region].buffer0004,
                                                    DAT_104c03c0[region].indexCount, MESH_SHADER_SHADED, &tint,
                                                    &scroll);
        }
        FUN_1013f070(RENDER_BLEND_MODE, 0, 0);
        FUN_1013f070(RENDER_DEPTH_WRITE, 1, 0);
    }
    FUN_1013f070(RENDER_DEPTH_TEST, 1, 0);
    FUN_1013f070(RENDER_CLEAR_DEPTH, CLEAR_DEPTH_FULL, 0);
    if (DAT_1021c634 != 0 && (DAT_1021c634->flags & NODE_FLAG_HIDDEN) == 0)
        DAT_1021c634->QueueDraw(&DAT_10836ec8, &DAT_108392d8,
                                DAT_10804bf8.LightForRoot(DAT_104c039c, &DAT_118e69e8, &DAT_118e6cb8), -1,
                                &DAT_10da90d0, 0);
    FUN_101999a0(&DAT_1065ca08[0], &DAT_1065ca08[1], DAT_10c67ef0, 1);
    DAT_10daaa80.Draw(DAT_1065ca08[1], &DAT_1065ca08[2], 3);
    return 0;
}
