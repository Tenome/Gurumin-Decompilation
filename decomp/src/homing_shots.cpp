// The homing shots of the kind 0x43 enemies: each frame every live shot of the first such enemy flies on, steers at the
// player and starts its effects, and ends when it hits the player or the map or runs out of life.

#include <math.h>
#include "vec4.h"

#include "matrix4.h"

#include "anim_model.h"

#include "node.h"

#include "effect_manager.h"

#include "char_slot.h"

#include "char_state.h"

#include "char_tables.h"

#include "last_position.h"

#include "map_vars.h"

#include "char_vars.h"

#include "homing_shots.h"

#define SHOT_PITCH 0
#define SHOT_YAW 1
#define SHOT_SPEED 2

inline float Atan2(float y, float x)
{
    return (float)atan2(y, x);
}

// 0x1003fc40 (Kind43_UpdateShots). Finds the first enemy slot (0x3a..0x5b) of kind 0x43; without one nothing
// happens. Each live shot of its six shot slots loses a frame of life and speeds up by 0.4 to at most 30. Once its life is
// below 285 it steers at the player (70 units above the feet): its pitch eases towards the angle up or down to the
// player by 0.06 a step up or 0.0125 down, its heading by 0.025 towards the player's bearing. It then moves along
// that direction, starts the effects 0x7c9 (every frame), 0x26 (unless the life is a multiple of 4) and 0x7cb (every
// third frame, 80 units behind) on a matrix of that direction, and ends (state 0, burst effect 0x36cf) when its life
// is over, when it is within 60 units horizontally and 120 vertically of the player, or when a 30-unit sphere at
// it touches the map. Returns 0.
int Kind43_UpdateShots(void)
{
    int owner;
    for (owner = SHOOTER_SLOT_FIRST; owner < SHOOTER_SLOT_END; owner += 2)
    {
        if (DAT_10daaae8[owner].kind == KIND_HOMING_SHOOTER)
            break;
        if (DAT_10daaae8[owner + 1].kind == KIND_HOMING_SHOOTER)
        {
            owner++;
            break;
        }
    }
    if (owner != SHOOTER_SLOT_END)
    {
        Vec4 player = DAT_10db6ee8[0].matrix.GetTranslation();
        Matrix4 shotMatrix;
        Matrix4 queryMatrix;
        Matrix4 trailMatrix;
        Vec4 forward;
        Vec4 normal;
        EffectOptions burst;
        player.z += SHOT_AIM_RAISE;
        burst.m_0000 = 0;
        burst.m_0004 = DAT_10daaae8[owner].stats[STAT_EFFECT_PARAM];
        burst.m_0008 = SHOT_EFFECT_SCALE;
        burst.m_000e = SHOT_EFFECT_SETTING;
        queryMatrix.SetIdentity();
        trailMatrix.SetIdentity();
        for (int slot = owner + SHOT_SLOT_FIRST; slot < owner + SHOT_SLOT_END; slot++)
        {
            if (DAT_1020b430[slot].state == 0)
                continue;
            DAT_1020b430[slot].values[0]--;
            if (DAT_10255160[slot][SHOT_SPEED] < SHOT_MAX_SPEED)
                DAT_10255160[slot][SHOT_SPEED] += SHOT_ACCELERATION;
            float distance = (player - DAT_10802bf8[slot].position).Length();
            float distance2D = (player - DAT_10802bf8[slot].position).Length2D();
            float pitchGoal;
            if (distance > SHOT_MIN_AIM_DISTANCE)
                pitchGoal = (float)acos(distance2D / distance);
            else
                pitchGoal = 0.0f;
            if (player.z > DAT_10802bf8[slot].position.z)
                pitchGoal = pitchGoal * -1.0f;
            if (DAT_1020b430[slot].values[0] < SHOT_STRAIGHT_LIFE)
            {
                if (pitchGoal < DAT_10255160[slot][SHOT_PITCH])
                {
                    DAT_10255160[slot][SHOT_PITCH] -= SHOT_PITCH_FALL;
                    if (pitchGoal > DAT_10255160[slot][SHOT_PITCH])
                        DAT_10255160[slot][SHOT_PITCH] = pitchGoal;
                }
                else if (pitchGoal > DAT_10255160[slot][SHOT_PITCH])
                {
                    DAT_10255160[slot][SHOT_PITCH] += SHOT_PITCH_RISE;
                    if (pitchGoal < DAT_10255160[slot][SHOT_PITCH])
                        DAT_10255160[slot][SHOT_PITCH] = pitchGoal;
                }
                float yawDelta = Atan2(player.x - DAT_10802bf8[slot].position.x,
                                       player.y - DAT_10802bf8[slot].position.y) + SHOT_ANGLE_HALF_TURN - DAT_10255160[slot][SHOT_YAW];
                for (; yawDelta > SHOT_ANGLE_HALF_TURN; yawDelta -= SHOT_ANGLE_FULL_TURN)
                {
                }
                for (; yawDelta < -SHOT_ANGLE_HALF_TURN; yawDelta += SHOT_ANGLE_FULL_TURN)
                {
                }
                if (yawDelta > 0.0f)
                    DAT_10255160[slot][SHOT_YAW] += SHOT_YAW_STEP;
                else if (yawDelta < 0.0f)
                    DAT_10255160[slot][SHOT_YAW] -= SHOT_YAW_STEP;
            }
            forward.y = -DAT_10255160[slot][SHOT_SPEED];
            forward.RotateX(-DAT_10255160[slot][SHOT_PITCH]);
            forward.RotateZ(-DAT_10255160[slot][SHOT_YAW]);
            DAT_10802bf8[slot].position.Copy(DAT_10802bf8[slot].position + forward);
            forward.Fill(0.0f);
            shotMatrix = RotationX(-DAT_10255160[slot][SHOT_PITCH]) * RotationZ(DAT_10255160[slot][SHOT_YAW]);
            shotMatrix.SetTranslation(DAT_10802bf8[slot].position);
            DAT_10daaa80.Start(SHOT_EFFECT_MAIN, forward, -1, -1, 0, 0, 0, &shotMatrix, 0, &DAT_10219770[slot]);
            if (DAT_1020b430[slot].values[0] % 4 != 0)
                DAT_10daaa80.Start(SHOT_EFFECT_SPARK, forward, -1, -1, 0, 0, 0, &shotMatrix, 0, &DAT_10219770[slot]);
            if (DAT_1020b430[slot].values[0] % SHOT_TRAIL_PERIOD == 0)
            {
                trailMatrix = RotationX(-DAT_10255160[slot][SHOT_PITCH]) * RotationZ(DAT_10255160[slot][SHOT_YAW]);
                forward.y = SHOT_TRAIL_OFFSET;
                forward.RotateX(-DAT_10255160[slot][SHOT_PITCH]);
                forward.RotateZ(-DAT_10255160[slot][SHOT_YAW]);
                trailMatrix.SetTranslation(DAT_10802bf8[slot].position + forward);
                forward.Fill(0.0f);
                DAT_10daaa80.Start(SHOT_EFFECT_TRAIL, forward, -1, -1, 0, 0, 0, &trailMatrix, 0, &DAT_10219770[slot]);
                trailMatrix.SetIdentity();
            }
            queryMatrix.SetTranslation(DAT_10802bf8[slot].position);
            if (DAT_1020b430[slot].values[0] == 0 ||
                (distance2D < SHOT_HIT_RADIUS && player.z < DAT_10802bf8[slot].position.z + SHOT_HIT_HEIGHT &&
                 DAT_10802bf8[slot].position.z < player.z + SHOT_HIT_HEIGHT) ||
                (DAT_10255f08 != 0 &&
                 DAT_10255f08->Query(&queryMatrix, &DAT_10802bf8[slot].position, SHOT_WALL_RADIUS, &normal, &distance2D, 0, 0) != 0))
            {
                DAT_1020b430[slot].state = 0;
                shotMatrix.SetIdentity();
                shotMatrix.SetTranslation(DAT_10802bf8[slot].position);
                DAT_10daaa80.Start(SHOT_EFFECT_BURST, forward, -1, -1, 0, 0, 0, &shotMatrix, &burst, &DAT_10219770[slot]);
            }
        }
    }
    return 0;
}
