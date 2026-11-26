#include "char_state.h"
// Following the walking routes of the map: each route has 16 points (x, y, z and a code). The code holds an action
// (code / 10000) and a value (code % 10000), each wrapped to -4999..5000; the action's low two bits say what happens
// when the walker reaches the point and the rest is a group number.

#include "route_point.h"

#include "map_records.h"

// 0x1002f320 (Route_Follow). Reads point `point` of route `route` for walker `walker`: stores its action (low two
// bits), value and group in *outAction, *outValue and *outGroup. When `position` lies within `radius` of the point
// on all three axes the walker moves on: action 0 goes to the next point in its direction, 1 forwards, 2 back to
// point 1 and 3 turns it round (1 and 2 also set it walking forwards). Stores the point it heads for in *outPoint
// and that point's coordinates in `position`. Returns 1-3 for the actions 1-3 when the point was reached, else 0.
extern "C" int FUN_1002f320(int walker, int route, int point, int *outPoint, float *position, int *outAction,
                            int *outValue, int *outGroup, int radius)
{
    float x = (float)DAT_10210fc0[route].points[point].x;
    float y = (float)DAT_10210fc0[route].points[point].y;
    float z = (float)DAT_10210fc0[route].points[point].z;
    int action = DAT_10210fc0[route].points[point].code / 10000;
    int result = 0;
    if (action > 5000)
        action -= 10000;
    *outAction = action;
    int value = DAT_10210fc0[route].points[point].code % 10000;
    if (value > 5000)
        value -= 10000;
    *outValue = value;
    *outGroup = *outAction / 4;
    *outAction = *outAction % 4;
    float range = (float)radius;
    int next = point;
    if (x - range < position[0] && range + x > position[0] && y - range < position[1] && range + y > position[1] &&
        z - range < position[2] && range + z > position[2])
    {
        switch (*outAction)
        {
        case 0:
            if (DAT_1020b430[walker].reversed != 0)
                next = point - 1;
            else
                next = point + 1;
            break;
        case 1:
            result = 1;
            DAT_1020b430[walker].reversed = 0;
            next = point + 1;
            break;
        case 2:
            result = 2;
            DAT_1020b430[walker].reversed = 0;
            next = 1;
            break;
        case 3:
            result = 3;
            DAT_1020b430[walker].reversed = 1;
            next = point - 1;
            break;
        }
        x = (float)DAT_10210fc0[route].points[next].x;
        y = (float)DAT_10210fc0[route].points[next].y;
        z = (float)DAT_10210fc0[route].points[next].z;
    }
    *outPoint = next;
    position[0] = x;
    position[1] = y;
    position[2] = z;
    return result;
}

#include "game.h"

extern "C" int FUN_100286b0(int value);          // Angle_Wrap: -4999..5000

// 0x1002f050 (Route_FollowGates). Like Route_Follow for routes whose points pack x * 10000 + y and z * 10000 + value
// (each part wrapped to -4999..5000) and carry a gate type and a parameter: stores the value, type and parameter in
// *outValue, *outType and *outParam. Within 20 units of the point the walker moves on: type 0 to the next point in
// its direction, 1 jumps to point `parameter` when script flag `value` is set, 2 stops at point 0 (parameter 0) or
// turns back, 3 waits `parameter` frames first and 4 walks forwards again. Stores the point it heads for in
// *outPoint and its coordinates in `position`. Returns 1 for the types 2 and 4 once reached, else 0.
extern "C" int FUN_1002f050(int walker, int route, int point, int *outPoint, float *position, int *outType,
                            int *outParam, int *outValue)
{
    int x = DAT_10210fc0[route].points[point].x / 10000;
    if (x > 5000)
        x -= 10000;
    int result = 0;
    float fx = (float)x;
    int y = DAT_10210fc0[route].points[point].x % 10000;
    if (y > 5000)
        y -= 10000;
    float fy = (float)y;
    int z = DAT_10210fc0[route].points[point].y / 10000;
    if (z > 5000)
        z -= 10000;
    float fz = (float)z;
    int value = DAT_10210fc0[route].points[point].y % 10000;
    if (value > 5000)
        value -= 10000;
    *outValue = value;
    *outType = DAT_10210fc0[route].points[point].z;
    int parameter = DAT_10210fc0[route].points[point].code;
    *outParam = parameter;
    if (fx - 20.0f < position[0] && fx + 20.0f > position[0] && fy - 20.0f < position[1] &&
        fy + 20.0f > position[1] && fz - 20.0f < position[2] && fz + 20.0f > position[2])
    {
        switch (*outType)
        {
        case 0:
            if (DAT_1020b430[walker].walkingBack != 0)
                point--;
            else
                point++;
            break;
        case 1:
            if (DAT_10874848.TestScriptFlag(*outValue) != 0)
                point = parameter;
            else if (DAT_1020b430[walker].walkingBack != 0)
                point--;
            else
                point++;
            break;
        case 2:
            result = 1;
            if (parameter != 0)
            {
                DAT_1020b430[walker].walkingBack = 1;
                point--;
            }
            else
                point = 0;
            break;
        case 3:
            if (DAT_1020b430[walker].wait == 0)
                DAT_1020b430[walker].wait = parameter;
            if (--DAT_1020b430[walker].wait < 1)
            {
                DAT_1020b430[walker].wait = 0;
                if (DAT_1020b430[walker].walkingBack != 0)
                    point--;
                else
                    point++;
            }
            break;
        case 4:
            result = 1;
            DAT_1020b430[walker].walkingBack = 0;
            point++;
            break;
        }
        int packed = DAT_10210fc0[route].points[point].x;
        fx = (float)FUN_100286b0(packed / 10000);
        fy = (float)FUN_100286b0(packed % 10000);
        fz = (float)FUN_100286b0(DAT_10210fc0[route].points[point].y / 10000);
    }
    *outPoint = point;
    position[0] = fx;
    position[1] = fy;
    position[2] = fz;
    return result;
}
