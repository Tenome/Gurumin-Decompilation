// A character that drifts or circles in the air (behaviour of kind 0x0b, run from the per-kind update 0x100f9670). Its
// first frame picks, from the digits of its own position, either a straight drift (state 0: a random step per axis
// that flips when the way is blocked) or a slow circle (state 1: three spin angles that grow every frame and turn a
// small vector round, put on top of a centre kept in whole units). The "@O_057:Layer" node is turned round x by an angle
// that grows with the speed it moves at.

#include <math.h>
#include <stdlib.h>

#include "vec4.h"

#include "matrix4.h"

#include "anim_model.h"

#include "node.h"

#include "last_position.h"

#include "char_state.h"

#include "char_slot.h"

#include "char_tables.h"

#include "char_vars.h"

#include "char_ground.h"

#include "world.h"

#include "enemy_chase_hop.h"

#include "drifter.h"

Matrix4 TranslationMatrix(float x, float y, float z);   // 0x101b1360

inline float Atan2(float y, float x)
{
    return (float)atan2(y, x);
}

#define DRIFT_STATE_LINE 0
#define DRIFT_STATE_ORBIT 1
#define DRIFT_SCALE_PER_LEVEL 0.1f
#define DRIFT_PROBE_DEPTH 3000.0f
#define DRIFT_PROBE_LIFT 10.0f
#define DRIFT_DIGIT_RANGE 10
#define DRIFT_DIGIT_CENTRE 5.0f
#define DRIFT_LINE_STEP 4.0f
#define DRIFT_ORBIT_ROUTE_RECORD 100
#define DRIFT_BIG_LIMIT 5000000.0f
#define DRIFT_SMALL_LIMIT 500000.0f
#define DRIFT_BIG_UNIT 100000.0f
#define DRIFT_SMALL_UNIT 10000.0f
#define DRIFT_WRAP_FIRST 720000.0f
#define DRIFT_WRAP_HALF 360000.0f
#define DRIFT_ANGLE_PER_UNIT 0.174533f
#define DRIFT_SPIN_STEP 0.01745f
#define DRIFT_TWO_PI 6.2831855f
#define DRIFT_ORBIT_RADIUS 25.0f
#define DRIFT_PI 3.1415f
#define DRIFT_PHASE_LIMIT 6.283f
#define DRIFT_PHASE_LINE 0.04f
#define DRIFT_PHASE_ORBIT 0.001f
#define DRIFT_SHADOW_PERIOD 30
#define DRIFT_LAYER_NODE "@O_057:Layer"
#define DRIFT_BLOCKED_BY_WALL 1
#define DRIFT_BLOCKED_BY_MAP 2

// The spin rates of an orbiting drifter (or the step of a line drifter) are kept in the position vector of its
// LastPosition record; the spin angles in its step vector.
#define DRIFT_SPIN DAT_10802bf8[index].position
#define DRIFT_ANGLES DAT_10802bf8[index].step

// 0x10024f80 (Char_Drifter). One frame of the drifting character `index`. The first call (driftStarted 0) picks the
// motion from its position: a state 0 line step per axis (-20 to 16) or a state 1 circle (spin rates from the position
// digits, spin angles grown by them); the layer node and the score digit are set up. Every frame the character moves on
// (line: by the step, bouncing back when blocked; circle: round its centre), turns to face the way it goes and has its
// layer node turned by an angle that grows with its speed. Returns 1 when the character is gone, else 0.
int Char_Drifter(int index)
{
    DAT_10daaae8[index].m_0141 = 1;
    DAT_10db6ee8[index].locked = 1;
    DAT_10db6ee8[index].m_0267 = 1;
    DAT_1021f6c0[index] = 1;
    if (DAT_10daaae8[index].stats[STAT_HEALTH] > 0)
        DAT_10db6ee8[index].mode = 0;
    DAT_10db6ee8[index].m_238c = 0;
    DAT_10daaae8[index].stats[STAT_BUSY] = 1;
    if (DAT_10210d30[index] != 0 && DAT_1030b8f8[index] != 0)
        return 1;
    if (DAT_10210b30[index] != 0)
        DAT_10210b30[index] = 0;
    DAT_10218fc0[index] = 0;
    DAT_10db6ee8[index].speedScale = (float)DAT_10daaae8[index].stats[9] * DRIFT_SCALE_PER_LEVEL + 1.0f;
    Vec4 player;
    Vec4 self;
    Vec4 delta;
    Vec4 hit;
    Vec4 probeOffset;
    player.Copy(DAT_10db6ee8[0].matrix.GetTranslation());
    self.Copy(DAT_10db6ee8[index].matrix.GetTranslation());
    (self - player).Length2D();
    (self - player).Length();
    probeOffset.Fill(0.0f);
    probeOffset.z = probeOffset.z + DRIFT_PROBE_LIFT;
    DAT_11b002c0 = DAT_10db6ee8[index].collisionFilter;
    FUN_10024750(index, &hit, DRIFT_PROBE_DEPTH, 0, &probeOffset, 0, 0);
    DAT_11b002c0 = 0;
    if (DAT_1020b430[index].driftStarted == 0)
    {
        if (DAT_10daaae8[index].stats[4] % 2 == 0)
        {
            DAT_10802bf8[index].position.x =
                ((float)abs((int)DAT_10db6ee8[index].matrix.m[12] % DRIFT_DIGIT_RANGE) - DRIFT_DIGIT_CENTRE) * DRIFT_LINE_STEP;
            DAT_10802bf8[index].position.y =
                ((float)abs((int)DAT_10db6ee8[index].matrix.m[13] % DRIFT_DIGIT_RANGE) - DRIFT_DIGIT_CENTRE) * DRIFT_LINE_STEP;
            DAT_10802bf8[index].position.z =
                ((float)abs((int)(DAT_10db6ee8[index].matrix.m[14] - DRIFT_DIGIT_CENTRE) % DRIFT_DIGIT_RANGE) -
                 DRIFT_DIGIT_CENTRE) * DRIFT_LINE_STEP;
            DAT_1020b430[index].state = DRIFT_STATE_LINE;
        }
        else
        {
            DAT_10802bf8[index].position.x =
                (float)abs((int)DAT_10db6ee8[index].matrix.m[12] % DRIFT_DIGIT_RANGE) - DRIFT_DIGIT_CENTRE;
            DAT_10802bf8[index].position.y =
                (float)abs((int)DAT_10db6ee8[index].matrix.m[13] % DRIFT_DIGIT_RANGE) - DRIFT_DIGIT_CENTRE;
            float spinZ = (float)((int)(DAT_10db6ee8[index].matrix.m[14] - DRIFT_DIGIT_CENTRE) % DRIFT_DIGIT_RANGE) -
                          DRIFT_DIGIT_CENTRE;
            DAT_10802bf8[index].position.z = spinZ;
            if (DAT_10daaae8[index].routeRecord == DRIFT_ORBIT_ROUTE_RECORD)
            {
                if (DAT_10db6ee8[index].matrix.m[13] > DRIFT_BIG_LIMIT)
                    DAT_10db6ee8[index].matrix.m[13] = DRIFT_BIG_LIMIT - DAT_10db6ee8[index].matrix.m[13];
                float whole = (float)(int)(DAT_10db6ee8[index].matrix.m[13] / DRIFT_BIG_UNIT);
                DAT_10255160[index][0] = whole;
                DAT_10db6ee8[index].matrix.m[13] = DAT_10db6ee8[index].matrix.m[13] - whole * DRIFT_BIG_UNIT;
            }
            else
            {
                if (DAT_10db6ee8[index].matrix.m[13] > DRIFT_SMALL_LIMIT)
                    DAT_10db6ee8[index].matrix.m[13] = DRIFT_SMALL_LIMIT - DAT_10db6ee8[index].matrix.m[13];
                float whole = (float)(int)(DAT_10db6ee8[index].matrix.m[13] / DRIFT_SMALL_UNIT);
                DAT_10255160[index][0] = whole;
                DAT_10db6ee8[index].matrix.m[13] = DAT_10db6ee8[index].matrix.m[13] - whole * DRIFT_SMALL_UNIT;
            }
            self.y = DAT_10db6ee8[index].matrix.m[13];
            DAT_1020b430[index].orbitCentreY = (int)DAT_10db6ee8[index].matrix.m[13];
            DAT_1020b430[index].state = DRIFT_STATE_ORBIT;
            if (DAT_10db6ee8[index].matrix.m[12] > DRIFT_WRAP_FIRST)
                DAT_10db6ee8[index].matrix.m[12] = (DAT_10db6ee8[index].matrix.m[12] - DRIFT_WRAP_HALF) * -1.0f;
            if (DAT_10db6ee8[index].matrix.m[12] > DRIFT_WRAP_HALF || DAT_10db6ee8[index].matrix.m[12] < -DRIFT_WRAP_HALF)
                DAT_10db6ee8[index].matrix.m[12] = DRIFT_WRAP_HALF - DAT_10db6ee8[index].matrix.m[12];
            if (DRIFT_SPIN.x != 0.0f)
                DRIFT_ANGLES.x = (float)(int)(DAT_10db6ee8[index].matrix.m[12] / DRIFT_SMALL_UNIT) * DRIFT_ANGLE_PER_UNIT +
                                 DRIFT_ANGLES.x;
            if (DRIFT_SPIN.y != 0.0f)
                DRIFT_ANGLES.y = (float)(int)(DAT_10db6ee8[index].matrix.m[12] / DRIFT_SMALL_UNIT) * DRIFT_ANGLE_PER_UNIT +
                                 DRIFT_ANGLES.y;
            if (spinZ != 0.0f)
                DRIFT_ANGLES.z = (float)(int)(DAT_10db6ee8[index].matrix.m[12] / DRIFT_SMALL_UNIT) * DRIFT_ANGLE_PER_UNIT +
                                 DRIFT_ANGLES.z;
            float remainder = DAT_10db6ee8[index].matrix.m[12] -
                              (float)(int)(DAT_10db6ee8[index].matrix.m[12] / DRIFT_SMALL_UNIT) * DRIFT_SMALL_UNIT;
            DAT_10db6ee8[index].matrix.m[12] = remainder;
            DAT_1020b430[index].orbitCentreX = (int)remainder;
            DAT_1020b430[index].orbitCentreZ = (int)DAT_10db6ee8[index].matrix.m[14];
        }
        DAT_1020b430[index].driftStarted = 1;
        DAT_1021bdf8[index][0] = DAT_10db6ee8[index].root->Find(DRIFT_LAYER_NODE, 0);
        DAT_10868838[index][0] = DAT_1021bdf8[index][0]->matrix;
        int value = DAT_10daaae8[index].m_00d8;
        int digit = value % 10;
        if (digit != 0)
            DAT_10255f10[index] = digit;
        DAT_10daaae8[index].m_00d8 = value / 10 * 10;
    }
    delta.Fill(0.0f);
    if ((unsigned short)DAT_104c0380 % DRIFT_SHADOW_PERIOD == index % DRIFT_SHADOW_PERIOD)
    {
        DAT_10db6ee8[index].m_0012 = 2;
        FUN_1001c090(index, 1);
    }
    if (DAT_1020b430[index].state == DRIFT_STATE_LINE)
    {
        if (DAT_1020b430[index].driftSignX != 0)
            delta.x = DRIFT_SPIN.x;
        else
            delta.x = -DRIFT_SPIN.x;
        if (DAT_1020b430[index].driftSignY != 0)
            delta.y = DRIFT_SPIN.y;
        else
            delta.y = -DRIFT_SPIN.y;
        if (DAT_1020b430[index].driftSignZ != 0)
            delta.z = DRIFT_SPIN.z;
        else
            delta.z = -DRIFT_SPIN.z;
        DAT_10db6ee8[index].matrix.m[12] = delta.x * DAT_10db6ee8[index].speedScale + DAT_10db6ee8[index].matrix.m[12];
        DAT_10db6ee8[index].matrix.m[13] = delta.y * DAT_10db6ee8[index].speedScale + DAT_10db6ee8[index].matrix.m[13];
        DAT_10db6ee8[index].matrix.m[14] = delta.z * DAT_10db6ee8[index].speedScale + DAT_10db6ee8[index].matrix.m[14];
        int blocked = DAT_10db6ee8[index].IsClearAny(0) == 0 || DAT_10db6ee8[index].IsClearOfMapLimited(0) == 0;
        if (DAT_104bfd08 != 0)
        {
            DAT_10db6ee8[index].matrix.m[12] = self.x;
            DAT_10db6ee8[index].matrix.m[13] = self.y;
            DAT_10db6ee8[index].matrix.m[14] = self.z;
        }
        if (blocked)
        {
            delta.Copy(delta * -1.0f);
            DAT_1020b430[index].driftSignX = DAT_1020b430[index].driftSignX == 0;
            DAT_1020b430[index].driftSignY = DAT_1020b430[index].driftSignY == 0;
            DAT_1020b430[index].driftSignZ = DAT_1020b430[index].driftSignZ == 0;
        }
        DAT_10db6ee8[index].move.x = DAT_10db6ee8[index].move.x * DAT_10db6ee8[index].speedScale;
        DAT_10db6ee8[index].move.y = DAT_10db6ee8[index].move.y * DAT_10db6ee8[index].speedScale;
        DAT_10db6ee8[index].move.z = DAT_10db6ee8[index].move.z * DAT_10db6ee8[index].speedScale;
        DAT_10db6ee8[index].targetHeading = DAT_10db6ee8[index].heading = Atan2(delta.x, delta.y) + DRIFT_PI;
    }
    else if (DAT_1020b430[index].state == DRIFT_STATE_ORBIT)
    {
        if (DRIFT_SPIN.x != 0.0f)
        {
            if (DAT_1020b430[index].orbitForward != 0)
                DRIFT_ANGLES.x = DAT_10255160[index][0] * DAT_10db6ee8[index].speedScale * DRIFT_SPIN_STEP + DRIFT_ANGLES.x;
            else
                DRIFT_ANGLES.x = DRIFT_ANGLES.x - DAT_10255160[index][0] * DAT_10db6ee8[index].speedScale * DRIFT_SPIN_STEP;
        }
        if (DRIFT_SPIN.y != 0.0f)
        {
            if (DAT_1020b430[index].orbitForward != 0)
                DRIFT_ANGLES.y = DAT_10255160[index][0] * DAT_10db6ee8[index].speedScale * DRIFT_SPIN_STEP + DRIFT_ANGLES.y;
            else
                DRIFT_ANGLES.y = DRIFT_ANGLES.y - DAT_10255160[index][0] * DAT_10db6ee8[index].speedScale * DRIFT_SPIN_STEP;
        }
        if (DRIFT_SPIN.z != 0.0f)
        {
            if (DAT_1020b430[index].orbitForward != 0)
                DRIFT_ANGLES.z = DAT_10255160[index][0] * DAT_10db6ee8[index].speedScale * DRIFT_SPIN_STEP + DRIFT_ANGLES.z;
            else
                DRIFT_ANGLES.z = DRIFT_ANGLES.z - DAT_10255160[index][0] * DAT_10db6ee8[index].speedScale * DRIFT_SPIN_STEP;
        }
        if (DRIFT_ANGLES.x < 0.0f)
            DRIFT_ANGLES.x = DRIFT_ANGLES.x + DRIFT_TWO_PI;
        else if (DRIFT_ANGLES.x > DRIFT_TWO_PI)
            DRIFT_ANGLES.x = DRIFT_ANGLES.x - DRIFT_TWO_PI;
        if (DRIFT_ANGLES.y < 0.0f)
            DRIFT_ANGLES.y = DRIFT_ANGLES.y + DRIFT_TWO_PI;
        else if (DRIFT_ANGLES.y > DRIFT_TWO_PI)
            DRIFT_ANGLES.y = DRIFT_ANGLES.y - DRIFT_TWO_PI;
        if (DRIFT_ANGLES.z < 0.0f)
            DRIFT_ANGLES.z = DRIFT_ANGLES.z + DRIFT_TWO_PI;
        else if (DRIFT_ANGLES.z > DRIFT_TWO_PI)
            DRIFT_ANGLES.z = DRIFT_ANGLES.z - DRIFT_TWO_PI;
        delta.x = DRIFT_SPIN.z + DRIFT_SPIN.y;
        delta.y = DRIFT_SPIN.z + DRIFT_SPIN.x;
        delta.z = DRIFT_SPIN.y + DRIFT_SPIN.x;
        delta.RotateX(DRIFT_ANGLES.x);
        delta.RotateY(DRIFT_ANGLES.y);
        delta.RotateZ(DRIFT_ANGLES.z);
        delta.Copy(delta * DRIFT_ORBIT_RADIUS);
        DAT_10db6ee8[index].matrix.m[12] = (float)DAT_1020b430[index].orbitCentreX + delta.x;
        DAT_10db6ee8[index].matrix.m[13] = (float)DAT_1020b430[index].orbitCentreY + delta.y;
        DAT_10db6ee8[index].matrix.m[14] = (float)DAT_1020b430[index].orbitCentreZ + delta.z;
        int blockedBy;
        if (DAT_10db6ee8[index].IsClearAny(0) == 0)
            blockedBy = DRIFT_BLOCKED_BY_WALL;
        if (DAT_10db6ee8[index].IsClearOfMapLimited(0) == 0)
            blockedBy = DRIFT_BLOCKED_BY_MAP;
        if (DAT_104bfd08 != 0)
        {
            DAT_10db6ee8[index].matrix.m[12] = self.x;
            DAT_10db6ee8[index].matrix.m[13] = self.y;
            DAT_10db6ee8[index].matrix.m[14] = self.z;
        }
        if (blockedBy == DRIFT_BLOCKED_BY_WALL)
            DAT_1020b430[index].orbitForward = DAT_1020b430[index].orbitForward == 0;
        else if (blockedBy == DRIFT_BLOCKED_BY_MAP)
            DAT_1020b430[index].orbitForward = DAT_1020b430[index].orbitForward == 0;
        DAT_10db6ee8[index].targetHeading = DAT_10db6ee8[index].heading =
            Atan2(DAT_10db6ee8[index].matrix.m[12] - self.x, DAT_10db6ee8[index].matrix.m[13] - self.y) + DRIFT_PI;
    }
    int opacity = DAT_10db6ee8[index].shadowOpacity;
    DAT_10db6ee8[index].SetPosition(DAT_10db6ee8[index].matrix.m[12], DAT_10db6ee8[index].matrix.m[13],
                                    DAT_10db6ee8[index].matrix.m[14]);
    DAT_10db6ee8[index].shadowOpacity = opacity;
    float speed = delta.Length();
    if (DAT_1020b430[index].state == DRIFT_STATE_LINE)
        DAT_10255160[index][1] += speed * DRIFT_PHASE_LINE;
    else
        DAT_10255160[index][1] += speed * DRIFT_PHASE_ORBIT;
    // Wrap the swing phase (written through the table, not a pointer local: a pointer makes cl store each pass)
    while (DAT_10255160[index][1] > DRIFT_PHASE_LIMIT)
        DAT_10255160[index][1] -= DRIFT_PHASE_LIMIT;
    DAT_1021bdf8[index][0]->matrix = RotationX(-DAT_10255160[index][1]) * TranslationMatrix(DAT_10868838[index][0].m[12],
                                                                          DAT_10868838[index][0].m[13],
                                                                          DAT_10868838[index][0].m[14]);
    return 0;
}
