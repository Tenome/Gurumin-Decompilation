// Enemy behaviours used by the AI updaters (enemy_ai.cpp). Each one runs for several frames and returns 1 on the
// frame it finishes. Per-enemy state lives in CharState (world.h): m_0004[0] = behaviour running,
// m_0004[1] = phase, m_0004[2] = frames left in the phase.
#include <math.h>
#include "world.h"

#include "vec4.h"

#include "last_position.h"

// Char_ProbeAhead (collide_query.cpp): casts a ray `depth` ahead of character `index`.
extern "C" int FUN_100242c0(int index, Vec4 *ground, float depth, int fromHead, int testOnly, const Vec4 *offset);

#include "node.h"
#include "anim_model.h"
#include "char_state.h"

#include "char_tables.h"

extern "C" int FUN_1013f880(void);  // the game's random number

// atan2 on floats (an inline helper: both arguments are rounded to float first, the second one first).
inline float Atan2(float y, float x)
{
    return (float)atan2(y, x);
}

// Absolute value of a float (an inline helper: its argument is rounded to float first).
inline float AbsFloat(float value)
{
    return (float)fabs(value);
}

// 0x10024d00 (Enemy_CanReturnHome). 1 when enemy `index` may head for its start position in direction (dirX, dirY):
// always when `distance` is below `minimum` or below `minimumHome`; otherwise only when nothing blocks the way to the
// start position (a test ray 10 units up) and that way is less than 150 degrees away from the direction. The sixth
// and seventh parameters are not used.
extern "C" int FUN_10024d00(int index, float distance, float minimumHome, float dirX, float dirY, int unused6,
                            int unused7, float minimum)
{
    if (distance < minimum || distance < minimumHome)
        return 1;
    Vec4 offset;
    offset.Fill(0);
    offset.z += 10.0f;
    float homeDistance = (DAT_10802bf8[index].position - DAT_10db6ee8[index].matrix.GetTranslation()).Length2D();
    if (FUN_100242c0(index, 0, homeDistance, 0, 1, &offset) == 0)
        return 0;
    Vec4 home;
    home.Copy(DAT_10802bf8[index].position - DAT_10db6ee8[index].matrix.GetTranslation());
    if (AbsFloat(Atan2(home.x, home.y) - Atan2(dirX, dirY)) < 2.61799f)
        return 1;
    return 0;
}

// 0x10027560 (Enemy_IdleMotion). Plays one of the two idle motions, chosen at random (motion 5 for 146 frames or
// motion 6 for 93 frames), and waits for it to end. Returns 1 when it has ended, else 0.
extern "C" int FUN_10027560(int index)
{
    DAT_1020b430[index].values[0] = 1;
    if (DAT_1020b430[index].values[1] == 0)
    {
        DAT_1020b430[index].values[2] = 0;
        switch (FUN_1013f880() % 2)
        {
        case 0:
            DAT_10db6ee8[index].PlayMotion(5, 1, 5, 1.0f);
            DAT_1020b430[index].values[2] = 146;
            DAT_1020b430[index].values[1] = 1;
            return 0;
        case 1:
            DAT_10db6ee8[index].PlayMotion(6, 1, 5, 1.0f);
            DAT_1020b430[index].values[2] = 93;
            DAT_1020b430[index].values[1] = 1;
            break;
        }
    }
    else if (DAT_1020b430[index].values[1] == 1)
    {
        DAT_1020b430[index].values[2]--;
        if (DAT_1020b430[index].values[2] == 0)
        {
            DAT_1020b430[index].values[1] = 0;
            return 1;
        }
    }
    return 0;
}

// 0x10027400 (Enemy_WatchPlayer). Starts motion 7 (unless it is already playing or being blended into) and then
// keeps the enemy turned towards the player until a 15-in-3000 chance per frame ends the behaviour. Returns 1 on
// the frame it ends, else 0.
extern "C" int FUN_10027400(int index)
{
    Vec4 move;
    Vec4 player;
    Vec4 self;
    player.Copy(DAT_10db6ee8[0].matrix.GetTranslation());
    self.Copy(DAT_10db6ee8[index].matrix.GetTranslation());
    DAT_1020b430[index].values[0] = 1;
    move.Fill(0);
    if (DAT_1020b430[index].values[1] == 0)
    {
        Playback *motion = DAT_10db6ee8[index].motion;
        DAT_1020b430[index].values[2] = 150;
        if (motion->layer != 7 && (motion->blending == 0 || motion->nextLayer != 7))
            DAT_10db6ee8[index].PlayMotion(7, 1, 5, 1.0f);
        DAT_1020b430[index].values[1] = 1;
    }
    else if (DAT_1020b430[index].values[1] == 1)
    {
        DAT_10db6ee8[index].targetHeading = Atan2(player.x - self.x, player.y - self.y) + 3.1415f;
        if (FUN_1013f880() % 3000 < 15)
        {
            DAT_1020b430[index].values[1] = 0;
            return 1;
        }
    }
    return 0;
}

// 0x10028360 (Enemy_TurnToPlayer). Turns the enemy towards the player, at most 0.349 radians (20 degrees) per frame,
// playing walk-in-place motion 0 while it turns and idle motion 2 otherwise. Facing the player (within 0.05236
// radians) and closer than 1500 units it keeps motion 0 for the remaining turn frames. The first call sets up the
// model's node flags. The behaviour ends with a chance of `endChance` in 400 per frame; returns 1 then, else 0.
// CharState: m_003c = set up, m_0004[2] = turn frames left, m_0004[3] = cooldown.
extern "C" int FUN_10028360(int index, int endChance)
{
    Vec4 player;
    Vec4 self;
    Vec4 toPlayer;
    DAT_1020b430[index].values[0] = 0;
    if (DAT_1020b430[index].behaviour == 0)
    {
        DAT_1020b430[index].behaviour = 1;
        DAT_10db6ee8[index].root->M_10194630((char)index, 0x400);
        DAT_10db6ee8[index].root->M_101946b0(0xfe, 0x400);
        DAT_10db6ee8[index].root->M_10194730(0x2f, 0x400);
        DAT_10db6ee8[index].root->M_10194730(0x2f, 0x200);
        DAT_1021f6c0[index] = 0;
    }
    player.Copy(DAT_10db6ee8[0].matrix.GetTranslation());
    self.Copy(DAT_10db6ee8[index].matrix.GetTranslation());
    toPlayer.Copy(player - self);
    if (DAT_1020b430[index].values[1] == 0)
    {
        // Angle still to turn, wrapped into -pi..pi.
        float turn = (float)atan2(toPlayer.x, toPlayer.y);
        turn = turn + 3.1415f;
        turn = turn - DAT_10db6ee8[index].heading;
        if (turn > 3.1415f)
            turn = -(6.28319f - turn);
        else if (turn < -3.1415f)
            turn = turn + 6.28319f;
        if ((float)fabs(turn) < 0.05236f && toPlayer.Length2D() < 1500.0f)
        {
            if (DAT_1020b430[index].values[3] == 0)
                DAT_1020b430[index].values[3] = 30;
            DAT_10db6ee8[index].targetHeading = DAT_10db6ee8[index].heading;
            if (DAT_1020b430[index].values[2] > 0)
            {
                DAT_10db6ee8[index].PlayMotion(0, 0, 5, 1.0f);
                DAT_1020b430[index].values[2]--;
            }
            else
            {
                DAT_10db6ee8[index].PlayMotion(2, 0, 5, 1.0f);
            }
        }
        else if (((float)fabs(turn) < 3.14159f && toPlayer.Length2D() < 1500.0f) || DAT_1020b430[index].values[3] > 0)
        {
            if (DAT_1020b430[index].values[3] == 0)
                DAT_1020b430[index].values[3] = 30;
            DAT_1020b430[index].values[2] = 4;
            if (turn > 0.0f)
            {
                if (turn > 0.349066f)
                    DAT_10db6ee8[index].targetHeading = DAT_10db6ee8[index].heading + 0.349066f;
                else
                    DAT_10db6ee8[index].targetHeading = turn + DAT_10db6ee8[index].heading;
            }
            else if (turn < 0.0f)
            {
                if (turn < -0.349066f)
                    DAT_10db6ee8[index].targetHeading = DAT_10db6ee8[index].heading - 0.349066f;
                else
                    DAT_10db6ee8[index].targetHeading = turn + DAT_10db6ee8[index].heading;
            }
            DAT_10db6ee8[index].PlayMotion(0, 0, 5, 1.0f);
        }
        else
        {
            DAT_10db6ee8[index].PlayMotion(2, 0, 5, 1.0f);
        }
        if (DAT_1020b430[index].values[3] > 0)
            DAT_1020b430[index].values[3]--;
        if (FUN_1013f880() % 400 < endChance)
        {
            DAT_1020b430[index].values[3] = 0;
            DAT_1020b430[index].values[2] = 0;
            return 1;
        }
    }
    else
    {
        DAT_10db6ee8[index].PlayMotion(2, 0, 5, 1.0f);
    }
    return 0;
}
