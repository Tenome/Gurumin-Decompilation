// NPCs that walk one of the map's route records (behaviours 2, 4 and 0x15 of Char_RunBehaviour 0x100dfcf0). The
// talk id is the start point * 100 + the route record. The first point of the record holds the walker's settings:
// x = units per frame, y = the prompt kind and the fixed facing, z = the walking style, code = the point actions that
// reset it.

#include <math.h>

#include "last_position.h"
#include "vec4.h"

#include "matrix4.h"

#include "anim_model.h"

#include "char_state.h"

#include "route_point.h"

#include "map_records.h"
#include "char_tables.h"
#include "char_vars.h"
#include "world.h"
#include "field_item_menu.h"
#include "doppler_loops.h"
#include "math_float.h"
#include "enemy_chase_hop.h"
#include "char_hikari.h"
#include "npc_route_walk.h"

// Sight check of the watching variant (behaviour 4), as in Npc_PatrolRoute.
#define WALK_SIGHT_RANGE 750.0f
#define WALK_SIGHT_ANGLE 1.099525f
#define WALK_SIGHT_NEAR_RANGE 100.0f
#define WALK_HEAD_HEIGHT 75.0f          // its head above its origin
#define WALK_PLAYER_HEAD_HEIGHT 50.0f   // the player's head above its origin
#define WALK_POINT_RADIUS 20            // how close counts as reaching a route point
#define WALK_WAKE_RANGE 1500.0f         // enemies this close wake up when it notices the player
#define WALK_WAKE_EXCEPTIONS 2
#define WALK_ALERT_VOICE 0x20c
#define WALK_ALERT_PROMPT 15
#define WALK_ALERT_PROMPT_FRAMES 60
#define WALK_ALERT_HEARING_RANGE_SQUARED 6250000.0f   // 2500 units
#define WALK_FIRST_ENEMY_SLOT 0x3a      // slots below it always play the alert voice
#define WALK_TOLD_TO_REACT 1            // sawPlayer: 1 = noticed the player or was told to react, 2 = turned to it

// The key prompts the walker shows now and then, picked by the record's prompt kind.
#define WALK_PROMPT_KIND_A 0
#define WALK_PROMPT_KEY_A 13
#define WALK_PROMPT_KIND_B 1
#define WALK_PROMPT_KEY_B 0x7db
#define WALK_PROMPT_KIND_C 2
#define WALK_PROMPT_KEY_C 7
#define WALK_PROMPT_FRAMES 60
#define WALK_PROMPT_MIN_AGE 60          // frames between prompts at least
#define WALK_PROMPT_RANDOM_RANGE 10000

// Behaviour states.
#define WALK_STATE_START 0              // reads the route and the record settings
#define WALK_STATE_STAND 1              // stands at a point turning towards its facing, then waits `value` frames
#define WALK_STATE_WALK 2               // walks to the next point
#define WALK_STATE_TURN_TO_PLAYER 4     // noticed the player: turns to it, then resets after 30 frames
#define WALK_TURN_TO_PLAYER_FRAMES 30

#define WALK_STYLE_FRACTION 0           // walkStyle: walks along the whole route by the frame counter
#define WALK_ARRIVED_DISTANCE_SQUARED 50.0f
#define WALK_RUN_SPEED_SQUARED 100.0f   // squared step above which it runs (motion 1)
#define WALK_STEP_SPEED_SQUARED 2.0f    // squared step above which it walks (motion 0), else it stands (motion 2)
#define WALK_FRACTION_STEP_SCALE 1.25f
#define WALK_FACE_TOLERANCE 0.12217f
#define WALK_TURN_TOLERANCE 0.01745f
#define WALK_QUARTER_TURN_DEGREES 90
#define WALK_BLOCKED_SPEED 32.0f
#define WALK_PI 3.1415f
#define WALK_TWO_PI 6.283f

// atan2 on floats (an inline helper: both arguments are rounded to float first, the second one first).
inline float Atan2(float y, float x)
{
    return (float)atan2(y, x);
}

// 0x100b2900 (Npc_WalkRoute). One frame of an NPC walking a route record. Phases (CharState::state): 0 reads the
// route (talk id = start point * 100 + record), speed, walking style, fixed facing and prompt kind from the
// record's first point; 1 stands at the current point turning towards its facing (the player, or a quarter turn
// count) and waits for the point's value in frames; 2 walks to the next point (walking style 0 follows the whole
// route by the frame counter, any other style walks point to point with Route_Follow), changing the walk motion by
// speed and speeding the motion up when the map blocks it; 4 faces the player for 30 frames and then resets.
// With `watchesPlayer` (behaviour 4) it also checks every frame whether it sees the player (or was told to react):
// it then wakes the enemies around, plays voice 0x20c with a prompt and switches to phase 4. Every so often a key
// prompt appears over it (once the first one fired, a chance rising with the frames since the last). Returns 0.
extern "C" int FUN_100b2900(int index, int watchesPlayer)
{
    Vec4 player;
    Vec4 self;
    Vec4 target;
    player.Copy(DAT_10db6ee8[0].matrix.GetTranslation());
    self.Copy(DAT_10db6ee8[index].matrix.GetTranslation());
    target.Copy(self);
    int point;
    int action;
    int value;
    int group;
    int reached;
    if (watchesPlayer != 0)
    {
        Vec4 head;
        Vec4 eye;
        head.Copy(self);
        head.z += WALK_HEAD_HEIGHT;
        eye.Copy(player);
        eye.z += WALK_PLAYER_HEAD_HEIGHT;
        DAT_1020b430[index].sawPlayer |= FUN_10043e20(&head, &eye, DAT_10db6ee8[index].heading, WALK_SIGHT_RANGE,
                                                       WALK_SIGHT_ANGLE, WALK_SIGHT_NEAR_RANGE, 1);
    }
    if (DAT_10210b30[index] != 0 && DAT_1020b430[index].sawPlayer == 0)
        DAT_1020b430[index].sawPlayer = WALK_TOLD_TO_REACT;
    if (DAT_1020b430[index].sawPlayer == WALK_TOLD_TO_REACT)
    {
        int behaviour = DAT_1020b430[index].behaviour;
        DAT_1020b430[index].state = WALK_STATE_TURN_TO_PLAYER;
        DAT_1020b430[index].wait = 0;
        FUN_1001b670(index, WALK_WAKE_RANGE, WALK_WAKE_EXCEPTIONS, behaviour, 2);
        if (DAT_104bf8f8[index] == 0 || DAT_1021d138[index] != WALK_ALERT_PROMPT)
        {
            DAT_104bf8f8[index] = 1;
            DAT_1021d138[index] = WALK_ALERT_PROMPT;
        }
        DAT_1021f740[index] = WALK_ALERT_PROMPT_FRAMES;
        float squaredDistance = (DAT_10db6ee8[index].matrix.GetTranslation() - DAT_118e6cb8).LengthSquared3();
        if (squaredDistance < WALK_ALERT_HEARING_RANGE_SQUARED || index < WALK_FIRST_ENEMY_SLOT)
        {
            Vec4 position;
            position.Set(DAT_10db6ee8[index].matrix.m[12], DAT_10db6ee8[index].matrix.m[13],
                         DAT_10db6ee8[index].matrix.m[14], 0);
            FUN_1003ad20(WALK_ALERT_VOICE, &position, 1.0f);
        }
    }
    DAT_10210b30[index] = 0;
    if (DAT_1020b430[index].promptTimer != 0 && ++DAT_1020b430[index].promptTimer > WALK_PROMPT_MIN_AGE &&
        FUN_1013f880() % WALK_PROMPT_RANDOM_RANGE < DAT_1020b430[index].promptTimer)
    {
        if (DAT_1020b430[index].promptKind == WALK_PROMPT_KIND_A)
        {
            FUN_1000d620(index, WALK_PROMPT_KEY_A, WALK_PROMPT_FRAMES);
            DAT_1020b430[index].promptTimer = 1;
        }
        else if (DAT_1020b430[index].promptKind == WALK_PROMPT_KIND_B)
        {
            FUN_1000d620(index, WALK_PROMPT_KEY_B, WALK_PROMPT_FRAMES);
            DAT_1020b430[index].promptTimer = 1;
        }
        else
        {
            if (DAT_1020b430[index].promptKind == WALK_PROMPT_KIND_C)
                FUN_1000d620(index, WALK_PROMPT_KEY_C, WALK_PROMPT_FRAMES);
            DAT_1020b430[index].promptTimer = 1;
        }
    }
    if (DAT_1020b430[index].state == WALK_STATE_START)
    {
        int route = DAT_1020b430[index].talk % 100;
        int start = DAT_1020b430[index].talk / 100;
        DAT_1020b430[index].walkerIdle = 0;
        DAT_1020b430[index].route = route;
        DAT_1020b430[index].nextPoint = start;
        FUN_1002f320(index, route, start, &point, &target.x, &action, &value, &group, WALK_POINT_RADIUS);
        route = DAT_1020b430[index].talk % 100;
        DAT_1020b430[index].nextPoint = point;
        DAT_1020b430[index].state = WALK_STATE_WALK;
        DAT_1020b430[index].wait = 0;
        DAT_1020b430[index].firstPoint = point;
        DAT_1020b430[index].speed = DAT_10210fc0[route].points[0].x;
        DAT_1020b430[index].walkStyle = DAT_10210fc0[route].points[0].z % 10;
        DAT_1020b430[index].fadeActions = DAT_10210fc0[route].points[0].code;
        DAT_1020b430[index].fixedFacing = DAT_10210fc0[route].points[0].y % 100 / 10;
        DAT_10802bf8[index].position.Fill(0.0f);
        int promptKind = DAT_10210fc0[DAT_1020b430[index].talk % 100].points[0].y % 10;
        if (promptKind == 0)
            return 0;
        DAT_1020b430[index].promptKind = promptKind - 1;
        DAT_1020b430[index].promptTimer = 1;
        return 0;
    }
    if (DAT_1020b430[index].state == WALK_STATE_STAND)
    {
        FUN_1002f320(index, DAT_1020b430[index].route, DAT_1020b430[index].nextPoint, &point, &target.x, &action,
                     &value, &group, WALK_POINT_RADIUS);
        DAT_1020b430[index].nextPoint = point;
        float facing;
        if (DAT_1020b430[index].fixedFacing == 0)
            facing = Atan2(target.x - self.x, target.y - self.y) + WALK_PI;
        else
            facing = (float)(DAT_1020b430[index].fixedFacing * WALK_QUARTER_TURN_DEGREES) * WALK_PI / 180.0f;
        DAT_10db6ee8[index].targetHeading = facing;
        float turn = facing - DAT_10db6ee8[index].heading;
        while (turn > WALK_PI)
            turn -= WALK_TWO_PI;
        while (turn < -WALK_PI)
            turn += WALK_TWO_PI;
        if (turn <= WALK_FACE_TOLERANCE)
        {
            if (turn >= -WALK_FACE_TOLERANCE)
                DAT_10db6ee8[index].PlayMotion(2, 0, 5, 1.0f);
            else
                DAT_10db6ee8[index].PlayMotion(group != 0 ? 1 : 0, 0, 5, 1.0f);
        }
        else
            DAT_10db6ee8[index].PlayMotion(group != 0 ? 1 : 0, 0, 5, 1.0f);
        if (DAT_1020b430[index].wait == 0)
        {
            DAT_1020b430[index].standing = 1;
            DAT_1020b430[index].wait = 1;
            return 0;
        }
        if (++DAT_1020b430[index].wait < value)
            return 0;
        DAT_1020b430[index].state = WALK_STATE_WALK;
        DAT_1020b430[index].wait = 0;
        return 0;
    }
    if (DAT_1020b430[index].state == WALK_STATE_WALK)
    {
        Vec4 step;
        Vec4 ahead;
        DAT_1020b430[index].standing = 0;
        if (DAT_1020b430[index].walkStyle == WALK_STYLE_FRACTION)
        {
            int scale = DAT_1020b430[index].speed < 2 ? 1 : DAT_1020b430[index].speed;
            float length = FUN_10049990(DAT_1020b430[index].route, scale);
            float period = length / (float)scale <= 1.0f ? 1.0f : length / (float)scale;
            int arrived = 0;
            int frame = DAT_104c0380 % (int)period;
            FUN_10049b00(DAT_1020b430[index].route, DAT_1020b430[index].firstPoint, (float)frame / period, &target,
                         scale);
            step.Copy(target - self);
            if ((DAT_10802bf8[index].position - target).LengthSquared3() < WALK_ARRIVED_DISTANCE_SQUARED)
                arrived = 1;
            DAT_10802bf8[index].position.Copy(target);
            step.LengthSquared3();
            if (arrived == 0)
            {
                step.z = 0;
                step.Normalized3();
                step.Copy(step * (float)DAT_1020b430[index].speed * WALK_FRACTION_STEP_SCALE);
                DAT_10db6ee8[index].targetHeading = FUN_10001f80(step.x, step.y) + WALK_PI;
                float speedSquared = step.LengthSquared3();
                if (speedSquared > WALK_RUN_SPEED_SQUARED)
                    DAT_10db6ee8[index].PlayMotion(1, 0, 5, 1.0f);
                else if (speedSquared >= WALK_STEP_SPEED_SQUARED)
                    DAT_10db6ee8[index].PlayMotion(0, 0, 5, 1.0f);
                else
                    DAT_10db6ee8[index].PlayMotion(2, 0, 5, 1.0f);
            }
            else
            {
                DAT_10db6ee8[index].PlayMotion(2, 0, 5, 1.0f);
                step.Fill(0.0f);
            }
        }
        else
        {
            reached = FUN_1002f320(index, DAT_1020b430[index].route, DAT_1020b430[index].nextPoint, &point,
                                   &target.x, &action, &value, &group, WALK_POINT_RADIUS);
            DAT_10db6ee8[index].targetHeading = FUN_10001f80(target.x - self.x, target.y - self.y) + WALK_PI;
            DAT_10db6ee8[index].PlayMotion(group != 0 ? 1 : 0, 0, 5, 1.0f);
            if (DAT_1020b430[index].nextPoint != point)
            {
                DAT_1020b430[index].state = WALK_STATE_STAND;
                DAT_1020b430[index].wait = 0;
                if (((1 << reached) & DAT_1020b430[index].fadeActions) != 0)
                    FUN_10043cc0(index);
            }
            step.Copy(target - self);
            step.z = 0;
            step.Normalized3();
            step.Copy(step * (float)DAT_1020b430[index].speed);
        }
        FUN_100241e0(index, step.x, step.y, 0);
        ahead.Copy(step);
        ahead.z += 10.0f;
        ahead.Copy(ahead * 8.0f);
        step.z += 40.0f;
        if (DAT_10db6ee8[index].m_238c == 0)
            return 0;
        if (DAT_10db6ee8[index].IsClearOfMapLimited(&step) == 0 || DAT_10db6ee8[index].IsClearOfMapLimited(&ahead) == 0)
            DAT_10db6ee8[index].AddMotionSpeed(WALK_BLOCKED_SPEED);
        return 0;
    }
    if (DAT_1020b430[index].state != WALK_STATE_TURN_TO_PLAYER)
        return 0;
    DAT_1020b430[index].sawPlayer = 2;
    float facing = FUN_10001f80(player.x - self.x, player.y - self.y) + WALK_PI;
    float turn = DAT_10db6ee8[index].heading;
    DAT_10db6ee8[index].targetHeading = facing;
    DAT_1020b430[index].wait++;
    turn -= facing;
    while (turn > WALK_PI)
        turn -= WALK_TWO_PI;
    while (turn < -WALK_PI)
        turn += WALK_TWO_PI;
    DAT_10db6ee8[index].PlayMotion(turn >= WALK_TURN_TOLERANCE ? 0 : 2, 0, 5, 1.0f);
    if (DAT_1020b430[index].wait == WALK_TURN_TO_PLAYER_FRAMES)
        FUN_10043cc0(index);
    return 0;
}
