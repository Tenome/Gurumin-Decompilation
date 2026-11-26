// Collision queries against the world (DAT_10255f08).
#include "node.h"
#include "map_vars.h"
#include "world.h"

#include "vec4.h"

#include "matrix4.h"
#include "anim_model.h"
#include "char_state.h"


// 0x1000ff80: 1 when a sphere of `radius` at the point (x, y, z) touches the world collision.
int Collision_ProbePoint(float x, float y, float z, float radius)
{
    Matrix4 matrix;
    matrix.SetIdentity();
    Vec4 point;
    Vec4 hit;
    float depth;
    point.Set(x, y, z, 0);
    return DAT_10255f08->Query(&matrix, &point, radius, &hit, &depth, 0, 0);
}

// Casts a ray from `from` to `to` against world collision (kind 4, mask 0x4000000) of the nodes called `name`.
// Returns 1 on a hit and stores the hit point in `hit` unless `testOnly` is set; 0 when nothing is hit or no world
// is loaded.
extern "C" int FUN_100246b0(const Vec4 *from, const Vec4 *to, Vec4 *hit, int testOnly, const char *name)
{
    Vec4 point;
    if (testOnly != 0)
    {
        if (DAT_10255f08 != 0 && DAT_10255f08->RayCast(name, 4, 0x4000000, to, from, &point, 0) != 0)
            return 1;
    }
    else
    {
        if (DAT_10255f08 != 0 && DAT_10255f08->RayCast(name, 4, 0x4000000, to, from, &point, 0) != 0)
        {
            hit->Copy(point);
            return 1;
        }
    }
    return 0;
}

// 0x10024c40. Ledge test: 1 when there is world collision (mask 0x4000000) below the point `distance` units along
// `direction` from `position`. The ray runs from 90 units above that point to `depth` units below it. Enemy AI
// uses it (depth 300, distance 15) to stop at edges. Returns 0 when nothing is hit or no world is loaded.
extern "C" int FUN_10024c40(const Vec4 *position, Vec4 *direction, float depth, int distance)
{
    Vec4 from;
    Vec4 to;
    Vec4 hit;
    from.Copy(*direction * (float)distance);
    from.Copy(from + *position);
    to.Copy(from);
    from.z += 90.0f;
    to.z -= depth;
    if (DAT_10255f08 != 0 && DAT_10255f08->Collide(0, 0x4000000, &from, &to, &hit) != 0)
        return 1;
    return 0;
}

// 0x100248e0 (Collision_ProbeDown). Casts a ray down from `height` units above `position` to `position` against the
// world collision (mask 0x4000000) and stores the result in `ground`. Returns 1 on a hit, 0 when nothing was hit
// (`ground` then has x = -200000) or no world is loaded. Note: on a hit the code copies `unused` (a zero vector)
// over the hit point and computes a length it throws away, so callers only learn whether something was hit.
extern "C" int FUN_100248e0(Vec4 position, Vec4 *ground, float height)
{
    Vec4 from;
    Vec4 unused;
    Vec4 hit;
    float zero = 0;
    hit.Fill(zero);
    hit.x = -200000.0f;
    from.Copy(position);
    from.z += height;
    if (DAT_10255f08 != 0 && DAT_10255f08->CastSegment(0, 0x4000000, &from, &position, &hit, 0) != 0)
    {
        hit.Copy(unused);
        (hit - position).Length();
    }
    ground->Copy(hit);
    if (hit.x == -200000.0f)
        return 0;
    return 1;
}

#include "last_position.h"

#include "char_tables.h"
#include "char_vars.h"

// 0x1001a450 (Slot_HasNoGround). Every tenth frame (spread over the slots by slot number) casts a ray at the point
// 15 units along slot `slot`'s vector from `point`, from 300 below to 50 above it, against the world collision
// (mask 0x4000000) filtered by the slot's nearCells, and records the hit point in DAT_10864cf8 or that there is no
// ground. Returns 1 when the slot has no ground below it (from this or an earlier test), else 0.
extern "C" int FUN_1001a450(int slot, const Vec4 *point)
{
    if (DAT_104c0380 % 10 == slot % 10)
    {
        Vec4 low;
        Vec4 high;
        Vec4 hit;
        low.Copy(DAT_10802bf8[slot].position * 15.0f);
        low.Copy(low + *point);
        high.Copy(low);
        high.z += 50.0f;
        low.z -= 300.0f;
        DAT_11b002c0 = (int)DAT_10db6ee8[slot].nearCells;
        if (DAT_10255f08 != 0 && DAT_10255f08->CastSegment(0, 0x4000000, &low, &high, &hit, 0) != 0)
        {
            DAT_10864cf8[slot].Copy(hit);
            DAT_10467920[slot] = 0;
        }
        else
        {
            DAT_10467920[slot] = 1;
        }
        DAT_11b002c0 = 0;
    }
    return DAT_10467920[slot] != 0;
}

// 0x100242c0 (Char_ProbeAhead). Casts a ray from character `index` (from its "Bip01 Head" node when `fromHead` is
// set, or its position when it has none; from its position otherwise), moved by `offset` when given, to the point
// `depth` units ahead of it (along -y turned by its heading), against world collision of kind 4 (mask 0x4000000). With `testOnly` only whether something is hit is returned; otherwise the hit
// point goes to `ground` (x = -200000 when there is none). Returns 1 on a hit, else 0.
extern "C" int FUN_100242c0(int index, Vec4 *ground, float depth, int fromHead, int testOnly, const Vec4 *offset)
{
    Vec4 from;
    Vec4 to;
    Vec4 unused;
    Vec4 hit;
    float zero = 0;
    hit.Fill(zero);
    hit.x = -200000.0f;
    if (fromHead != 0)
    {
        Node *head = DAT_10db6ee8[index].root->Find("Bip01 Head", 0);
        if (head != 0)
            from.Copy(head->worldMatrix.GetTranslation());
        else
            from.Copy(DAT_10db6ee8[index].matrix.GetTranslation());
    }
    else
    {
        from.x = DAT_10db6ee8[index].matrix._41;
        from.y = DAT_10db6ee8[index].matrix._42;
        from.z = DAT_10db6ee8[index].matrix._43;
        from.w = DAT_10db6ee8[index].matrix.m[15];
    }
    if (offset != 0)
        from.Copy(from + *offset);
    to.Fill(0);
    to.y -= depth;
    to.RotateZ(-DAT_10db6ee8[index].heading);
    to.Copy(to + from);
    if (testOnly != 0)
        return DAT_10255f08 != 0 && DAT_10255f08->RayCast(0, 4, 0x4000000, &to, &from, &hit, 0) != 0;
    if (DAT_10255f08 != 0 && DAT_10255f08->RayCast(0, 4, 0x4000000, &to, &from, &hit, 0) != 0)
        float distance = (hit - from).Length();   // computed and not used
    ground->Copy(hit);
    if (hit.x == -200000.0f)
        return 0;
    return 1;
}

// 0x100244b0 (Char_ProbeDirection). Char_ProbeAhead with the direction given as `angle` (the drop of `depth` along -y
// turned by it) and the collision limited to the nodes called `name` (0 = all).
extern "C" int FUN_100244b0(int index, float angle, Vec4 *ground, float depth, int fromHead, int testOnly,
                            const Vec4 *offset, const char *name)
{
    Vec4 from;
    Vec4 to;
    Vec4 unused;
    Vec4 hit;
    float zero = 0;
    hit.Fill(zero);
    hit.x = -200000.0f;
    if (fromHead != 0)
    {
        Node *head = DAT_10db6ee8[index].root->Find("Bip01 Head", 0);
        if (head != 0)
            from.Copy(head->worldMatrix.GetTranslation());
        else
            from.Copy(DAT_10db6ee8[index].matrix.GetTranslation());
    }
    else
    {
        from.x = DAT_10db6ee8[index].matrix._41;
        from.y = DAT_10db6ee8[index].matrix._42;
        from.z = DAT_10db6ee8[index].matrix._43;
        from.w = DAT_10db6ee8[index].matrix.m[15];
    }
    if (offset != 0)
        from.Copy(from + *offset);
    to.Fill(0);
    to.y -= depth;
    to.RotateZ(angle);
    to.Copy(to + from);
    if (testOnly != 0)
        return DAT_10255f08 != 0 && DAT_10255f08->RayCast(name, 4, 0x4000000, &to, &from, &hit, 0) != 0;
    if (DAT_10255f08 != 0 && DAT_10255f08->RayCast(name, 4, 0x4000000, &to, &from, &hit, 0) != 0)
        float distance = (hit - from).Length();   // computed and not used
    ground->Copy(hit);
    if (hit.x == -200000.0f)
        return 0;
    return 1;
}

// 0x10024750 (Char_ProbeGround). Casts a ray down `depth` units from character `index` (from its head node when
// `fromHead` is set and it has one, else from its position, 30 units up when `fromHead` is set), moved by `offset`
// when given, against the world collision (mask 0x4000000, or 4 with `smallObjects`). With `testOnly` only whether
// something is hit is returned; otherwise the hit point goes to `ground` (x = -200000 when there is none). Returns 1
// on a hit, else 0.
extern "C" int FUN_10024750(int index, Vec4 *ground, float depth, int fromHead, const Vec4 *offset, int testOnly,
                            int smallObjects)
{
    Vec4 from;
    Vec4 to;
    Vec4 unused;
    Vec4 hit;
    float zero = 0;
    hit.Fill(zero);
    int mask = smallObjects != 0 ? 4 : 0x4000000;
    hit.x = -200000.0f;
    if (fromHead != 0 && DAT_10db6ee8[index].headNode != 0)
    {
        from.Copy(DAT_10db6ee8[index].headNode->worldMatrix.GetTranslation());
    }
    else
    {
        from.x = DAT_10db6ee8[index].matrix._41;
        from.y = DAT_10db6ee8[index].matrix._42;
        from.z = DAT_10db6ee8[index].matrix._43;
        from.w = DAT_10db6ee8[index].matrix.m[15];
        if (fromHead != 0)
            from.z += 30.0f;
    }
    if (offset != 0)
        from.Copy(from + *offset);
    to.Copy(from);
    to.z -= depth;
    if (testOnly != 0)
    {
        if (DAT_10255f08 != 0 && DAT_10255f08->Collide(0, mask, &to, &from, &hit) != 0)
            return 1;
        return 0;
    }
    if (DAT_10255f08 != 0)
        DAT_10255f08->CastSegment(0, mask, &to, &from, &hit, 0);
    if (ground != 0)
        ground->Copy(hit);
    if (hit.x == -200000.0f)
        return 0;
    return 1;
}
