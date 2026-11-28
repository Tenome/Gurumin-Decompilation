// Drawing every character: queues each slot's model under the view matrix, with the outline pass for the selected one
// (0x1000c720).

#include "vec4.h"

#include "anim_model.h"
#include "char_tables.h"
#include "char_vars.h"
#include "matrix4.h"
#include "model_bank.h"
#include "node.h"
#include "render_state.h"
#include "render_vars.h"
#include "texture_object.h"
#include "ui_vars.h"
#include "game_vars.h"
#include "map_vars.h"

#define FIRST_AREA_COLOUR_SLOT 0x3a   // slots from here on are drawn with their own value of DAT_10f655c4
#define FIRST_UNLIT_SLOT 0x5c         // slots from here on are lit by the caller's light set
#define MODEL_ACTIVE_OPACITY 0xf      // shadowOpacity of a model that is placed
#define OVERHEAD_MARGIN_XY 200.0f     // how far outside the visible area (x and y) a character is still drawn overhead
#define OVERHEAD_MARGIN_Z 500.0f
#define OUTLINE_LEFT (-1)
#define OUTLINE_TOP (-1)
#define OUTLINE_RIGHT 0x27f
#define OUTLINE_BOTTOM 0x1df
#define OUTLINE_UV_MIN (1.0f / 1024)
#define OUTLINE_UV_MAX (1023.0f / 1024)
#define OUTLINE_DEPTH_SCALE 0.98
#define OUTLINE_DEPTH_BIAS 0.01

// 0x1000c720 (Characters_Draw). Queues the model of every character slot in the list DAT_10315140, last first, under
// `view` * `projection`: slots from 0x3a on set DAT_10f655c4 to their value in DAT_10447bd0 (0 when the slot has its
// flag in DAT_1020d438), the lights come from the model bank (or `defaultLight` for slots from 0x5c on). In the normal
// pass only placed models (shadow opacity 15) are drawn, and the slot DAT_104c02c5 gets an outline: with the depth test
// kept and depth writes off, a full-screen quad of one of the two outline textures is drawn at the slot's projected
// depth with DAT_104c02c7 percent opacity (added to the scene, or darkened into it when DAT_104c02c6 is set). In the
// overhead pass (DAT_10f65520) every character inside the visible area plus a margin is drawn. Returns 1.
int FUN_1000c720(Matrix4 *view, const Matrix4 &projection, Vec4 *defaultLight)
{
    Matrix4 viewProjection = (*view) * projection;
    if (DAT_10f65520 != 0)
    {
        for (int i = DAT_104c0054 - 1; i >= 0; i--)
        {
            int slot = DAT_10315140[i];
            Vec4 position = DAT_10db6ee8[slot].matrix.GetTranslation();
            if (DAT_10f668a8.min.x - OVERHEAD_MARGIN_XY < position.x && DAT_10f668a8.min.y - OVERHEAD_MARGIN_XY < position.y &&
                DAT_10f668a8.min.z - OVERHEAD_MARGIN_Z < position.z && position.x < DAT_10f668a8.max.x + OVERHEAD_MARGIN_XY &&
                position.y < DAT_10f668a8.max.y + OVERHEAD_MARGIN_XY && position.z < DAT_10f668a8.max.z + OVERHEAD_MARGIN_Z)
            {
                DAT_10f66948.Copy(DAT_10f08080[slot] + position);
                if (slot >= FIRST_AREA_COLOUR_SLOT)
                    DAT_10f655c4 = DAT_10447bd0[slot].bits;
                if (DAT_1020d438[slot] != 0)
                    DAT_10f655c4 = 0;
                if (slot < FIRST_UNLIT_SLOT)
                    DAT_10db6ee8[slot].Draw(&viewProjection,
                                            DAT_10804bf8.LightDirection(&DAT_10db6ee8[slot], &DAT_118e69e8, &DAT_118e6cb8), 0);
                else
                    DAT_10db6ee8[slot].Draw(&viewProjection, defaultLight, 0);
                DAT_10f655c4 = 0;
            }
        }
    }
    else
    {
        for (int i = DAT_104c0054 - 1; i >= 0; i--)
        {
            int slot = DAT_10315140[i];
            if (DAT_10db6ee8[slot].shadowOpacity == MODEL_ACTIVE_OPACITY)
            {
                Vec4 position = DAT_10db6ee8[slot].matrix.GetTranslation();
                DAT_10f66948.Copy(DAT_10f08080[slot] + position);
                if (slot >= FIRST_AREA_COLOUR_SLOT)
                    DAT_10f655c4 = DAT_10447bd0[slot].bits;
                if (DAT_1020d438[slot] != 0)
                    DAT_10f655c4 = 0;
                if (slot < FIRST_UNLIT_SLOT)
                    DAT_10db6ee8[slot].Draw(&viewProjection,
                                            DAT_10804bf8.LightDirection(&DAT_10db6ee8[slot], &DAT_118e69e8, &DAT_118e6cb8), 0);
                else
                    DAT_10db6ee8[slot].Draw(&viewProjection, defaultLight, 0);
                if (DAT_104c02c5 != 0 && DAT_10f654cc != 0 && slot == DAT_104c02c5)
                {
                    FUN_1013f070(RENDER_DEPTH_TEST, 1, 0);
                    FUN_1013f070(RENDER_DEPTH_WRITE, 0, 0);
                    if (DAT_104c02c6 == 0)
                        FUN_1013f070(RENDER_BLEND_MODE, BLEND_ADD, 0);
                    else
                        FUN_1013f070(RENDER_BLEND_MODE, BLEND_DARKEN, 0);
                    Vec4 screen;
                    screen.Copy(Project(position, viewProjection));
                    DAT_118d0ba8[DAT_10836100[DAT_104c02c4 % 2][0]].DrawTinted(
                        OUTLINE_LEFT, OUTLINE_TOP, OUTLINE_RIGHT, OUTLINE_BOTTOM, OUTLINE_UV_MIN, OUTLINE_UV_MIN,
                        OUTLINE_UV_MAX, OUTLINE_UV_MAX, (float)(screen.z * OUTLINE_DEPTH_SCALE - OUTLINE_DEPTH_BIAS),
                        (float)DAT_104c02c7 / 100.0f, 0, 0);
                    FUN_1013f070(RENDER_BLEND_MODE, BLEND_ALPHA, 0);
                    FUN_1013f070(RENDER_DEPTH_TEST, 1, 0);
                    FUN_1013f070(RENDER_DEPTH_WRITE, 1, 0);
                }
                DAT_10f655c4 = 0;
            }
        }
    }
    return 1;
}

#define FIRST_NEARBY_SLOT 0x5c        // the slots the nearby-model pass draws: 0x5c..0x7b
#define END_NEARBY_SLOT 0x7c
#define NOT_PLACED_X (-100000.0f)     // a model whose x is not above this has not been placed in the world
#define NEARBY_RANGE 800.0f           // models farther than this from the shadow caster's position are skipped

// 0x1000c620 (the map root node's draw callback, set up by the map loaders). After the node's own subtree is drawn it
// also draws the model trees of the slots 0x5c..0x7b that are placed in the world (x above -100000) and lie within 800
// units on the x/y plane of the position the caller passes (shadowX, shadowY, shadowZ), skipping the model the call
// itself draws for. The arguments are Node::Draw's own, handed through; the pass argument is 0. Returns 1.
extern "C" int FUN_1000c620(Matrix4 *world, const Matrix4 *view, int a3, const Matrix4 *shadowMatrix, float shadowX,
                            float shadowY, float shadowZ, int shadowTexture, float shadowScale, int shadowSize,
                            const Vec4 *lightDirection, float modelSize, AnimModel *model, float alpha)
{
    Vec4 position;
    position.Set(shadowX, shadowY, shadowZ, 0.0f);
    for (int slot = FIRST_NEARBY_SLOT; slot < END_NEARBY_SLOT; slot++)
    {
        if (model != &DAT_10db6ee8[slot] && DAT_10db6ee8[slot].root != 0 && DAT_10db6ee8[slot].matrix._41 > NOT_PLACED_X &&
            (position - DAT_10db6ee8[slot].matrix.GetTranslation()).Length2D() < NEARBY_RANGE)
            DAT_10db6ee8[slot].root->Draw(world, view, 0, shadowMatrix, shadowX, shadowY, shadowZ, shadowTexture,
                                          shadowScale, shadowSize, lightDirection, modelSize, model, alpha);
    }
    return 1;
}
