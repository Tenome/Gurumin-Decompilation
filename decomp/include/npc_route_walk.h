// Npc_WalkRoute (npc_route_walk.cpp, behaviours 2, 4 and 0x15 of the dispatch 0x100dfcf0) and the routines it calls
// that no other shared header declares.
#ifndef GURUMIN_NPC_ROUTE_WALK_H
#define GURUMIN_NPC_ROUTE_WALK_H

#include "vec4.h"

// 0x100b2900 (Npc_WalkRoute): one frame of NPC `index`. `watchesPlayer` is 1 for behaviour 4 (it also notices the
// player). Returns 0.
extern "C" int FUN_100b2900(int index, int watchesPlayer);

// 0x10043cc0 (Char_ResetState): puts the character back to its initial behaviour state.
extern "C" int FUN_10043cc0(int index);

// 0x10043e20 (Char_CanSee): 1 when `eye` lies within `range` of `target` and inside the view cone of `angleRange`
// around `heading` (or within `nearRange`), with no wall between when `checkWalls` is set.
extern "C" int FUN_10043e20(const Vec4 *target, const Vec4 *eye, float heading, float range, float angleRange,
                            float nearRange, int checkWalls);

// 0x1001b670 (Enemies_WakeNearby): wakes the sleeping enemies within `range` of character `index`. The `count` extra
// arguments are the behaviours it leaves alone.
extern "C" int FUN_1001b670(int index, float range, int count, ...);

// 0x1002f320 (Route_Follow, route_follow.cpp): reads route point `point` of map record `route` for walker `walker`;
// see route_follow.cpp for the outputs. Returns 1-3 when the walker reached a point with that action, else 0.
extern "C" int FUN_1002f320(int walker, int route, int point, int *outPoint, float *position, int *outAction,
                            int *outValue, int *outGroup, int radius);

// 0x10049990 (Map_RouteLength) and 0x10049b00 (Map_RoutePosition), map_route.cpp.
extern "C" float FUN_10049990(int map, int costScale);
extern "C" int FUN_10049b00(int map, int part, float t, Vec4 *out, int costScale);

#endif
