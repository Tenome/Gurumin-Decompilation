// Barrage enemies: behaviour numbers of Enemy_UpdateBehaviour 0x100bcb30 (char_dispatch.cpp) that walk with the
// common enemy behaviour (Enemy_ChaseLungeAndSpit) and now and then stop to call down a barrage of effects on the player.

#include <math.h>
#include "vec4.h"

#include "matrix4.h"

#include "anim_model.h"

#include "node.h"

#include "effect_manager.h"

#include "char_slot.h"

#include "char_state.h"

#include "char_tables.h"

#include "map_vars.h"

#include "char_vars.h"

#include "barrage_enemy.h"
#include "enemy_chase_lunge.h"

#include "strafe_enemy.h"

extern "C" int FUN_1013f880(void);                                          // the game's random number
extern "C" int FUN_10065b10(int index, int sound, int always, int optional);  // Character_PlaySound
extern "C" int FUN_1001d8b0(int index, int effect, Vec4 position, int parent, int unused, const Matrix4 *owner,
                            int value, int frame, const Matrix4 *matrix, const void *options);  // Char_StartEffect
Matrix4 RotationZ(float angle);                                             // 0x101b1b40
Matrix4 RotationX(float angle);                                             // 0x101b1ad0
extern "C" int FUN_1001bfb0(int index, float maxTurn);                      // Char_LimitTurn (char_turn.cpp)

#define FULL_TURN 6.283f               // two pi as the game writes it

// 0x100a77f0 (Enemy_CallBarrage, behaviour 15 of Enemy_UpdateBehaviour). Walks with the common enemy behaviour;
// with a 2.5 % chance per frame (when motion layer 4 is not playing and no blend runs) it starts a barrage: it turns
// to the player (motion 0 while turning, 2 once facing within a degree), starts effect 0x3702 four times with sound
// 0x1c4, fires effect 0x429 from a random point above itself at the player every 10 frames from frame 60 to 110, and
// goes back to walking on frame 150. Returns 0.
int Enemy_CallBarrage(int index)
{
    if (DAT_1020b430[index].state == BARRAGE_RUNNING)
    {
        Vec4 player;
        Vec4 self;
        player.Copy(DAT_10db6ee8[0].matrix.GetTranslation());
        Matrix4 *matrix = &DAT_10db6ee8[index].matrix;
        self.Copy(matrix->GetTranslation());
        (player - self).Length();  // computed and dropped
        // turn towards the player; walk while the heading is more than a degree off
        float dy = player.y - self.y;
        float dx = player.x - self.x;
        float turn = (float)atan2((double)dx, (double)dy) + PI_APPROX;
        DAT_10db6ee8[index].targetHeading = turn;
        turn = DAT_10db6ee8[index].heading - turn;  // now the angle still to turn
        while (turn > PI_APPROX)
            turn -= FULL_TURN;
        while (turn < -PI_APPROX)
            turn += FULL_TURN;
        int motion = turn * turn < FACING_TOLERANCE_SQUARED ? 2 : 0;  // idle once facing the player
        int frame = ++DAT_1020b430[index].wait;
        if (frame == 1)
        {
            // the charge: four copies of the effect following the enemy, three frames apart
            Vec4 offset;
            EffectOptions options;
            options.followOwner = 1;
            FUN_1001d8b0(index, BARRAGE_CHARGE_EFFECT, offset, -1, -1, matrix, 0, 0, 0, &options);
            FUN_1001d8b0(index, BARRAGE_CHARGE_EFFECT, offset, -1, -1, matrix, 0, 3, 0, &options);
            FUN_1001d8b0(index, BARRAGE_CHARGE_EFFECT, offset, -1, -1, matrix, 0, 6, 0, &options);
            FUN_1001d8b0(index, BARRAGE_CHARGE_EFFECT, offset, -1, -1, matrix, 0, 9, 0, &options);
            FUN_10065b10(index, BARRAGE_CHARGE_SOUND, 0, 0);
        }
        else if (frame > BARRAGE_FIRST_SHOT - 1 && frame < BARRAGE_LAST_SHOT + 1 && frame % BARRAGE_SHOT_PERIOD == 0)
        {
            // one shot from a random point above the enemy, aimed at the player
            Vec4 position;
            Vec4 aim;
            EffectOptions options;
            Matrix4 shot;
            shot.SetIdentity();
            position.Copy(self);
            position.z = (float)(FUN_1013f880() % BARRAGE_SHOT_HEIGHT) + position.z + 50.0f;
            position.x = (float)(FUN_1013f880() % BARRAGE_SHOT_SPREAD) - 150.0f + position.x;
            position.y = (float)(FUN_1013f880() % BARRAGE_SHOT_SPREAD) - 150.0f + position.y;
            aim.Copy(player);
            aim.z += BARRAGE_AIM_HEIGHT;
            shot = RotationZ(atan2f(aim.x - position.x, aim.y - position.y) + PI_APPROX);
            shot.SetTranslation(position);
            options.m_0004 = DAT_10daaae8[index].stats[STAT_EFFECT_PARAM];
            options.m_0008 = BARRAGE_SHOT_SCALE;
            options.m_0000 = 0;
            options.followOwner = 1;
            position.Fill(0.0f);
            FUN_1001d8b0(index, BARRAGE_SHOT_EFFECT, position, -1, -1, 0, 0, 0, &shot, &options);
        }
        else if (frame == BARRAGE_FRAMES)
        {
            DAT_1020b430[index].state = 0;
            DAT_1020b430[index].wait = 0;
        }
        if (motion > -1)
            DAT_10db6ee8[index].PlayMotion(motion, 0, 0, 1.0f);
    }
    else
    {
        Enemy_ChaseLungeAndSpit(index, 0);
        if (DAT_10db6ee8[index].motion->layer != BARRAGE_BLOCKING_LAYER &&
            DAT_10db6ee8[index].motion->blending == 0 && FUN_1013f880() % 10000 < BARRAGE_CHANCE)
        {
            DAT_1020b430[index].state = BARRAGE_RUNNING;
            DAT_1020b430[index].wait = 0;
        }
    }
    return 0;
}

#define min(a, b) ((a) < (b) ? (a) : (b))

// 0x100a7db0 (Enemy_SpinFire, behaviour 14 of Enemy_UpdateBehaviour). Walks with the common enemy behaviour; with a
// 2.5 % chance per frame (as Enemy_CallBarrage) it spins in place for 150 frames, speeding up to 30 degrees a frame
// over the first 30 frames and slowing down over the last 30. Effect 0x3701 starts from its "@firenull" node (else
// its head) when the spin starts (sound 0x1c3) and every 40 frames; effect 0x36ff fires 125 units above it every 10
// frames from frame 30 to 120 (sound 0x1c2). A model with neither node stops the spin at once. Returns 0.
int Enemy_SpinFire(int index)
{
    if (DAT_1020b430[index].state == BARRAGE_RUNNING)
    {
        Vec4 player;
        Vec4 self;
        player.Copy(DAT_10db6ee8[0].matrix.GetTranslation());
        self.Copy(DAT_10db6ee8[index].matrix.GetTranslation());
        (player - self).Length();    // computed and dropped
        (player - self).Length2D();  // computed and dropped
        int starting = 0;
        if (DAT_1020b430[index].wait == 0)
        {
            starting = 1;
            Matrix4 effectMatrix;
            Vec4 offset;
            EffectOptions options;
            options.followOwner = 1;
            options.m_0004 = DAT_10daaae8[index].stats[STAT_EFFECT_PARAM];
            options.m_0008 = SPIN_EFFECT_SCALE;
            offset.Fill(0.0f);
            effectMatrix.SetIdentity();
            options.owner = index + EFFECT_OWNER_BASE;
            Node *fire = DAT_10db6ee8[index].root->Find(FIRE_NODE_NAME, 0);
            // the fire node, else the head node; a model with neither stops the spin
            if (fire != 0)
                FUN_1001d8b0(index, SPIN_START_EFFECT, offset, -1, -1, &fire->worldMatrix, 0, 0, &effectMatrix, &options);
            else if (DAT_10db6ee8[index].headNode != 0)
                FUN_1001d8b0(index, SPIN_START_EFFECT, offset, -1, -1, &DAT_10db6ee8[index].headNode->worldMatrix, 0, 0,
                             &effectMatrix, &options);
            else
            {
                DAT_1020b430[index].wait = 0;
                DAT_1020b430[index].state = 0;
                return 0;
            }
            FUN_10065b10(index, SPIN_START_SOUND, 0, 0);
        }
        int frame = ++DAT_1020b430[index].wait;
        // the speed ramps up over the first SPIN_RAMP_FRAMES frames and down over the last ones
        int ramp = min(frame > BARRAGE_FRAMES - frame ? BARRAGE_FRAMES - frame : frame, SPIN_RAMP_FRAMES);
        float heading = (float)ramp / SPIN_RAMP_FRAMES * SPIN_MAX_TURN + DAT_10db6ee8[index].targetHeading;
        DAT_10db6ee8[index].targetHeading = heading;
        DAT_10db6ee8[index].heading = heading;
        if (frame > SPIN_FIRST_SHOT - 1 && frame < SPIN_LAST_SHOT + 1 && frame % SPIN_SHOT_PERIOD == 0)
        {
            Matrix4 effectMatrix;
            Vec4 offset;
            EffectOptions options;
            options.followOwner = 1;
            options.m_0004 = DAT_10daaae8[index].stats[STAT_EFFECT_PARAM];
            options.m_0008 = SPIN_EFFECT_SCALE;
            offset.Fill(0.0f);
            effectMatrix.SetIdentity();
            Node *fire = DAT_10db6ee8[index].root->Find(FIRE_NODE_NAME, 0);
            options.owner = index + EFFECT_OWNER_BASE;
            if (fire != 0)
            {
                effectMatrix.SetTranslation(fire->worldMatrix.GetTranslation());
                effectMatrix.m[MATRIX_TRANSLATION_Z] = self.z + SPIN_SHOT_HEIGHT;
                FUN_1001d8b0(index, SPIN_SHOT_EFFECT, offset, -1, -1, 0, 0, 0, &effectMatrix, &options);
            }
            else if (DAT_10db6ee8[index].headNode != 0)
            {
                effectMatrix.SetTranslation(DAT_10db6ee8[index].headNode->worldMatrix.GetTranslation());
                effectMatrix.m[MATRIX_TRANSLATION_Z] = self.z + SPIN_SHOT_HEIGHT;
                FUN_1001d8b0(index, SPIN_SHOT_EFFECT, offset, -1, -1, 0, 0, 0, &effectMatrix, &options);
            }
            FUN_10065b10(index, SPIN_SHOT_SOUND, 0, 0);
        }
        if (DAT_1020b430[index].wait % SPIN_FIRE_PERIOD == 0)
        {
            Matrix4 effectMatrix;
            Vec4 offset;
            EffectOptions options;
            options.followOwner = 1;
            options.m_0004 = DAT_10daaae8[index].stats[STAT_EFFECT_PARAM];
            options.m_0008 = SPIN_EFFECT_SCALE;
            offset.Fill(0.0f);
            effectMatrix.SetIdentity();
            options.owner = index + EFFECT_OWNER_BASE;
            Node *fire = DAT_10db6ee8[index].root->Find(FIRE_NODE_NAME, 0);
            if (fire != 0)
                FUN_1001d8b0(index, SPIN_START_EFFECT, offset, -1, -1, &fire->worldMatrix, 0, 0, &effectMatrix, &options);
            else if (DAT_10db6ee8[index].headNode != 0)
                FUN_1001d8b0(index, SPIN_START_EFFECT, offset, -1, -1, &DAT_10db6ee8[index].headNode->worldMatrix, 0, 0,
                             &effectMatrix, &options);
        }
        if (DAT_1020b430[index].wait == BARRAGE_FRAMES)
        {
            DAT_1020b430[index].state = 0;
            DAT_1020b430[index].wait = 0;
        }
        DAT_10db6ee8[index].PlayMotion(RUN_MOTION, starting, 5, 1.0f);
    }
    else
    {
        Enemy_ChaseLungeAndSpit(index, 0);
        if (DAT_10db6ee8[index].motion->layer != BARRAGE_BLOCKING_LAYER &&
            DAT_10db6ee8[index].motion->blending == 0 && FUN_1013f880() % 10000 < BARRAGE_CHANCE)
        {
            DAT_1020b430[index].state = BARRAGE_RUNNING;
            DAT_1020b430[index].wait = 0;
        }
    }
    return 0;
}

// 0x100a7150 (Enemy_StarVolley, behaviour 13 of Enemy_UpdateBehaviour). Turns to the player at most 30 degrees a
// frame (motion 2 once facing within a degree, else motion 0). While the player is nearer than 1000 units it starts
// runs of 1 to 10 volleys; each volley fires effect 600 at the player from points of a five-point ring around the
// enemy (radius 50, 75 units up): point 1 straight ahead, then points 2 and 5 at 72 degrees, then points 3 and 4 at
// 144 degrees, five frames apart (sound 0x1c5 with each step). The ring points (DAT_10868838[index][0..4]) are
// placed again every frame. The first frame only sets `state` to 1. Returns 0.
int Enemy_StarVolley(int index)
{
    Vec4 player;
    Vec4 self;
    player.Copy(DAT_10db6ee8[0].matrix.GetTranslation());
    self.Copy(DAT_10db6ee8[index].matrix.GetTranslation());
    float distance = (player - self).Length();
    (player - self).Length2D();  // computed and dropped
    if (DAT_1020b430[index].state == 0)
        DAT_1020b430[index].state = 1;
    else if (DAT_1020b430[index].state == 1)
    {
        float dy = player.y - self.y;
        float dx = player.x - self.x;
        DAT_10db6ee8[index].targetHeading = (float)atan2((double)dx, (double)dy) + PI_APPROX;
        FUN_1001bfb0(index, STAR_TURN_LIMIT);
        float turn = DAT_10db6ee8[index].targetHeading - DAT_10db6ee8[index].heading;
        int motion = (float)fabs(turn) > ONE_DEGREE ? 0 : 2;  // idle once facing the player
        if (DAT_1020b430[index].action > 0)
            DAT_1020b430[index].action--;
        if (DAT_1020b430[index].action == 0 && distance < STAR_START_DISTANCE)
        {
            // a new run of volleys
            int volleys = FUN_1013f880() % STAR_MAX_VOLLEYS;
            DAT_1020b430[index].range = STAR_VOLLEY_START;
            DAT_1020b430[index].talk = volleys + 1;
            DAT_1020b430[index].action = (DAT_1020b430[index].talk + 8) * STAR_VOLLEY_FRAMES;
        }
        if (DAT_1020b430[index].talk != 0 && DAT_1020b430[index].range % STAR_VOLLEY_STEP == 0)
        {
            // which ring points fire this step, and their angles from straight ahead
            int step = DAT_1020b430[index].range;
            int points[2];
            float angles[2];
            points[0] = points[1] = 0;
            if (step == STAR_VOLLEY_START)
            {
                points[0] = 1;
                points[1] = 0;
                angles[0] = 0.0f;
                angles[1] = 0.0f;
            }
            else if (step == STAR_VOLLEY_STEP)
            {
                points[0] = 2;
                points[1] = 5;
                angles[0] = STAR_SIDE_ANGLE;
                angles[1] = -STAR_SIDE_ANGLE;
            }
            else if (step == 0)
            {
                points[0] = 3;
                points[1] = 4;
                angles[0] = STAR_WIDE_ANGLE;
                angles[1] = -STAR_WIDE_ANGLE;
            }
            if (DAT_1020b430[index].range == 0)
            {
                // the volley is done; wait for the next one
                DAT_1020b430[index].talk--;
                DAT_1020b430[index].range = DAT_10c635f8 == 2 ? STAR_VOLLEY_GAP_MODE2 : STAR_VOLLEY_GAP;
            }
            Matrix4 shot;
            EffectOptions options;
            Vec4 aim;
            int fired = 0;
            for (int i = 0; i < 2; i++)
            {
                if (points[i] != 0)
                {
                    fired = 1;
                    aim.Copy(player - self);
                    if (aim.Length() < MIN_AIM_DISTANCE)
                        shot = RotationZ(angles[i] + DAT_10db6ee8[index].heading);
                    else if (aim.z > 0.0f)
                        shot = RotationX(acosf(aim.Length2D() / aim.Length())) *
                               RotationZ(angles[i] + DAT_10db6ee8[index].targetHeading);
                    else
                        shot = RotationX(-acosf(aim.Length2D() / aim.Length())) *
                               RotationZ(angles[i] + DAT_10db6ee8[index].targetHeading);
                    options.m_0004 = DAT_10daaae8[index].stats[STAT_EFFECT_PARAM];
                    options.m_0008 = STAR_SHOT_SCALE;
                    aim.Fill(0.0f);
                    // ring point n is DAT_10868838[index][n - 1]
                    FUN_1001d8b0(index, STAR_SHOT_EFFECT, aim, -1, -1, &DAT_10868838[index][points[i] - 1], 0, 0,
                                 &shot, &options);
                }
            }
            if (fired)
                FUN_10065b10(index, STAR_SHOT_SOUND, 0, 0);
        }
        if (DAT_1020b430[index].range != 0)
            DAT_1020b430[index].range--;
        // place the ring points around the enemy
        for (int point = 0; point < STAR_POINT_COUNT; point++)
        {
            DAT_10868838[index][point].SetIdentity();
            Vec4 offset;
            offset.z = STAR_POINT_HEIGHT;
            offset.y = STAR_POINT_DISTANCE;
            offset.RotateZ((float)point * STAR_POINT_STEP - DAT_10db6ee8[index].heading);
            offset.Copy(offset + self);
            DAT_10868838[index][point].SetTranslation(offset);
        }
        if (motion > -1)
            DAT_10db6ee8[index].PlayMotion(motion, 0, 5, 1.0f);
    }
    return 0;
}


// 0x100a6a50 (Enemy_AimedShots, behaviour 12 of Enemy_UpdateBehaviour). Turns to the player like Enemy_StarVolley;
// while the player is nearer than 1000 units it starts runs of 1 to 10 shots 15 frames apart: effect 600 from a point
// 50 units behind and 75 units above the enemy (DAT_10868838[index][0], placed every frame), aimed at the player, plus
// two more 30 degrees to the left and right when DAT_10c635f8 is 2 (sound 0x1c5). The first frame only sets `state`
// to 1. Returns 0.
int Enemy_AimedShots(int index)
{
    Vec4 player;
    Vec4 self;
    player.Copy(DAT_10db6ee8[0].matrix.GetTranslation());
    self.Copy(DAT_10db6ee8[index].matrix.GetTranslation());
    float distance = (player - self).Length();
    (player - self).Length2D();  // computed and dropped
    if (DAT_1020b430[index].state == 0)
        DAT_1020b430[index].state = 1;
    else if (DAT_1020b430[index].state == 1)
    {
        float dy = player.y - self.y;
        float dx = player.x - self.x;
        DAT_10db6ee8[index].targetHeading = (float)atan2((double)dx, (double)dy) + PI_APPROX;
        FUN_1001bfb0(index, STAR_TURN_LIMIT);
        float turn = DAT_10db6ee8[index].targetHeading - DAT_10db6ee8[index].heading;
        int motion = (float)fabs(turn) > ONE_DEGREE ? 0 : 2;  // idle once facing the player
        if (DAT_1020b430[index].action > 0)
            DAT_1020b430[index].action--;
        if (DAT_1020b430[index].action == 0 && distance < STAR_START_DISTANCE)
        {
            // a new run of shots
            int shots = FUN_1013f880() % STAR_MAX_VOLLEYS;
            DAT_1020b430[index].range = 1;
            DAT_1020b430[index].talk = shots + 1;
            DAT_1020b430[index].action = (DAT_1020b430[index].talk + AIMED_RUN_EXTRA) * STAR_VOLLEY_FRAMES;
        }
        if (DAT_1020b430[index].talk != 0 && DAT_1020b430[index].range == 0)
        {
            DAT_1020b430[index].talk--;
            DAT_1020b430[index].range = AIMED_SHOT_GAP;
            Matrix4 shot;
            EffectOptions options;
            Vec4 aim;
            aim.Copy(player - self);
            if (aim.Length() < MIN_AIM_DISTANCE)
                shot = RotationZ(DAT_10db6ee8[index].heading);
            else if (aim.z > 0.0f)
                shot = RotationX(acosf(aim.Length2D() / aim.Length())) * RotationZ(DAT_10db6ee8[index].targetHeading);
            else
                shot = RotationX(-acosf(aim.Length2D() / aim.Length())) * RotationZ(DAT_10db6ee8[index].targetHeading);
            options.m_0004 = DAT_10daaae8[index].stats[STAT_EFFECT_PARAM];
            options.m_0008 = STAR_SHOT_SCALE;
            FUN_1001d8b0(index, STAR_SHOT_EFFECT, aim, -1, -1, &DAT_10868838[index][0], 0, 0, &shot, &options);
            if (DAT_10c635f8 == 2)
            {
                Matrix4 side;
                side = shot * RotationZ(-AIMED_SIDE_ANGLE);
                FUN_1001d8b0(index, STAR_SHOT_EFFECT, aim, -1, -1, &DAT_10868838[index][0], 0, 0, &side, &options);
                side = shot * RotationZ(AIMED_SIDE_ANGLE);
                FUN_1001d8b0(index, STAR_SHOT_EFFECT, aim, -1, -1, &DAT_10868838[index][0], 0, 0, &side, &options);
            }
            FUN_10065b10(index, STAR_SHOT_SOUND, 0, 0);
        }
        if (DAT_1020b430[index].range != 0)
            DAT_1020b430[index].range--;
        // place the shot point behind the enemy
        DAT_10868838[index][0].SetIdentity();
        Vec4 offset;
        offset.z = STAR_POINT_HEIGHT;
        offset.y = STAR_POINT_DISTANCE;
        offset.RotateZ(-DAT_10db6ee8[index].heading);
        offset.Copy(offset + self);
        DAT_10868838[index][0].SetTranslation(offset);
        if (motion > -1)
            DAT_10db6ee8[index].PlayMotion(motion, 0, 5, 1.0f);
    }
    return 0;
}


// 0x100a8370 (Enemy_CallBursts, behaviours 16 and 17 of Enemy_UpdateBehaviour with variant 0 and 1). Walks with the
// common enemy behaviour and, with the same 2.5 % chance per frame as Enemy_CallBarrage, stops to call bursts: it
// faces the player, speeds its motion up and waits until its motion sets AnimModel::m_238c; then effect 0x3705 bursts
// on the player's spine (else head, else position) at points around the enemy's facing: two 120 units up, and with
// variant 1 four more at 45 and 87 units. It walks again 60 frames later. Returns 0.
int Enemy_CallBursts(int index, int variant)
{
    if (DAT_1020b430[index].state == BARRAGE_RUNNING)
    {
        Vec4 player;
        Vec4 self;
        player.Copy(DAT_10db6ee8[0].matrix.GetTranslation());
        self.Copy(DAT_10db6ee8[index].matrix.GetTranslation());
        if (++DAT_1020b430[index].wait == 1)
        {
            // the call starts: face the player
            DAT_10db6ee8[index].AddMotionSpeed(BURST_MOTION_SPEED);
            DAT_10db6ee8[index].m_238c = 0;
            float dy = player.y - self.y;
            float dx = player.x - self.x;
            DAT_10db6ee8[index].targetHeading = (float)atan2((double)dx, (double)dy) + PI_APPROX;
        }
        if (DAT_1020b430[index].wait < BURST_FIRED)
        {
            if (DAT_10db6ee8[index].m_238c == 1)
            {
                // the motion reached its call frame: fire the bursts
                DAT_1020b430[index].wait = BURST_FIRED;
                EffectOptions options;
                Matrix4 burst;
                Vec4 offset;
                const Matrix4 *target;
                if (DAT_10db6ee8[0].root->Find(BURST_NODE_NAME, 0) != 0)
                    target = &DAT_10db6ee8[0].root->Find(BURST_NODE_NAME, 0)->worldMatrix;
                else if (DAT_10db6ee8[0].headNode != 0)
                    target = &DAT_10db6ee8[0].headNode->worldMatrix;
                else
                    target = &DAT_10db6ee8[0].matrix;
                options.m_0004 = DAT_10daaae8[index].stats[STAT_EFFECT_PARAM];
                options.m_0008 = BURST_SCALE;
                burst.SetIdentity();
                burst = RotationZ(DAT_10db6ee8[index].heading);
                if (variant != 0)
                {
                    offset.x = -52.0f;
                    offset.z = 45.0f;
                    offset.RotateZ(-DAT_10db6ee8[index].heading);
                    burst.SetTranslation(offset + self);
                    offset.Fill(0.0f);
                    FUN_1001d8b0(index, BURST_EFFECT, offset, -1, -1, target, 0, FUN_1013f880() % BURST_FRAME_SPREAD,
                                 &burst, &options);
                    offset.x = 52.0f;
                    offset.z = 45.0f;
                    offset.RotateZ(-DAT_10db6ee8[index].heading);
                    burst.SetTranslation(offset + self);
                    offset.Fill(0.0f);
                    FUN_1001d8b0(index, BURST_EFFECT, offset, -1, -1, target, 0, FUN_1013f880() % BURST_FRAME_SPREAD,
                                 &burst, &options);
                    offset.x = -50.0f;
                    offset.z = 87.0f;
                    offset.RotateZ(-DAT_10db6ee8[index].heading);
                    burst.SetTranslation(offset + self);
                    offset.Fill(0.0f);
                    FUN_1001d8b0(index, BURST_EFFECT, offset, -1, -1, target, 0, FUN_1013f880() % BURST_FRAME_SPREAD,
                                 &burst, &options);
                    offset.x = 52.0f;
                    offset.z = 87.0f;
                    offset.RotateZ(-DAT_10db6ee8[index].heading);
                    burst.SetTranslation(offset + self);
                    offset.Fill(0.0f);
                    FUN_1001d8b0(index, BURST_EFFECT, offset, -1, -1, target, 0, FUN_1013f880() % BURST_FRAME_SPREAD,
                                 &burst, &options);
                }
                offset.x = -25.0f;
                offset.z = 120.0f;
                offset.RotateZ(-DAT_10db6ee8[index].heading);
                burst.SetTranslation(offset + self);
                offset.Fill(0.0f);
                FUN_1001d8b0(index, BURST_EFFECT, offset, -1, -1, target, 0, FUN_1013f880() % BURST_FRAME_SPREAD, &burst,
                             &options);
                offset.x = 25.0f;
                offset.z = 120.0f;
                offset.RotateZ(-DAT_10db6ee8[index].heading);
                burst.SetTranslation(offset + self);
                offset.Fill(0.0f);
                FUN_1001d8b0(index, BURST_EFFECT, offset, -1, -1, target, 0, FUN_1013f880() % BURST_FRAME_SPREAD, &burst,
                             &options);
            }
            DAT_10db6ee8[index].PlayMotion(2, 0, 5, 1.0f);
        }
        else if (DAT_1020b430[index].wait > BURST_END)
        {
            DAT_1020b430[index].state = 0;
            DAT_1020b430[index].wait = 0;
        }
    }
    else
    {
        Enemy_ChaseLungeAndSpit(index, 0);
        if (DAT_10db6ee8[index].motion->layer != BARRAGE_BLOCKING_LAYER &&
            DAT_10db6ee8[index].motion->blending == 0 && FUN_1013f880() % 10000 < BARRAGE_CHANCE)
        {
            DAT_1020b430[index].state = BARRAGE_RUNNING;
            DAT_1020b430[index].wait = 0;
        }
    }
    return 0;
}
