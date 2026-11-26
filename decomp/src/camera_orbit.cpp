// Camera helpers: orbiting the camera position (DAT_11af3e28) around its target (DAT_118e6c78) on the ground plane
// and turning input directions into camera space.

#include <math.h>

#include "vec4.h"

#include "camera_vars.h"

// Places the camera at heading `angle` around the target, keeping its ground distance and its height above the
// target. Returns 0.
extern "C" int FUN_1016bdf0(float angle)
{
    Vec4 offset;
    float zero = 0;
    offset.Fill(zero);
    offset.z = DAT_11af3e28.z - DAT_118e6c78.z;
    offset.y = (DAT_11af3e28 - DAT_118e6c78).Length2D();
    offset.RotateZ(angle);
    DAT_11af3e28.Copy(DAT_118e6c78 + offset);
    return 0;
}

// Turns the direction (x, y) by the camera heading (the angle from the camera position to its target) and stores it,
// doubled, in DAT_11af3f58 with z = 0. Returns 0.
extern "C" int FUN_1016b260(float x, float y)
{
    float angle = (float)atan2(DAT_118e6c78.x - DAT_11af3e28.x, DAT_118e6c78.y - DAT_11af3e28.y);
    float turnedX = (float)cos(angle) * x - (float)sin(angle) * y;
    float turnedY = (float)sin(angle) * x + (float)cos(angle) * y;
    DAT_11af3f58.z = 0;
    DAT_11af3f58.x = turnedX + turnedX;
    DAT_11af3f58.y = turnedY + turnedY;
    return 0;
}

// Turns the input direction (x, y) into world space for the current camera heading (the camera looks from its
// position towards its target) and stores the result in *outX and *outY. Returns 0.
extern "C" int FUN_1016b340(float *outX, float *outY, float x, float y)
{
    float angle = -(float)atan2(DAT_118e6c78.x - DAT_11af3e28.x, DAT_118e6c78.y - DAT_11af3e28.y);
    *outX = (float)cos(angle) * x - (float)sin(angle) * y;
    *outY = (float)cos(angle) * y + (float)sin(angle) * x;
    return 0;
}


// 0x1016b400 (Camera_TurnInput). Like FUN_1016b340, but while a camera cut is blending (DAT_1101d88c frames of
// 60 left) the direction is a mix of the turn by the old camera heading (weight 1 - frames / 60) and by the new
// one; without a blend the current camera is remembered as the old one. The input is also stored in DAT_11af3f58
// (z = 0). Returns 0.
extern "C" int FUN_1016b400(float *outX, float *outY, float x, float y)
{
    if (DAT_1101d88c == 0)
    {
        float angle = -(float)atan2(DAT_118e6c78.x - DAT_11af3e28.x, DAT_118e6c78.y - DAT_11af3e28.y);
        *outX = (float)cos(angle) * x - (float)sin(angle) * y;
        *outY = (float)cos(angle) * y + (float)sin(angle) * x;
        DAT_11af3f08.Copy(DAT_11af3e28);
        DAT_11af3f18.Copy(DAT_118e6c78);
    }
    else
    {
        float frames = (float)DAT_1101d88c;
        *outX = 0;
        *outY = 0;
        float blend = frames / 60.0f;
        float before = 1.0f - frames / 60.0f;
        float angle = -(float)atan2(DAT_11af3f18.x - DAT_11af3f08.x, DAT_11af3f18.y - DAT_11af3f08.y);
        *outX = ((float)cos(angle) * x - (float)sin(angle) * y) * before + *outX;
        *outY = ((float)cos(angle) * y + (float)sin(angle) * x) * before + *outY;
        angle = -(float)atan2(DAT_118e6c78.x - DAT_11af3e28.x, DAT_118e6c78.y - DAT_11af3e28.y);
        *outX = ((float)cos(angle) * x - (float)sin(angle) * y) * blend + *outX;
        *outY = ((float)cos(angle) * y + (float)sin(angle) * x) * blend + *outY;
    }
    DAT_11af3f58.x = x;
    DAT_11af3f58.y = y;
    DAT_11af3f58.z = 0;
    return 0;
}
