// Visibility of map regions (the table at DAT_104c03c0): a region counts as visible when a corner of its box, at
// half its height, is inside the camera's view volume.

#include <math.h>

#include "vec4.h"

#include "matrix4.h"

#include "region_view.h"
#include "render_vars.h"
#include "game_vars.h"
#include "map_vars.h"

extern "C" int FUN_101ae4d0(const Vec4 *point, const Matrix4 *matrix, float margin);  // View_ContainsPoint

// 0x10021070 (Region_IsVisible). 1 when one of the four corners of region `index` (at half its height) lies in the
// view volume widened by 50, else 0 (also for hidden regions).
extern "C" int FUN_10021070(int index)
{
    Vec4 corner;
    if (DAT_104c03c0[index].hidden != 0)
        return 0;
    float middle = DAT_104c03c0[index].max.z + DAT_104c03c0[index].min.z;
    middle *= 0.5f;  // half the height
    corner.Copy(DAT_104c03c0[index].min);
    corner.z = middle;
    if (FUN_101ae4d0(&corner, &DAT_108392d8, 50.0f) != 0)
        return 1;
    corner.Copy(DAT_104c03c0[index].max);
    corner.z = middle;
    if (FUN_101ae4d0(&corner, &DAT_108392d8, 50.0f) != 0)
        return 1;
    corner.x = DAT_104c03c0[index].min.x;
    corner.y = DAT_104c03c0[index].max.y;
    corner.z = middle;
    if (FUN_101ae4d0(&corner, &DAT_108392d8, 50.0f) != 0)
        return 1;
    corner.x = DAT_104c03c0[index].max.x;
    corner.y = DAT_104c03c0[index].min.y;
    corner.z = middle;
    return FUN_101ae4d0(&corner, &DAT_108392d8, 50.0f) != 0;
}

// 0x100211a0 (Region_WaveVertex). Moves vertex `point` of region `index` by sine waves of the frame counter: 15 units
// in x and y (periods of 240 and 300 frames) and 10 in z (356 frames), half a period later when exactly one of the
// point's two values has an odd hundreds digit. The offsets add up from frame to frame. Returns 0.
extern "C" int FUN_100211a0(int index, int point)
{
    int flip = 0;
    if ((int)DAT_104c03c0[index].points[point].m_0060 / 100 & 1)
        flip = 1;
    if ((int)DAT_104c03c0[index].points[point].m_0064 / 100 & 1)
        flip++;
    float phase = (float)(flip & 1) * 3.1415f;
    float angleX = (float)(DAT_104bf8a4 % 240) * 0.026179167f + phase;
    float angleY = (float)(DAT_104bf8a4 % 300) * 0.020943332f + phase;
    float angleZ = (float)(DAT_104bf8a4 % 356) * 0.017648876f + phase;
    DAT_104c03c0[index].vertices[point].x += sinf(angleX) * 15.0f;
    DAT_104c03c0[index].vertices[point].y += sinf(angleY) * 15.0f;
    DAT_104c03c0[index].vertices[point].z += sinf(angleZ) * 10.0f;
    return 0;
}

#define MORPH_FRAMES 60

// 0x1004d4b0 (Regions_Update). Per frame, for every visible region (see FUN_10021070): copies each point's current
// position into its vertex, and while a point is morphing (`morphing` set) blends its vertex towards the point's
// "to" values over 60 frames (age = frame counter - the region's start frame - the point's start; one of the two
// blends runs at twice the speed, which one depends on the two tint w values), then lets the wave move the vertex.
// A finished morph copies the "from" values to the vertex, clears the flag and keeps the position as the point's
// current one. A region with no morphing point is marked settled. Invisible regions only get `visible` = 0, and with
// DAT_104c03c4 clear every region is marked invisible. Returns 0.
extern "C" int FUN_1004d4b0()
{
    if (DAT_104c03c4 == 0)
    {
        for (int region = 0; region < DAT_102564a0; region++)
            DAT_104c03c0[region].visible = 0;
        return 0;
    }
    for (int region = 0; region < DAT_102564a0; region++)
    {
        if (FUN_10021070(region) != 0)
        {
            DAT_104c03c0[region].visible = 1;
            int anyMorphing = 0;
            for (int point = 0; point < DAT_104c03c0[region].pointCount; point++)
            {
                DAT_104c03c0[region].vertices[point].x = DAT_104c03c0[region].points[point].m_0060;
                DAT_104c03c0[region].vertices[point].y = DAT_104c03c0[region].points[point].m_0064;
                DAT_104c03c0[region].vertices[point].z = DAT_104c03c0[region].points[point].m_0068;
                RegionPoint *p = &DAT_104c03c0[region].points[point];
                if (p->morphing != 0)
                {
                    unsigned int start = DAT_104c03c0[region].morphStart;
                    anyMorphing = 1;
                    int age;
                    if (DAT_104bf8a4 < start)
                        age = (DAT_104bf8a4 - start) - 1;
                    else
                        age = DAT_104bf8a4 - start;
                    age -= p->morphStart;
                    if (age >= 0)
                    {
                        if (age < MORPH_FRAMES)
                        {
                            float positionBlend = (float)age / 60.0f;
                            float fast = positionBlend + positionBlend;
                            float tintBlend;
                            if (p->tintFrom.w <= p->tintTo.w)
                                tintBlend = 1.0f <= fast ? 1.0f : fast;
                            else
                            {
                                tintBlend = positionBlend;
                                positionBlend = 1.0f <= fast ? 1.0f : fast;
                            }
                            float rest = 1.0f - tintBlend;
                            DAT_104c03c0[region].vertices[point].offset.x = tintBlend * p->offsetFrom.x + rest * p->offsetTo.x;
                            DAT_104c03c0[region].vertices[point].offset.y =
                                tintBlend * DAT_104c03c0[region].points[point].offsetFrom.y + rest * DAT_104c03c0[region].points[point].offsetTo.y;
                            DAT_104c03c0[region].vertices[point].offset.z =
                                tintBlend * DAT_104c03c0[region].points[point].offsetFrom.z + rest * DAT_104c03c0[region].points[point].offsetTo.z;
                            DAT_104c03c0[region].vertices[point].tint.x =
                                tintBlend * DAT_104c03c0[region].points[point].tintFrom.x + rest * DAT_104c03c0[region].points[point].tintTo.x;
                            DAT_104c03c0[region].vertices[point].tint.y =
                                tintBlend * DAT_104c03c0[region].points[point].tintFrom.y + rest * DAT_104c03c0[region].points[point].tintTo.y;
                            DAT_104c03c0[region].vertices[point].tint.z =
                                tintBlend * DAT_104c03c0[region].points[point].tintFrom.z + rest * DAT_104c03c0[region].points[point].tintTo.z;
                            DAT_104c03c0[region].vertices[point].tint.w =
                                tintBlend * DAT_104c03c0[region].points[point].tintFrom.w + rest * DAT_104c03c0[region].points[point].tintTo.w;
                            if (positionBlend > 0.0f)
                            {
                                rest = 1.0f - positionBlend;
                                p = &DAT_104c03c0[region].points[point];
                                DAT_104c03c0[region].vertices[point].x = positionBlend * p->positionFrom.x + rest * p->positionTo.x;
                                DAT_104c03c0[region].vertices[point].y =
                                    positionBlend * DAT_104c03c0[region].points[point].positionFrom.y + rest * DAT_104c03c0[region].points[point].positionTo.y;
                                DAT_104c03c0[region].vertices[point].z =
                                    positionBlend * DAT_104c03c0[region].points[point].positionFrom.z + rest * DAT_104c03c0[region].points[point].positionTo.z;
                            }
                        }
                        else
                        {
                            DAT_104c03c0[region].vertices[point].x = p->positionFrom.x;
                            DAT_104c03c0[region].vertices[point].y = DAT_104c03c0[region].points[point].positionFrom.y;
                            DAT_104c03c0[region].vertices[point].z = DAT_104c03c0[region].points[point].positionFrom.z;
                            DAT_104c03c0[region].vertices[point].tint.w = DAT_104c03c0[region].points[point].tintFrom.w;
                            DAT_104c03c0[region].vertices[point].offset.x = DAT_104c03c0[region].points[point].offsetFrom.x;
                            DAT_104c03c0[region].vertices[point].offset.y = DAT_104c03c0[region].points[point].offsetFrom.y;
                            DAT_104c03c0[region].vertices[point].offset.z = DAT_104c03c0[region].points[point].offsetFrom.z;
                            DAT_104c03c0[region].vertices[point].tint.x = DAT_104c03c0[region].points[point].tintFrom.x;
                            DAT_104c03c0[region].vertices[point].tint.y = DAT_104c03c0[region].points[point].tintFrom.y;
                            DAT_104c03c0[region].vertices[point].tint.z = DAT_104c03c0[region].points[point].tintFrom.z;
                            DAT_104c03c0[region].points[point].morphing = 0;
                            DAT_104c03c0[region].points[point].m_0060 = DAT_104c03c0[region].vertices[point].x;
                            DAT_104c03c0[region].points[point].m_0064 = DAT_104c03c0[region].vertices[point].y;
                            DAT_104c03c0[region].points[point].m_0068 = DAT_104c03c0[region].vertices[point].z;
                        }
                    }
                }
                FUN_100211a0(region, point);
            }
            if (!anyMorphing)
                DAT_104c03c0[region].settled = 0;
        }
        else
            DAT_104c03c0[region].visible = 0;
    }
    return 0;
}
