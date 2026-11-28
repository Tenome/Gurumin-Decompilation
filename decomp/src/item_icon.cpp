// Item icons.

#include "game.h"
#include "anim_model.h"
#include "texture_object.h"

// Item table at 0x1088e6b0, 0xc0 bytes per item; `icon` indexes the 8x8 icon sheet texIcon01.
#include "item_info.h"

#include "ui_vars.h"

// Draws the icon of the item in inventory slot `slot` as a 16x16 sprite at (x, y), insetting the UVs by 0.002
// to avoid bleeding from the neighbouring icons.
extern "C" int FUN_1000c200(int x, int y, int slot, float alpha)
{
    int icon = DAT_1088e6b0[DAT_1042e2a8[slot]].icon;
    float u = icon % 8 * 0.125f;
    float v = icon / 8 * 0.125f;
    DAT_118d0ba8[DAT_104b7200.texIcon01].DrawTinted(x, y + 2, x + 0x10, y + 0x12, u + 0.002f, v + 0.002f, u + 0.125f - 0.002f,
                                                     v + 0.125f - 0.002f, 0, alpha, 0, 0);
    return 0;
}

// 0x1000c2f0. qsort comparison of two model slot numbers (game frame, before the models are updated): the model
// whose depth key comes first sorts first. The key is the world z of the model plus its draw-order bias m_0068.
extern "C" int FUN_1000c2f0(const void *first, const void *second)
{
    int a = *(const int *)first;
    int b = *(const int *)second;
    if (DAT_10db6ee8[a].m_0068 + DAT_10db6ee8[a].matrix.translation.z <
        DAT_10db6ee8[b].m_0068 + DAT_10db6ee8[b].matrix.translation.z)
        return -1;
    return 1;
}

// Draws texture slot `slot` as a screen rectangle (left, top)-(right, bottom) using a source rectangle given in
// texture pixels (u0, v0)-(u1, v1), converted here to 0..1 texture coordinates. Returns 0 for a negative slot.
extern "C" int FUN_101616e0(int slot, int left, int top, int right, int bottom, float u0, float v0, float u1, float v1,
                            float z, float alpha, float angle)
{
    if (slot < 0)
        return 0;
    float height = (float)DAT_118d0ba8[slot].height;
    float width = (float)DAT_118d0ba8[slot].width;
    DAT_118d0ba8[slot].DrawTinted(left, top, right, bottom, u0 / width, v0 / height, u1 / width, v1 / height, z, alpha,
                                  angle, 0);
    return 1;
}
