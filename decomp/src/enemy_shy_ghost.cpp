// The shy ghost (0x10075350, behaviour 0x11 of the per-kind update 0x100f9670). It hides (alpha 0) while the player
// looks at it and shows itself when the player looks away or comes within 300 units, then creeps up to the player,
// sidesteps, lunges and spits, and sinks away again when the player turns round to it. See shy_ghost.h for the steps.

#include <math.h>

#include "vec4.h"

#include "matrix4.h"

#include "node.h"

#include "effect_manager.h"

#include "anim_model.h"

typedef char check_charrecord[sizeof(AnimModel) == 0x249c ? 1 : -1];

#include "char_state.h"

#include "char_slot.h"
#include "char_tables.h"
#include "char_vars.h"
#include "map_vars.h"
#include "world.h"

#include "shy_ghost.h"

extern "C" int FUN_1013f880(void);                                            // the game's random number
extern "C" int FUN_10065b10(int index, int sound, int always, int optional);  // Character_PlaySound
extern "C" int FUN_100241e0(int index, float x, float y, float z);            // Character_SetMove
extern "C" int FUN_1016b230(const char *text);                                // sets the title caption
extern "C" int FUN_10055ee0(void);                                            // Player_KnockBack
extern "C" int FUN_1001d8b0(int index, int effect, Vec4 position, int parent, int unused, const Matrix4 *owner,
                            int value, int frame, const Matrix4 *matrix, const void *options);  // Char_StartEffect

// The bearing in whole float steps (the arguments are rounded to float before the call).
inline float Atan2(float y, float x)
{
    return (float)atan2(y, x);
}

#define GHOST_PI 3.1415f
#define GHOST_FULL_TURN 6.283f
#define GHOST_HEAD_NODE "Bip01 Head"
#define GHOST_SPINE_NODE "Bip01 Spine"
#define GHOST_PELVIS_NODE "Bip01 Pelvis"
#define GHOST_SIGHT_PERIOD 15           // every 15th frame (by slot) it tests the line to the player
#define GHOST_SIGHT_LIFT 25.0f          // the line runs this far above its position and the player's
#define GHOST_SIGHT_MASK 4
#define GHOST_PELVIS_DROP 10.0f         // the effects' attachment matrix is its pelvis lowered by this
#define GHOST_STRIKE_FRAMES 0x2d        // a hit shows it this long, then it sinks away
#define GHOST_NEAR_RANGE 300.0f         // closer than this it shows itself at once
#define GHOST_LOOK_RANGE 500.0f         // closer than this it shows itself when the player looks away
#define GHOST_LOOK_AWAY 2.3f            // the player's heading differs from the bearing to it by more than this
#define GHOST_LOOK_AWAY_MOVE 1.91986f   // 110 degrees: it comes out of hiding to stalk
#define GHOST_STALK_RANGE 350.0f        // closer than this it faces the player and picks lunge or spit
#define GHOST_FACING_LIMIT 0.8f         // facing the player this well it stands still, else it turns (motion 0)
#define GHOST_ROLL_RANGE 10000
#define GHOST_LUNGE_ROLL 0x2ee
#define GHOST_SPIT_RANGE 1200.0f
#define GHOST_SPIT_ROLL 0x78
#define GHOST_SPIT_ROLL_FAR 0x69
#define GHOST_SEEN_ANGLE 1.5708f        // the player faces it within this angle: it vanishes
#define GHOST_SIDESTEP_FORWARD 1.57075f
#define GHOST_SIDESTEP_BACK 4.71225f
#define GHOST_SIDESTEP_PERIOD 7
#define GHOST_SIDESTEP_FRAMES 0x1e
#define GHOST_SIDESTEP_SPEED (-8.0f)
#define GHOST_SIDESTEP_LIFT 20.0f
#define GHOST_MOTION_WALK 0
#define GHOST_MOTION_RUN 1
#define GHOST_MOTION_IDLE 2
#define GHOST_MOTION_LUNGE 4
#define GHOST_MOTION_HIDDEN 7
#define GHOST_MOTION_APPEAR 8
#define GHOST_MOTION_VANISH 9
#define GHOST_MOTION_REAPPEAR 10
#define GHOST_MOTION_SPIT 11
#define GHOST_MOTION_BLEND 5
#define GHOST_RUN_SPEED 10.0f           // a walk faster than this (times the motion speed scale) is played as a run
#define GHOST_FADE_FRAMES 10.0f
#define GHOST_VANISH_WAIT 0x3c
#define GHOST_LUNGE_HIT_FROM 3          // frames 4 to 34 of the lunge hit (DAT_1021f6c0 set)
#define GHOST_LUNGE_HIT_TO 0x23
#define GHOST_LUNGE_SLOW_BEFORE 5       // before frame 5 and after frame 30 the lunge only creeps
#define GHOST_LUNGE_SLOW_AFTER 0x1e
#define GHOST_LUNGE_FAST (-15.0f)
#define GHOST_LUNGE_SLOW (-0.5f)
#define GHOST_SPIT_FRAME 0xc0           // motion frame at which the spit starts
#define GHOST_SPIT_JUMP_END 0xcc        // frames 0xc1 to 0xcb jump forward
#define GHOST_SPIT_END_FRAME 0xd1
#define GHOST_SPIT_JUMP_SPEED 12.0f
#define GHOST_SPIT_SCALE 1.1f
#define GHOST_APPEAR_CAPTION_FRAME 0xa2
#define GHOST_APPEAR_CAPTION "1919"
#define GHOST_EFFECT_SPIT 0x762
#define GHOST_EFFECT_HIDDEN 0x763
#define GHOST_EFFECT_FADE_OUT 0x764
#define GHOST_EFFECT_FADE_IN 0x765
#define GHOST_EFFECT_LUNGE 0x655
#define GHOST_LUNGE_SOUND 0x2cc
#define GHOST_APPEAR_SOUND 0x226
#define GHOST_OWNER_OFFSET 100          // effect owners are character index + 100

// 0x10075350 (Enemy_ShyGhost). Returns 1 while DAT_10210d30 is set, 0 otherwise. Dropping below the pit floor inside
// the pit area (with a pit on the map) kills it. Every 15th frame (by slot) it tests the line to the player. Its
// pelvis matrix is copied (and lowered by 10) as the attachment of its effects. A hit (DAT_102550e0) freezes the
// steps except the sink; 46 frames after it, it sinks (step 8). Steps: start binds the head, spine and pelvis nodes,
// hides it and starts the hidden motion; hidden shows it (step 5) within 300 units, or when the player looks away
// and it is in the open (or already alerted, within 500 units), and goes to step 7 when the player looks well away;
// stalk (2) turns to face the player and, within 350 units, lunges (3, 3 % a frame) or spits (4, 1.2 %), else
// sidesteps and closes in, and vanishes (6) when the player faces it or a wall is in the way; lunge moves fast
// from frame 5 to 30 and hits from 4 to 34; spit waits for motion frame 0xc0, spits an effect from its head and
// jumps forward until frame 0xd1; steps 5 to 8 fade it in or out over ten frames.
int Enemy_ShyGhost(int index)
{
    if (DAT_10210d30[index] != 0)
        return 1;
    if (DAT_104483e8[index] == 0)
        return 0;
    if (DAT_10210b30[index] != 0)
        DAT_10210b30[index] = 0;
    Vec4 player;
    Vec4 self;
    Vec4 toPlayer;
    Vec4 move;
    int motion = -1;
    int moveFlag = 0;
    player.Copy(DAT_10db6ee8[0].matrix.GetTranslation());
    self.Copy(DAT_10db6ee8[index].matrix.GetTranslation());
    toPlayer.Copy(player - self);
    toPlayer.Length();
    float distance = toPlayer.Length2D();
    float bearing;
    bearing = Atan2(DAT_118e69e8.x - self.x, DAT_118e69e8.y - self.y);
    DAT_10255a88[index] = 0;
    if (self.z < DAT_104bfe24)
    {
        int y = (int)self.y;
        int x = (int)self.x;
        if (x >= (int)DAT_104d29e8.x && y >= (int)DAT_104d29e8.y && x <= (int)DAT_104d29f8.x &&
            y <= (int)DAT_104d29f8.y && DAT_104bfe28 != 0)
            DAT_10daaae8[index].stats[STAT_HEALTH] = 0;
    }
    if ((int)DAT_104c0380 % GHOST_SIGHT_PERIOD == index % GHOST_SIGHT_PERIOD)
    {
        Vec4 from;
        Vec4 to;
        Vec4 hit;
        from.Copy(player);
        to.Copy(self);
        to.z = to.z + GHOST_SIGHT_LIFT;
        from.z = to.z;
        DAT_11b002c0 = DAT_10db6ee8[index].collisionFilter;
        if (DAT_10255f08 == 0 || DAT_10255f08->Collide(0, GHOST_SIGHT_MASK, &from, &to, &hit) == 0)
            DAT_1020b430[index].sightBlocked = 0;
        else
            DAT_1020b430[index].sightBlocked = 1;
        DAT_11b002c0 = 0;
    }
    if (DAT_1021bdf8[index][2] != 0)
    {
        DAT_10868838[index][1] = DAT_1021bdf8[index][2]->worldMatrix;
        DAT_10868838[index][1].m[MATRIX_TRANSLATION_Z] -= GHOST_PELVIS_DROP;
    }
    if (DAT_102550e0[index] == 0)
    {
        if (DAT_1020b430[index].hurtFrames > 0)
            DAT_1020b430[index].hurtFrames--;
    }
    if (DAT_102550e0[index] > 0 && DAT_10daaae8[index].held == 0)
    {
        if (++DAT_1020b430[index].hurtFrames > GHOST_STRIKE_FRAMES)
        {
            DAT_10daaae8[index].held = 1;
            DAT_1020b430[index].hurtFrames = 0;
            DAT_1020b430[index].state = GHOST_STEP_SINK;
            DAT_1020b430[index].wait = 0;
            motion = GHOST_MOTION_VANISH;
            EffectOptions options;
            Vec4 offset;
            options.owner = index;
            FUN_1001d8b0(index, GHOST_EFFECT_FADE_OUT, offset, -1, -1, &DAT_10868838[index][1], 0, 0, 0, &options);
        }
    }
    if (DAT_1020b430[index].state == GHOST_STEP_SINK || DAT_102550e0[index] == 0)
    {
        if (DAT_1020b430[index].state == GHOST_STEP_START)
        {
            DAT_1021bdf8[index][0] = DAT_10db6ee8[index].root->Find(GHOST_HEAD_NODE, 0);
            DAT_1021bdf8[index][1] = DAT_10db6ee8[index].root->Find(GHOST_SPINE_NODE, 0);
            DAT_1021bdf8[index][2] = DAT_10db6ee8[index].root->Find(GHOST_PELVIS_NODE, 0);
            DAT_1020b430[index].state = GHOST_STEP_HIDDEN;
            DAT_1020b430[index].wait = 0;
            DAT_1020b430[index].sightBlocked = 1;
            DAT_10db6ee8[index].PlayMotion(GHOST_MOTION_HIDDEN, 1, 0, 1.0f);
            DAT_10db6ee8[index].heading = bearing;
            DAT_10db6ee8[index].targetHeading = bearing;
            EffectOptions options;
            Vec4 offset;
            options.owner = index;
            FUN_1001d8b0(index, GHOST_EFFECT_HIDDEN, offset, -1, -1, &DAT_10868838[index][1], 0, 0, 0, &options);
            DAT_10db6ee8[index].alpha = 0.0f;
            DAT_10daaae8[index].stats[STAT_BUSY] = 1;
        }
        else if (DAT_1020b430[index].state == GHOST_STEP_HIDDEN)
        {
            DAT_10db6ee8[index].alpha = 0.0f;
            DAT_10daaae8[index].stats[STAT_BUSY] = 1;
            float angle = atan2f(toPlayer.x, toPlayer.y);
            float turn = DAT_10db6ee8[0].heading - angle;
            while (turn > GHOST_PI)
                turn = turn - GHOST_FULL_TURN;
            while (turn < -GHOST_PI)
                turn = turn + GHOST_FULL_TURN;
            DAT_10db6ee8[index].targetHeading = bearing;
            motion = GHOST_MOTION_HIDDEN;
            if (distance < GHOST_NEAR_RANGE || (DAT_1020b430[index].hurtFrames != 0 && DAT_1020b430[index].alerted == 0) ||
                (distance < GHOST_LOOK_RANGE && DAT_1020b430[index].sightBlocked == 0 &&
                 DAT_1020b430[index].alerted == 0 && floorf(turn) > GHOST_LOOK_AWAY))
            {
                DAT_1020b430[index].state = GHOST_STEP_APPEAR;
                DAT_1020b430[index].wait = 0;
                DAT_1020b430[index].alerted = 1;
                motion = GHOST_MOTION_APPEAR;
                EffectOptions options;
                Vec4 offset;
                FUN_1001d8b0(index, GHOST_EFFECT_FADE_IN, offset, -1, -1, &DAT_10868838[index][1], 0, 0, 0, &options);
            }
            else if (DAT_1020b430[index].sightBlocked == 0 && floorf(turn) > GHOST_LOOK_AWAY_MOVE &&
                     DAT_1020b430[index].alerted != 0)
            {
                DAT_1020b430[index].state = GHOST_STEP_REAPPEAR;
                DAT_1020b430[index].wait = 0;
                motion = GHOST_MOTION_REAPPEAR;
                EffectOptions options;
                Vec4 offset;
                FUN_1001d8b0(index, GHOST_EFFECT_FADE_IN, offset, -1, -1, &DAT_10868838[index][1], 0, 0, 0, &options);
            }
        }
        else if (DAT_1020b430[index].state == GHOST_STEP_STALK)
        {
            if (distance < GHOST_STALK_RANGE)
            {
                DAT_1020b430[index].sightBlocked = 0;
                DAT_10db6ee8[index].targetHeading = atan2f(toPlayer.x, toPlayer.y) + GHOST_PI;
                float turn = DAT_10db6ee8[index].targetHeading - DAT_10db6ee8[index].heading;
                while (turn > GHOST_PI)
                    turn = turn - GHOST_FULL_TURN;
                while (turn < -GHOST_PI)
                    turn = turn + GHOST_FULL_TURN;
                if (floorf(turn) > GHOST_FACING_LIMIT)
                    motion = GHOST_MOTION_WALK;
                else
                    motion = GHOST_MOTION_IDLE;
                if (FUN_1013f880() % GHOST_ROLL_RANGE < GHOST_LUNGE_ROLL)
                {
                    DAT_1020b430[index].state = GHOST_STEP_LUNGE;
                    DAT_1020b430[index].wait = 0;
                    motion = GHOST_MOTION_LUNGE;
                }
                if (distance < GHOST_SPIT_RANGE && FUN_1013f880() % GHOST_ROLL_RANGE < GHOST_SPIT_ROLL)
                {
                    motion = GHOST_MOTION_SPIT;
                    DAT_1020b430[index].state = GHOST_STEP_SPIT;
                    DAT_1020b430[index].wait = 0;
                }
            }
            else
            {
                float turn = DAT_10db6ee8[0].heading - atan2f(toPlayer.x, toPlayer.y);
                while (turn > GHOST_PI)
                    turn = turn - GHOST_FULL_TURN;
                while (turn < -GHOST_PI)
                    turn = turn + GHOST_FULL_TURN;
                if (DAT_1020b430[index].sightBlocked == 1 || floorf(turn) < GHOST_SEEN_ANGLE)
                {
                    DAT_1020b430[index].state = GHOST_STEP_VANISH;
                    DAT_1020b430[index].wait = 0;
                    motion = GHOST_MOTION_VANISH;
                    EffectOptions options;
                    Vec4 offset;
                    options.owner = index;
                    FUN_1001d8b0(index, GHOST_EFFECT_FADE_OUT, offset, -1, -1, &DAT_10868838[index][1], 0, 0, 0,
                                 &options);
                }
                else
                {
                    motion = GHOST_MOTION_WALK;
                    if (DAT_1020b430[index].strafeFrames == 0)
                        DAT_10db6ee8[index].targetHeading = atan2f(toPlayer.x, toPlayer.y) + GHOST_PI;
                    else if (DAT_1020b430[index].strafeFrames < 1)
                    {
                        DAT_1020b430[index].strafeFrames++;
                        DAT_10db6ee8[index].targetHeading = atan2f(toPlayer.x, toPlayer.y) + GHOST_SIDESTEP_FORWARD;
                    }
                    else
                    {
                        DAT_1020b430[index].strafeFrames--;
                        DAT_10db6ee8[index].targetHeading = atan2f(toPlayer.x, toPlayer.y) + GHOST_SIDESTEP_BACK;
                    }
                    moveFlag = 1;
                    move.y = GHOST_SIDESTEP_SPEED;
                    move.RotateZ(-DAT_10db6ee8[index].heading);
                    move.z = GHOST_SIDESTEP_LIFT;
                    if ((int)DAT_104c0380 % GHOST_SIDESTEP_PERIOD == index % GHOST_SIDESTEP_PERIOD &&
                        (DAT_10db6ee8[index].IsClearOfMapLimited(&move) == 0 || DAT_10db6ee8[index].IsClearAny(&move) == 0))
                    {
                        if (DAT_1020b430[index].strafeFrames > 0)
                            DAT_1020b430[index].strafeFrames = -GHOST_SIDESTEP_FRAMES;
                        else if (DAT_1020b430[index].strafeFrames < 0)
                            DAT_1020b430[index].strafeFrames = GHOST_SIDESTEP_FRAMES;
                        else
                            DAT_1020b430[index].strafeFrames = (FUN_1013f880() % 2 != 0 ? -1 : 1) * GHOST_SIDESTEP_FRAMES;
                    }
                    move.z = 0.0f;
                    if (distance < GHOST_SPIT_RANGE && FUN_1013f880() % GHOST_ROLL_RANGE < GHOST_SPIT_ROLL_FAR)
                    {
                        motion = GHOST_MOTION_SPIT;
                        DAT_1020b430[index].state = GHOST_STEP_SPIT;
                        DAT_1020b430[index].wait = 0;
                    }
                }
            }
        }
        else if (DAT_1020b430[index].state == GHOST_STEP_LUNGE)
        {
            motion = GHOST_MOTION_LUNGE;
            if (DAT_10db6ee8[index].motion->blending == 0 && DAT_10db6ee8[index].motion->layer != GHOST_MOTION_LUNGE)
            {
                DAT_1020b430[index].state = GHOST_STEP_STALK;
                DAT_1020b430[index].wait = 0;
                DAT_1021f6c0[index] = 0;
                DAT_10daaa80.RemoveOwnedBy(index + GHOST_OWNER_OFFSET);
                motion = GHOST_MOTION_IDLE;
            }
            else
            {
                if (++DAT_1020b430[index].wait == 1)
                {
                    Matrix4 rotation;
                    Vec4 offset;
                    rotation = RotationZ(DAT_10db6ee8[index].heading);
                    EffectOptions options;
                    options.owner = index + GHOST_OWNER_OFFSET;
                    FUN_1001d8b0(index, GHOST_EFFECT_LUNGE, offset, -1, -1, &DAT_1021bdf8[index][1]->worldMatrix, 0, 0,
                                 &rotation, &options);
                    FUN_10065b10(index, GHOST_LUNGE_SOUND, 0, 0);
                }
                int frame = DAT_1020b430[index].wait;
                if (frame > GHOST_LUNGE_HIT_FROM && frame < GHOST_LUNGE_HIT_TO)
                    DAT_1021f6c0[index] = 1;
                if (frame < GHOST_LUNGE_SLOW_BEFORE || frame > GHOST_LUNGE_SLOW_AFTER)
                    move.y = GHOST_LUNGE_SLOW;
                else
                    move.y = GHOST_LUNGE_FAST;
                move.RotateZ(-DAT_10db6ee8[index].heading);
                moveFlag = 1;
            }
        }
        else if (DAT_1020b430[index].state == GHOST_STEP_SPIT)
        {
            motion = GHOST_MOTION_SPIT;
            if (DAT_10db6ee8[index].motionFrame < GHOST_SPIT_FRAME)
                DAT_10db6ee8[index].targetHeading = atan2f(toPlayer.x, toPlayer.y) + GHOST_PI;
            int frame = DAT_10db6ee8[index].motionFrame;
            if (frame == GHOST_SPIT_FRAME)
            {
                EffectOptions options;
                Vec4 offset;
                Matrix4 rotation;
                options.m_0004 = DAT_10daaae8[index].stats[STAT_EFFECT_PARAM];
                options.m_0008 = GHOST_SPIT_SCALE;
                options.m_0000 = 0;
                rotation = RotationZ(DAT_10db6ee8[index].targetHeading);
                rotation.m[MATRIX_TRANSLATION_X] = DAT_1021bdf8[index][0]->worldMatrix.m[MATRIX_TRANSLATION_X];
                rotation.m[MATRIX_TRANSLATION_Y] = DAT_1021bdf8[index][0]->worldMatrix.m[MATRIX_TRANSLATION_Y];
                rotation.m[MATRIX_TRANSLATION_Z] = DAT_1021bdf8[index][0]->worldMatrix.m[MATRIX_TRANSLATION_Z];
                FUN_1001d8b0(index, GHOST_EFFECT_SPIT, offset, -1, -1, 0, 0, 0, &rotation, &options);
            }
            else if (frame <= GHOST_SPIT_FRAME || frame >= GHOST_SPIT_JUMP_END)
            {
                if (frame == GHOST_SPIT_END_FRAME)
                {
                    motion = GHOST_MOTION_IDLE;
                    DAT_1020b430[index].state = GHOST_STEP_STALK;
                    DAT_1020b430[index].wait = 0;
                }
            }
            else
            {
                move.y = (float)(GHOST_SPIT_END_FRAME - frame) * GHOST_SPIT_JUMP_SPEED / GHOST_FADE_FRAMES;
                move.RotateZ(-DAT_10db6ee8[index].heading);
                moveFlag = 1;
            }
        }
        else if (DAT_1020b430[index].state == GHOST_STEP_APPEAR)
        {
            if (DAT_1020b430[index].wait == 0)
                DAT_10daaa80.RemoveOwnedBy(index);
            float fade = (float)++DAT_1020b430[index].wait / GHOST_FADE_FRAMES;
            DAT_10db6ee8[index].alpha = (1.0f > fade) ? fade : 1.0f;
            DAT_10daaae8[index].stats[STAT_BUSY] = 1;
            motion = GHOST_MOTION_APPEAR;
            if (DAT_10db6ee8[index].motionFrame == GHOST_APPEAR_CAPTION_FRAME)
            {
                FUN_1016b230(GHOST_APPEAR_CAPTION);
                FUN_10055ee0();
                FUN_10065b10(index, GHOST_APPEAR_SOUND, 0, 0);
            }
            if (DAT_10db6ee8[index].motion->blending == 0 && DAT_10db6ee8[index].motion->layer != GHOST_MOTION_APPEAR)
            {
                motion = GHOST_MOTION_IDLE;
                DAT_1020b430[index].state = GHOST_STEP_STALK;
                DAT_1020b430[index].wait = 0;
                DAT_10db6ee8[index].alpha = 1.0f;
                DAT_10daaae8[index].stats[STAT_BUSY] = 0;
            }
            DAT_10db6ee8[index].targetHeading = atan2f(toPlayer.x, toPlayer.y) + GHOST_PI;
        }
        else if (DAT_1020b430[index].state == GHOST_STEP_VANISH)
        {
            int step = ++DAT_1020b430[index].wait;
            motion = GHOST_MOTION_VANISH;
            float fade = (float)(10 - step) / GHOST_FADE_FRAMES;
            DAT_10db6ee8[index].alpha = (0.0f < fade) ? fade : 0.0f;
            DAT_10daaae8[index].stats[STAT_BUSY] = 0;
            if (DAT_10db6ee8[index].motion->blending == 0 && DAT_10db6ee8[index].motion->layer != GHOST_MOTION_VANISH)
            {
                motion = GHOST_MOTION_HIDDEN;
                DAT_1020b430[index].state = GHOST_STEP_HIDDEN;
                DAT_1020b430[index].wait = 0;
                DAT_10db6ee8[index].alpha = 0.0f;
                DAT_10daaae8[index].stats[STAT_BUSY] = 1;
            }
            DAT_10db6ee8[index].targetHeading = atan2f(toPlayer.x, toPlayer.y);
        }
        else if (DAT_1020b430[index].state == GHOST_STEP_REAPPEAR)
        {
            if (DAT_1020b430[index].wait == 0)
                DAT_10daaa80.RemoveOwnedBy(index);
            float fade = (float)++DAT_1020b430[index].wait / GHOST_FADE_FRAMES;
            DAT_10db6ee8[index].alpha = (1.0f > fade) ? fade : 1.0f;
            DAT_10daaae8[index].stats[STAT_BUSY] = 1;
            motion = GHOST_MOTION_REAPPEAR;
            if (DAT_10db6ee8[index].motion->blending == 0 && DAT_10db6ee8[index].motion->layer != GHOST_MOTION_REAPPEAR)
            {
                motion = GHOST_MOTION_IDLE;
                DAT_1020b430[index].state = GHOST_STEP_STALK;
                DAT_1020b430[index].wait = 0;
                DAT_10db6ee8[index].alpha = 1.0f;
                DAT_10daaae8[index].stats[STAT_BUSY] = 0;
            }
            DAT_10db6ee8[index].targetHeading = atan2f(toPlayer.x, toPlayer.y) + GHOST_PI;
        }
        else if (DAT_1020b430[index].state == GHOST_STEP_SINK)
        {
            int step = ++DAT_1020b430[index].wait;
            motion = GHOST_MOTION_VANISH;
            float fade = (float)(10 - step) / GHOST_FADE_FRAMES;
            DAT_10db6ee8[index].alpha = (0.0f < fade) ? fade : 0.0f;
            DAT_10daaae8[index].stats[STAT_BUSY] = 1;
            float angle = atan2f(toPlayer.x, toPlayer.y);
            float turn = DAT_10db6ee8[0].heading - angle;
            while (turn > GHOST_PI)
                turn = turn - GHOST_FULL_TURN;
            while (turn < -GHOST_PI)
                turn = turn + GHOST_FULL_TURN;
            if (DAT_1020b430[index].wait > GHOST_VANISH_WAIT && floorf(turn) > GHOST_LOOK_AWAY_MOVE)
            {
                DAT_1020b430[index].wait = 0;
                DAT_10daaae8[index].stats[STAT_BUSY] = 0;
                DAT_10daaae8[index].held = 0;
                DAT_1020b430[index].state = GHOST_STEP_REAPPEAR;
                motion = GHOST_MOTION_REAPPEAR;
                EffectOptions options;
                Vec4 offset;
                FUN_1001d8b0(index, GHOST_EFFECT_FADE_IN, offset, -1, -1, &DAT_10868838[index][1], 0, 0, 0, &options);
            }
            DAT_10db6ee8[index].targetHeading = atan2f(toPlayer.x, toPlayer.y);
        }
    }
    if (motion < 0)
        return 0;
    if (motion == GHOST_MOTION_WALK && move.Length2D() * DAT_10db6ee8[index].speedScale > GHOST_RUN_SPEED)
        motion = GHOST_MOTION_RUN;
    DAT_10db6ee8[index].PlayMotion(motion, 0, GHOST_MOTION_BLEND, 1.0f);
    if (moveFlag == 1)
    {
        FUN_100241e0(index, move.x, move.y, move.z);
        DAT_10db6ee8[index].move.x = DAT_10db6ee8[index].speedScale * DAT_10db6ee8[index].move.x;
        DAT_10db6ee8[index].move.y = DAT_10db6ee8[index].speedScale * DAT_10db6ee8[index].move.y;
        DAT_10db6ee8[index].move.z = DAT_10db6ee8[index].speedScale * DAT_10db6ee8[index].move.z;
    }
    return 0;
}
