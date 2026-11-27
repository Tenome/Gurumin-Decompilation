// A character walking its route: each frame the next route target is looked up (0x100286d0) and the character
// steps towards it at walking or running speed.

#include "vec4.h"

#include "matrix4.h"

#include "anim_model.h"

#include "last_position.h"

#include "char_state.h"

#include "char_tables.h"
#include "char_vars.h"

// 0x100286d0: the route target of character `slot` heading for `point` (writes the next point, the target and the
// point's action, value and group).
extern "C" int FUN_100286d0(int slot, int *state, int point, int *nextPoint, Vec4 *target, int *group, int *value,
                            int *action);

// Looks up the route target of character `slot` for its current route point and moves the point on (the route
// lookup also reports the point's group, value and action, which this caller does not need). On the first call the
// character is placed at the target and listed in DAT_1032d5b8. Written as a helper expanded at its one call site:
// the lookup's output variables then live only for this part of Char_WalkRoute.
static __forceinline void LookUpRouteTarget(int slot, int *state, Vec4 *target)
{
    int next;
    int group;
    int value;
    int action;
    FUN_100286d0(slot, state, DAT_1020b430[slot].targetPoint, &next, target, &group, &value, &action);
    if (DAT_1020b430[slot].placed == 0)
    {
        DAT_1032d5b8[DAT_1049f0f0] = &DAT_10db6ee8[slot].matrix.translation;
        DAT_1021bbc0[DAT_1049f0f0] = 0;
        DAT_1049f0f0++;
        DAT_1020b430[slot].placed = 1;
        DAT_10db6ee8[slot].SetPosition(target->x, target->y, target->z);
    }
    DAT_1020b430[slot].targetPoint = next;
}

// 0x1002a110 (Char_WalkRoute). Moves character `slot` one step along its route: it is unlocked and flagged as
// walking, the route target looked up (on the first call the character is placed at it and listed in
// DAT_1032d5b8), and unless it is stopped it steps 5 units (12 with `run`) towards the target. Returns 0.
extern "C" int FUN_1002a110(int slot, int run)
{
    Vec4 target;
    Vec4 step;
    int state = 0;
    target.Copy(DAT_10db6ee8[slot].matrix.GetTranslation());
    DAT_10db6ee8[slot].mode = 0;
    DAT_10db6ee8[slot].walkingRoute = 1;
    DAT_10db6ee8[slot].motionFrame = 0x1e;
    DAT_10db6ee8[slot].M_10161be0(0, 0);
    LookUpRouteTarget(slot, &state, &target);
    DAT_10802bf8[slot].step.Fill(0.0f);
    if (DAT_1020b430[slot].routeStopped == 0)
    {
        step.Copy(target - DAT_10db6ee8[slot].matrix.GetTranslation());
        if (step.x != 0.0f || step.y != 0.0f || step.z != 0.0f)
            step.Normalized3();
        if (run != 0)
            step.Copy(step * 12.0f);
        else
            step.Copy(step * 5.0f);
        DAT_10db6ee8[slot].matrix.m[12] += step.x;
        DAT_10db6ee8[slot].matrix.m[13] += step.y;
        DAT_10db6ee8[slot].matrix.m[14] += step.z;
        DAT_10802bf8[slot].step.Copy(step);
        DAT_10db6ee8[slot].velocity.Fill(0.0f);
    }
    DAT_10db6ee8[slot].velocity.Fill(0.0f);
    return 0;
}
