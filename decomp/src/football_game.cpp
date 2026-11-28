// The map 0x18 football game, one frame. C++ rather than extern "C": the function keeps an exception frame for its
// Matrix4 locals (the effect placement and the rolling matrix of the ball model).

#include <math.h>
#include <stdlib.h>

#include "vec4.h"
#include "matrix4.h"
#include "node.h"
#include "anim_model.h"
#include "effect_manager.h"
#include "game.h"
#include "game_vars.h"
#include "sound_vars.h"
#include "camera_vars.h"
#include "char_state.h"
#include "char_slot.h"
#include "char_tables.h"
#include "char_vars.h"
#include "char_hikari.h"
#include "last_position.h"
#include "map_vars.h"
#include "map_records.h"
#include "minigame_hud.h"
#include "mole_game.h"
#include "barrage_enemy.h"
#include "partner_follow.h"
#include "game_load_data.h"
#include "doppler_loops.h"
#include "soccer_ai.h"
#include "hikari_button.h"
#include "football_state.h"
#include "football_game.h"

#define FB_PI 3.1415f
#define FB_TWO_PI 6.283f
#define FB_HALF_PI 1.57075f
#define FB_SCENERY_FIRST_SLOT 0x5c          // slots 0x5c..0x7b hold the switch scenery
#define FB_SCENERY_END_SLOT 0x7c
#define FB_SCENERY_KIND_A 0x13
#define FB_SCENERY_KIND_B 0x14
#define FB_BALL_RADIUS 30.0f                // the sweep radius and the ball's resting height
#define FB_SANE_LIMIT 10000.0f              // velocity components beyond this are reset
#define FB_MAX_SPEED 90.0f                  // longer velocities are cut back to 3 * 30
#define FB_MIN_SPEED 0.5f                   // the move loop stops below this speed
#define FB_MAX_STEPS 100
#define FB_PLAYER_IDLE_FRAMES 300
#define FB_SPIN_LIMIT 0x28
#define FB_CPU_SPIN_LIMIT 0x2d
#define FB_GOAL_FRAMES 3                    // frames the ball must stay inside a net to count
#define FB_CELEBRATION_FRAMES -0x96
#define FB_CELEBRATION_FADE_OUT -0x3c
#define FB_CELEBRATION_RESET -0x1e
#define FB_BALL_PLAYER_REACH 800.0f
#define FB_EFFECT_BALL 0x76d
#define FB_SOUND_BEEP 0x221
#define FB_SOUND_BEEP_LONG 0x222
#define FB_SOUND_GOAL 0x223
#define FB_SOUND_KICK 0x21f
#define FB_EVENT_GOAL 0x1c7
#define FB_FORMATION_PERIOD 0x708
#define FB_VOICE_COOLDOWN 0x3c
#define FB_LINE_EMPTY ""
#define FB_ROSTER_END 8

#define FB_SHOT_KIND_COUNT 11

#define FB_SHOT_WILDCAT "\x83\x8f\x83" "C\x83\x8b\x83h\x83L\x83\x83\x83" "b\x83g\x83V\x83\x85\x81[\x83g"   // wild cat shot
#define FB_SHOT_GO_IN "\x93\xfc\x82\xea"   // "go in!"
#define FB_SHOT_HYPER "\x83n\x83" "C\x83p\x81[\x83X\x83y\x83V\x83\x83\x83\x8b\x83V\x83\x85\x81[\x83g\x82\xbe\x82\xd7"   // hyper special shot
#define FB_SHOT_MIRACLE "\x92\xb4\x83~\x83\x89\x83N\x83\x8b\x83" "E\x83\x8b\x83g\x83\x89\x83V\x83\x85\x81[\x83g\x82\xbe\x82\xd7"   // super miracle ultra shot
#define FB_SHOT_GO "\x82\xa2\x82\xaf"   // "go!"
#define FB_SHOT_SHOOT "\x83V\x83\x85\x81[\x83g"   // "shoot"
#define FB_SHOT_WILL_IT_GO_IN "\x93\xfc\x82\xe9\x82\xa9\x82\xc8\x81" "c\x81" "c"   // "will it go in..."
#define FB_SHOT_SHOT "\x83V\x83\x85\x81[\x83g\x82\xbe\x82\xe6"   // "a shot"
#define FB_SHOT_HOPE_IN "\x93\xfc\x82\xc1\x82\xc4\x97~\x82\xb5\x82\xa2\x82\xdb"   // "I hope it goes in"
#define FB_SHOT_SHOT_B "\x83V\x83\x85\x81[\x83g\x82\xbe\x82\xdb"   // "a shot"
#define FB_SHOT_PUT_IN_GOAL "\x83S\x81[\x83\x8b\x82\xc9\x93\xfc\x82\xea\x82\xc4"   // "put it in the goal"
#define FB_PASS_CARRY "\x83S\x81[\x83\x8b\x82\xdc\x82\xc5\x89^\x82\xf1\x82\xc5\x82\xad\x82\xea"   // "carry it to the goal"
#define FB_PASS_THAT_IS_A_PASS "\x82\xbb\x82\xea\x81" "A\x83p\x83X\x82\xbe"   // "that is a pass"
#define FB_PASS_THROW "\x91\x97\x8b\x85\x82\xbe\x82\xd7"   // "a throw"
#define FB_PASS_LEAVE_REST "\x8c\xe3\x82\xcd\x97\x8a\x82\xf1\x82\xbe"   // "I leave the rest to you"
#define FB_PASS_PASS "\x83p\x83X"   // "pass"
#define FB_PASS_WILL_IT_REACH "\x93\xcd\x82\xad\x82\xa9\x82\xc8\x81" "c\x81" "c"   // "will it reach..."
#define FB_PASS_RECEIVE "\x8e\xf3\x82\xaf\x8e\xe6\x82\xc1\x82\xc4\x97~\x82\xb5\x82\xa2\x82\xdb"   // "I want you to receive it"
#define FB_PASS_PASS_B "\x83p\x83X\x82\xbe\x82\xdb"   // "a pass"
#define FB_PASS_FRONT_LINE "\x91O\x90\xfc\x82\xd6\x82\xc2\x82\xc8\x82\xb0"   // "connect to the front line"
#define FB_PASS_GO_ON "\x82\xbb\x82\xea\x82\xa2\x82\xaf"   // "go on"
#define FB_PASS_PASS_C "\x83p\x83X\x82\xbe\x82\xd7"   // "a pass"
#define FB_PASS_ADVANCE "\x90i\x82\xdf\x90i\x82\xdf"   // "advance"
#define FB_PASS_OUR_ATTACK "\x8d\xa1\x93x\x82\xcd\x82\xb1\x82\xc1\x82\xbf\x82\xcc\x8dU\x8c\x82\x82\xbe"   // "now it is our attack"
#define FB_PASS_PASS_D "\x83p\x83X\x82\xbe\x82\xe6"   // "a pass"
#define FB_PASS_QUICK_ATTACK "\x91\xac\x8dU\x82\xbe\x82\xdb"   // "quick attack"
#define FB_CUT_NOT_LET "\x82\xbb\x82\xf1\x82\xc8\x8e\x96\x82\xcd\x82\xb3\x82\xb9\x82\xc8\x82\xa2\x82\xbc"   // "I will not let you do that"
#define FB_CUT_PASS_CUT "\x83p\x83X\x83J\x83" "b\x83g\x82\xbe\x82\xd7"   // "pass cut"
#define FB_CUT_NOT_LET_IT "\x82\xb3\x82\xb9\x82\xc4\x82\xbd\x82\xdc\x82\xe9\x82\xa9"   // "I will not let it happen"
#define FB_CUT_PASS_CUT_B "\x83p\x83X\x83J\x83" "b\x83g\x82\xbe\x82\xe6"   // "pass cut"
#define FB_CUT_CUT "\x83J\x83" "b\x83g\x82\xbe\x82\xdb"   // "cut"
#define FB_FORMATION_CHANGE "\x83t\x83H\x81[\x83\x81\x81[\x83V\x83\x87\x83\x93\x83`\x83" "F\x83\x93\x83W\x82\xbe\x81I"   // "formation change!"

// Voice lines of the players' call-outs (Message_Open on the player's character slot). The five slots are the roster of
// DAT_1020c2b0[3..7]: 0x3c, 0x3d, 0x3e, 0x3f and 0x40.
#define FB_SLOT_A 0x3c
#define FB_SLOT_B 0x3d
#define FB_SLOT_C 0x3e
#define FB_SLOT_D 0x3f
#define FB_SLOT_E 0x40

// Kick kinds (DAT_1020c2f0[0]) picked from the map area when the human kicks: 0 plain, 1 pass to a mate, 2 flat pass,
// 3 high shot, 4 hard shot, 5 and 6 lob over/under, 7 fast chip, 8 and 10 slow lobs, 9 slide.
#define FB_KICK_PLAIN 0
#define FB_KICK_PASS 1
#define FB_KICK_FLAT_PASS 2
#define FB_KICK_HIGH_SHOT 3
#define FB_KICK_HARD_SHOT 4
#define FB_KICK_LOB_UP 5
#define FB_KICK_LOB_DOWN 6
#define FB_KICK_CHIP 7
#define FB_KICK_SLOW_LOB 8
#define FB_KICK_SLIDE 9
#define FB_KICK_SLOW_LOB_B 10

#define FB_AREA_FIRST_ROLE 1

// A call-out spoken by the character in `slot`.
static __forceinline void FootballSay(int slot, const char *line)
{
    FUN_10056260(slot, line, FB_LINE_EMPTY, FB_LINE_EMPTY, FB_LINE_EMPTY);
}

// Two ways to say the same thing: the first on an even random number, the second on an odd one.
static __forceinline void FootballSayEither(int slot, const char *evenLine, const char *oddLine)
{
    if (FUN_1013f880() % 2 == 0)
        FootballSay(slot, evenLine);
    else
        FootballSay(slot, oddLine);
}

// The call-out of a player who passes the ball ahead (short pass lines of the roster's five characters).
static __forceinline void FootballSayPass(int slot)
{
    switch (slot)
    {
    case FB_SLOT_A:
        FootballSayEither(slot, FB_PASS_RECEIVE, FB_PASS_QUICK_ATTACK);
        break;
    case FB_SLOT_B:
        FootballSayEither(slot, FB_PASS_WILL_IT_REACH, FB_PASS_PASS_D);
        break;
    case FB_SLOT_C:
        FootballSayEither(slot, FB_PASS_ADVANCE, FB_PASS_OUR_ATTACK);
        break;
    case FB_SLOT_D:
        FootballSayEither(slot, FB_PASS_THROW, FB_PASS_PASS_C);
        break;
    case FB_SLOT_E:
        FootballSayEither(slot, FB_PASS_FRONT_LINE, FB_PASS_GO_ON);
        break;
    }
}

// The call-out of a player who passes the ball across to a mate.
static __forceinline void FootballSayCrossPass(int slot)
{
    switch (slot)
    {
    case FB_SLOT_A:
        FootballSayEither(slot, FB_PASS_RECEIVE, FB_PASS_PASS_B);
        break;
    case FB_SLOT_B:
        FootballSayEither(slot, FB_PASS_WILL_IT_REACH, FB_PASS_PASS_D);
        break;
    case FB_SLOT_C:
        FootballSayEither(slot, FB_PASS_LEAVE_REST, FB_PASS_PASS);
        break;
    case FB_SLOT_D:
        FootballSayEither(slot, FB_PASS_THROW, FB_PASS_PASS_C);
        break;
    case FB_SLOT_E:
        FootballSayEither(slot, FB_PASS_CARRY, FB_PASS_THAT_IS_A_PASS);
        break;
    }
}

// The call-out of a player who shoots at the goal.
static __forceinline void FootballSayShot(int slot)
{
    switch (slot)
    {
    case FB_SLOT_A:
        FootballSayEither(slot, FB_SHOT_HOPE_IN, FB_SHOT_SHOT_B);
        break;
    case FB_SLOT_B:
        FootballSayEither(slot, FB_SHOT_WILL_IT_GO_IN, FB_SHOT_SHOT);
        break;
    case FB_SLOT_C:
        FootballSayEither(slot, FB_SHOT_GO, FB_SHOT_SHOOT);
        break;
    case FB_SLOT_D:
        FootballSayEither(slot, FB_SHOT_HYPER, FB_SHOT_MIRACLE);
        break;
    case FB_SLOT_E:
        FootballSayEither(slot, FB_SHOT_WILDCAT, FB_SHOT_GO_IN);
        break;
    }
}

// The call-out of a player of the other team who steals a pass.
static __forceinline void FootballSayCut(int slot)
{
    switch (slot)
    {
    case FB_SLOT_A:
        FootballSay(slot, FB_CUT_CUT);
        break;
    case FB_SLOT_B:
        FootballSay(slot, FB_CUT_PASS_CUT_B);
        break;
    case FB_SLOT_C:
        FootballSay(slot, FB_CUT_NOT_LET_IT);
        break;
    case FB_SLOT_D:
        FootballSay(slot, FB_CUT_PASS_CUT);
        break;
    case FB_SLOT_E:
        FootballSay(slot, FB_CUT_NOT_LET);
        break;
    }
}

// Turns `angle` into -pi..pi.
static __forceinline void FootballWrapAngle(float &angle)
{
    for (; FB_PI < angle; angle -= FB_TWO_PI)
    {
    }
    for (; angle < -FB_PI; angle += FB_TWO_PI)
    {
    }
}

// Resets the components of `v` that ran away (beyond +-FB_SANE_LIMIT) to 0.
static __forceinline void FootballResetRunaway(Vec4 &v)
{
    if (v.x < -FB_SANE_LIMIT)
        v.x = 0.0f;
    if (v.y < -FB_SANE_LIMIT)
        v.y = 0.0f;
    if (v.z < -FB_SANE_LIMIT)
        v.z = 0.0f;
    if (FB_SANE_LIMIT < v.x)
        v.x = 0.0f;
    if (FB_SANE_LIMIT < v.y)
        v.y = 0.0f;
    if (FB_SANE_LIMIT < v.z)
        v.z = 0.0f;
}

// Rounds to tenths (the normals of the sweep are snapped to a coarse grid before they are classified).
static __forceinline float FootballRoundTenths(float value)
{
    return (float)(int)(value * 10.0f + 0.5f);
}

// Puts the human and the five CPU players on their kick-off spots (the human and one CPU mate on the first team's
// half, the two opponents' three on the other).
static __forceinline void FootballPlaceKickoff()
{
    DAT_10db6ee8[0].SetPosition(0.0f, -750.0f, 0.0f);
    DAT_10db6ee8[DAT_1020c2b0[3]].SetPosition(-300.0f, -750.0f, 0.0f);
    DAT_10db6ee8[DAT_1020c2b0[4]].SetPosition(300.0f, -750.0f, 0.0f);
    DAT_10db6ee8[DAT_1020c2b0[5]].SetPosition(0.0f, 750.0f, 0.0f);
    DAT_10db6ee8[DAT_1020c2b0[6]].SetPosition(-300.0f, 750.0f, 0.0f);
    DAT_10db6ee8[DAT_1020c2b0[7]].SetPosition(300.0f, 750.0f, 0.0f);
}

// Records that `holder` knows the character in the role of `member` (the role tables of soccer_ai.cpp).
static __forceinline void FootballRecordRole(int holder, int member)
{
    DAT_1020b430[holder].soccer.roleOwner[DAT_1020b430[member].soccer.role - 1] = member;
}

// 0x1007f1e0 (Map18_UpdateFootballGame). One frame of the map 0x18 football game: the human and two team mates play
// against three CPU opponents (soccer_ai.cpp) with one ball (character slot 0x3b, its velocity in DAT_10803ab8). The
// first frame prepares the scenery and puts everybody on the kick-off spots. Every frame the call-out timers and the
// score pops run, the CPU players move, then the ball is moved along its velocity in up to 100 sweeps against the map
// (bouncing off walls and the floor, spinning, losing speed), rolled on the screen and, when the human or a CPU player
// touches it, kicked (pass, long pass or one of the shot variants chosen from the map area, with the players
// call-outs). A ball inside a net for three frames scores for the other team and starts the celebration timeline
// (counting up from -150: the goal movie, the fade, the kick-off reset). Returns 0.
int Map18_UpdateFootballGame()
{
    if (DAT_1020c2e0 == 0)
    {
        for (int slot = FB_SCENERY_FIRST_SLOT; slot < FB_SCENERY_END_SLOT; slot++)
        {
            if (DAT_1021f3c0[slot] == FB_SCENERY_KIND_A || DAT_1021f3c0[slot] == FB_SCENERY_KIND_B)
                DAT_10db6ee8[slot].root->M_10194730(0x20, 0x200);
        }
        DAT_1020c2e0 = 1;
    }
    if (DAT_1020c2e8 == 0 && DAT_10c66d14 == 4)
    {
        DAT_102028cc = -1;
        DAT_1020c2e8 = 1;
        DAT_102540ca = 0;
        DAT_102540cb = 0;
        FUN_1003ad20(FB_SOUND_BEEP, 0, 1.0f);
    }
    if (DAT_10c66d14 == 4 && DAT_1020c2e8 < 0x78)
    {
        if (DAT_1020c2e8 == 0x5a)
            FUN_1003ad20(FB_SOUND_BEEP_LONG, 0, 1.0f);
        else if (DAT_1020c2e8 % 0x1e == 0)
            FUN_1003ad20(FB_SOUND_BEEP, 0, 1.0f);
        DAT_1020c2e8++;
    }
    if (DAT_10c6505c < 0x5b && DAT_10c6505c % 0x1e == 0)
        FUN_1003ad20(FB_SOUND_BEEP, 0, 1.0f);
    if (DAT_104c0135 == 0 && DAT_104bfe1c == 0)
    {
        if (DAT_1020c2ec != 0)
            DAT_1020c2ec--;
        if (DAT_1020c2e4 != 0)
            DAT_1020c2e4--;
    }
    if ((DAT_1032d69c == 0 && DAT_1020c2b0[FOOTBALL_ROSTER_STATE] == 0) || DAT_104c0135 != 0 || DAT_104bfe1c != 0)
        return 0;

    if (DAT_1020c2b0[FOOTBALL_ROSTER_STATE] == 0)
    {
        DAT_1020c270 = 0;
        DAT_1020c2f0[0].footballKick = 0;
        DAT_10db6ee8[FOOTBALL_BALL_SLOT].SetPosition((float)DAT_10210fc0[0].points[0].x,
                                                     (float)DAT_10210fc0[0].points[0].y,
                                                     (float)DAT_10210fc0[0].points[0].z);
        DAT_1020c2f4 = 0;
        DAT_1020c2f8 = 0;
        DAT_1020c2fc = 0;
        DAT_1020c300 = 0;
        DAT_10db6ee8[FOOTBALL_BALL_SLOT].locked = 1;
        DAT_10803ab8[0].Fill(0.0f);
        float elapsed = (float)DAT_1020c2b0[FOOTBALL_ROSTER_FRAMES];
        DAT_10803ab8[0].y = 0.0f;
        float speedScale = elapsed / 10000.0f + 1.0f;
        DAT_10255510[0].footballSpeedFactor = speedScale;
        if (2.0f <= speedScale)
            DAT_10255510[0].footballSpeedFactor = 2.0f;
        DAT_10daaae8[FOOTBALL_BALL_SLOT].stats[STAT_HEALTH] = 99999;
        DAT_10daaae8[FOOTBALL_BALL_SLOT].stats[STAT_MAX_HEALTH] = 99999;
        DAT_10daaae8[FOOTBALL_BALL_SLOT].stats[STAT_VALUE_4] = 99999;
        DAT_102540cb = 0;
        DAT_1030879c = 0;
        DAT_10daaae8[FOOTBALL_BALL_SLOT].kind = -1;
        DAT_10db6ee8[FOOTBALL_BALL_SLOT].root->M_10194730(0x20, 0x400);
        DAT_10db6ee8[FOOTBALL_BALL_SLOT].root->M_10194730(0x20, 0x200);
        DAT_1020c2b0[FOOTBALL_ROSTER_STATE] = 1;
        DAT_10db6ee8[FOOTBALL_BALL_SLOT].m_17e8 = 1;
        DAT_1020d473 = 1;
        if (DAT_10255f08 != 0)
        {
            for (int index = 0; index < 9; index++)
                DAT_1021c644[index] = DAT_10255f08->FindByFlags(0x24, index, 0);
        }
        DAT_1021c620 = DAT_10db6ee8[FOOTBALL_BALL_SLOT].root->Find("Bip01", 0);
        DAT_10255500 = DAT_1021c620->matrix.m[14];
        DAT_1020c2b0[3] = FB_SLOT_A;
        DAT_1020c2b0[4] = FB_SLOT_B;
        DAT_1020c2b0[5] = FB_SLOT_E;
        DAT_1020c2b0[6] = FB_SLOT_C;
        DAT_1020c2b0[7] = FB_SLOT_D;
        FootballPlaceKickoff();
        for (int member = FOOTBALL_ROSTER_HUMAN; member < FB_ROSTER_END; member++)
        {
            int slot = member == FOOTBALL_ROSTER_HUMAN ? 0 : DAT_1020c2b0[member];
            DAT_10802bf8[slot].position.Copy(DAT_10db6ee8[slot].matrix.GetTranslation());
        }
        for (int member = FOOTBALL_TEAM0_FIRST; member < FB_ROSTER_END; member++)
        {
            int slot = DAT_1020c2b0[member];
            DAT_1021f3c0[slot] = 3;
            DAT_10daaae8[slot].kind = -1;
            DAT_10daaae8[slot].m_0141 = 1;
        }
        for (int member = FOOTBALL_TEAM0_FIRST; member < FB_ROSTER_END; member++)
        {
            DAT_10db6ee8[DAT_1020c2b0[member]].root->M_10194730(0x21, 0x400);
            DAT_10db6ee8[DAT_1020c2b0[member]].root->M_10194730(0x21, 0x200);
        }
        DAT_1086e0b8[0].endA.SetIdentity();
        DAT_1021c644[0] = DAT_10db6ee8[FOOTBALL_BALL_SLOT].root->Find("@EN_006", 0);
        for (int member = FOOTBALL_TEAM0_FIRST; member < FB_ROSTER_END; member++)
        {
            for (int word = 0; word < 0xf; word++)
                DAT_1020b430[DAT_1020c2b0[member]].words[word] = 0;
        }
        return 0;
    }

    if (DAT_1020c270 > 0)
        DAT_1020c270--;
    for (int member = FOOTBALL_ROSTER_HUMAN; member < FB_ROSTER_END; member++)
    {
        if (member == FOOTBALL_ROSTER_HUMAN)
        {
            DAT_10802bf8[0].step.Copy(DAT_10db6ee8[0].matrix.GetTranslation() - DAT_10802bf8[0].position);
            DAT_10802bf8[0].position.Copy(DAT_10db6ee8[0].matrix.GetTranslation());
            if (5.0f < DAT_10802bf8[0].step.Length2D())
                DAT_1020c270 = FB_PLAYER_IDLE_FRAMES;
        }
        else
        {
            int slot = DAT_1020c2b0[member];
            DAT_10802bf8[slot].step.Copy(DAT_10db6ee8[slot].matrix.GetTranslation() - DAT_10802bf8[slot].position);
            DAT_10802bf8[slot].position.Copy(DAT_10db6ee8[slot].matrix.GetTranslation());
            DAT_1020b430[slot].soccer.roleOwner[0] = 0;
            DAT_1020b430[slot].soccer.roleOwner[1] = 0;
            DAT_1020b430[slot].soccer.roleOwner[2] = 0;
        }
    }
    if (DAT_1020c270 == 0)
    {
        DAT_10db6ee8[DAT_1020c2b0[3]].PlayMotion(SOCCER_MOTION_IDLE, 0, 5, 1.0f);
        DAT_10db6ee8[DAT_1020c2b0[4]].PlayMotion(SOCCER_MOTION_IDLE, 0, 5, 1.0f);
    }
    else
    {
        SoccerGame_UpdateCpuPlayer(DAT_1020c2b0[3], 0, 0, DAT_1020c2b0[4], DAT_10211040[0][1]);
        SoccerGame_UpdateCpuPlayer(DAT_1020c2b0[4], 0, 0, DAT_1020c2b0[3], DAT_10211040[0][2]);
    }
    SoccerGame_UpdateCpuPlayer(DAT_1020c2b0[5], 1, DAT_1020c2b0[6], DAT_1020c2b0[7], DAT_10211040[0][3]);
    SoccerGame_UpdateCpuPlayer(DAT_1020c2b0[6], 1, DAT_1020c2b0[5], DAT_1020c2b0[7], DAT_10211040[1][0]);
    SoccerGame_UpdateCpuPlayer(DAT_1020c2b0[7], 1, DAT_1020c2b0[5], DAT_1020c2b0[6], DAT_10211040[1][1]);
    DAT_1020b430[DAT_1020c2b0[3]].soccer.roleOwner[0] = 0;
    DAT_1020b430[DAT_1020c2b0[3]].soccer.roleOwner[1] = 0;
    DAT_1020b430[DAT_1020c2b0[3]].soccer.roleOwner[2] = 0;
    DAT_1020b430[DAT_1020c2b0[4]].soccer.roleOwner[0] = 0;
    DAT_1020b430[DAT_1020c2b0[4]].soccer.roleOwner[1] = 0;
    DAT_1020b430[DAT_1020c2b0[4]].soccer.roleOwner[2] = 0;
    FootballRecordRole(DAT_1020c2b0[4], DAT_1020c2b0[3]);
    FootballRecordRole(DAT_1020c2b0[4], DAT_1020c2b0[4]);
    FootballRecordRole(DAT_1020c2b0[3], DAT_1020c2b0[3]);
    FootballRecordRole(DAT_1020c2b0[3], DAT_1020c2b0[4]);
    FootballRecordRole(DAT_1020c2b0[5], DAT_1020c2b0[5]);
    FootballRecordRole(DAT_1020c2b0[5], DAT_1020c2b0[6]);
    FootballRecordRole(DAT_1020c2b0[5], DAT_1020c2b0[7]);
    FootballRecordRole(DAT_1020c2b0[6], DAT_1020c2b0[5]);
    FootballRecordRole(DAT_1020c2b0[6], DAT_1020c2b0[6]);
    FootballRecordRole(DAT_1020c2b0[6], DAT_1020c2b0[7]);
    FootballRecordRole(DAT_1020c2b0[7], DAT_1020c2b0[5]);
    FootballRecordRole(DAT_1020c2b0[7], DAT_1020c2b0[6]);
    FootballRecordRole(DAT_1020c2b0[7], DAT_1020c2b0[7]);
    DAT_1020c2b0[FOOTBALL_ROSTER_FRAMES]++;

    if (DAT_1020c2d0 < 0)
    {
        if (DAT_1020c2d0 == FB_CELEBRATION_FRAMES)
        {
            FUN_1003ad20(FB_SOUND_GOAL, 0, 1.0f);
            DAT_1020c368[0].frames = 0x96;
            DAT_1020c368[1].frames = 0x96;
            DAT_1020c368[2].frames = 0x96;
            DAT_1020c368[3].frames = 0x96;
            DAT_1020c368[4].frames = 0x96;
            FUN_1016afa0(0x3b, 0, 0, 0x172);
            FUN_1015c9e0(0xf);
            DAT_10874848.RegisterEvent(FB_EVENT_GOAL);
            FUN_10016580("goal.mmv");
        }
        if (DAT_1020c2d0 == FB_CELEBRATION_FADE_OUT)
        {
            DAT_104bf8e8 = 100.0f;
            DAT_104bf8ec = 0;
            DAT_104bf8e4 = 0x1e;
            DAT_1032d4b0 = (100.0f - DAT_104bf8f0) / 30.0f;
        }
        else if (DAT_1020c2d0 == FB_CELEBRATION_RESET)
        {
            FUN_1016afa0(-1, 0, 0, 0);
            DAT_11af3e28.x = -875.0f;
            DAT_11af3e28.y = -1350.0f;
            DAT_11af3e28.z = 370.0f;
            DAT_118e6c78.x = 0.0f;
            DAT_118e6c78.y = -750.0f;
            DAT_118e6c78.z = 30.0f;
            DAT_118e69e8.Copy(DAT_11af3e28);
            DAT_118e6cb8.Copy(DAT_118e6c78);
            DAT_10f65688.Copy(DAT_118e69e8);
            DAT_10f668e8.Copy(DAT_118e6cb8);
            FUN_1015c9e0(0xf);
            DAT_1032d4b0 = -DAT_104bf8f0 / 30.0f;
            DAT_104bf8e8 = 0.0f;
            DAT_104bf8ec = 0;
            DAT_104bf8e4 = 0x1e;
            DAT_10db6ee8[FOOTBALL_BALL_SLOT].SetPosition(0.0f, 0.0f, 100.0f);
            DAT_1020c2fc = 0;
            DAT_1020c304 = 0;
            DAT_1020c308 = 0;
            DAT_1020c2f0[0].footballKick = 0;
            float zero = 0.0f;
            DAT_10803ab8[0].Fill(zero);
            zero = 0.0f;
            DAT_10803ab8[1].Fill(zero);
            FootballPlaceKickoff();
            for (int index = 0; index < 5; index++)
            {
                DAT_10467d10[index] = 0;
                DAT_104482c8[index] = 0;
                DAT_104bfbf8[index] = 0;
            }
        }
    }
    else
    {
        DAT_10c6505c--;
        if (DAT_10c6505c % FB_FORMATION_PERIOD == 0 && DAT_10c6505c != 0)
        {
            DAT_1020b430[DAT_1020c2b0[5]].soccer.role++;
            DAT_1020b430[DAT_1020c2b0[6]].soccer.role++;
            DAT_1020b430[DAT_1020c2b0[7]].soccer.role++;
            if (DAT_1020b430[DAT_1020c2b0[5]].soccer.role == 4)
            {
                DAT_1020b430[DAT_1020c2b0[5]].soccer.role = 1;
                DAT_1020b430[DAT_1020c2b0[5]].soccer.pursuing = 0;
            }
            if (DAT_1020b430[DAT_1020c2b0[6]].soccer.role == 4)
            {
                DAT_1020b430[DAT_1020c2b0[6]].soccer.role = 1;
                DAT_1020b430[DAT_1020c2b0[6]].soccer.pursuing = 0;
            }
            if (DAT_1020b430[DAT_1020c2b0[7]].soccer.role == 4)
            {
                DAT_1020b430[DAT_1020c2b0[7]].soccer.role = 1;
                DAT_1020b430[DAT_1020c2b0[7]].soccer.pursuing = 0;
            }
            FootballSay(FB_SLOT_E, FB_FORMATION_CHANGE);
        }
    }

    Vec4 velocity;
    Vec4 ballPos;
    Vec4 target;
    if (DAT_1031538b != 0)
    {
        Matrix4 placement;
        Vec4 effectPos;
        placement.SetIdentity();
        placement.SetTranslation(DAT_10e40204.GetTranslation());
        placement.m[14] += FB_BALL_RADIUS;
        DAT_10daaa80.Start(FB_EFFECT_BALL, effectPos, -1, -1, 0, 0, 0, &placement, 0, 0);
        return 0;
    }

    DAT_10daaae8[FOOTBALL_BALL_SLOT].kind = -1;
    DAT_10255ac3 = 0;
    DAT_10db6ee8[FOOTBALL_BALL_SLOT].mode = 0;
    DAT_10db6ee8[FOOTBALL_BALL_SLOT].m_238c = 0;
    DAT_10db6ee8[FOOTBALL_BALL_SLOT].shadowMatrix.m[14] = 2.0f;
    ballPos.Copy(DAT_10e40204.GetTranslation());
    velocity.Copy(DAT_10803ab8[0]);
    FootballResetRunaway(velocity);
    FootballResetRunaway(DAT_10803ab8[0]);
    velocity.Copy(velocity * DAT_10255510[0].footballSpeedFactor);
    if (DAT_1020c304 == 0)
    {
        if (DAT_1020c308 == 0)
            DAT_1020c2fc -= 2;
        if (DAT_1020c308 % 3 == 0)
        {
            DAT_1020c2fc--;
            if (DAT_1020c2fc < -0x1e)
                DAT_1020c2fc = -0x1e;
        }
        velocity.z = (float)DAT_1020c2fc;
    }
    else
    {
        if (DAT_1020c2f8 == 0)
            DAT_10803ab8[0].Copy(DAT_10803ab8[0] * 0.9f);
        DAT_10e40204._43 = 0.0f;
    }
    if (0 < DAT_1020c2f8)
        DAT_1020c2f8--;
    if (0 < DAT_1020c308)
        DAT_1020c308--;
    if (DAT_1020c310 < 1)
        DAT_1020c30c = 0;
    else
        DAT_1020c310--;
    if (FB_MAX_SPEED < velocity.Length())
    {
        velocity.Normalized3();
        velocity.Copy(velocity * 30.0f * 3.0f);
    }
    DAT_10803ab8[0].z = (float)DAT_1020c2fc;
    Vec4 lastHit;
    Vec4 reflection;
    int previousSpin = DAT_1020c2fc;
    int step = 1;
    do
    {
        if (DAT_10803ab8[0].Length() < 0.001f || velocity.Length() < FB_MIN_SPEED)
            break;
        ballPos.Copy(DAT_10e40204.GetTranslation());
        Vec4 contact;
        Vec4 landing;
        Vec4 start;
        Vec4 normal;
        Vec4 probe;
        Vec4 push;
        float sweepDistance;
        probe.Copy(DAT_10e40204.GetTranslation());
        probe.z += FB_BALL_RADIUS;
        int hitKind = 0;
        start.Copy(probe);
        int hit = FUN_10034210(&probe, &velocity, FB_BALL_RADIUS, &contact, &sweepDistance, &push,
                               step < 2 ? 0 : &lastHit);
        if (hit == 0)
            break;
        lastHit.Copy(contact);
        if (hit == 2)
        {
            normal.Copy(contact);
            normal.Normalized3();
            normal.x = FootballRoundTenths(normal.x);
            normal.y = FootballRoundTenths(normal.y);
            normal.z = FootballRoundTenths(normal.z);
            hitKind = 1;
            if ((int)normal.x == 0)
            {
                float steepness = (float)abs((int)normal.y);
                if (steepness <= normal.z / 1.6f && 0.0f < normal.z)
                    hitKind = 2;
            }
        }
        probe.Copy(probe + push);
        sweepDistance = sqrtf(floorf(sweepDistance));
        normal.Copy(velocity);
        normal.Normalized3();
        contact.Normalized3();
        landing.Copy(probe - contact * (sweepDistance - FB_BALL_RADIUS));
        reflection.Copy(landing - start);
        if (velocity.Length() <= reflection.Length())
        {
            velocity.Fill(0.0f);
        }
        else if (0.0f <= Vec3_Dot(&velocity, &reflection))
        {
            velocity.Copy(velocity - reflection);
        }
        else
        {
            velocity.Copy(velocity + reflection);
        }
        landing.z -= FB_BALL_RADIUS;
        if (landing.z < 0.0f)
            landing.z = 0.0f;
        DAT_10db6ee8[FOOTBALL_BALL_SLOT].SetPosition(landing.x, landing.y, landing.z);
        contact.x = FootballRoundTenths(contact.x);
        contact.y = FootballRoundTenths(contact.y);
        contact.z = FootballRoundTenths(contact.z);
        float velocityHeading = atan2f(DAT_10803ab8[0].x, DAT_10803ab8[0].y) + FB_PI;
        float normalHeading = atan2f(contact.x, contact.y) + FB_PI;
        float velocityPitch = acosf(DAT_10803ab8[0].Length2D() / DAT_10803ab8[0].Length());
        if (0.0f < DAT_10803ab8[0].z)
            velocityPitch *= -1.0f;
        float normalPitch;
        if (0.0001f <= contact.Length())
        {
            normalPitch = acosf(contact.Length2D() / contact.Length());
            if (0.0f < contact.z)
                normalPitch *= -1.0f;
        }
        else
        {
            normalPitch = 0.0f;
        }
        float tilt = 1.0f;
        if (normalPitch != 0.0f)
        {
            tilt = FB_HALF_PI - floorf(normalPitch);
            tilt = floorf(tilt) / FB_HALF_PI;
        }
        normalHeading = velocityHeading - (normalHeading + FB_HALF_PI);
        normalPitch = velocityPitch - (normalPitch + FB_HALF_PI);
        FootballWrapAngle(normalHeading);
        FootballWrapAngle(normalPitch);
        if (DAT_1020c2f8 == 0)
        {
            DAT_10803ab8[0].y = DAT_10803ab8[0].Length() * 0.8f;
            velocity.y = velocity.Length() * 0.8f;
        }
        else
        {
            DAT_10803ab8[0].y = DAT_10803ab8[0].Length();
            velocity.y = velocity.Length();
        }
        DAT_10803ab8[0].x = 0.0f;
        DAT_10803ab8[0].z = 0.0f;
        velocity.x = 0.0f;
        velocity.z = 0.0f;
        velocityPitch = velocityPitch - (1.0f - tilt) * (normalPitch + normalPitch);
        DAT_10803ab8[0].RotateX(velocityPitch);
        velocity.RotateX(velocityPitch);
        DAT_10803ab8[0].y *= -1.0f;
        velocity.y *= -1.0f;
        if (hitKind == 1)
        {
            DAT_10803ab8[0].y *= 0.5f;
            velocity.y *= 0.5f;
            if (15.0f < DAT_10803ab8[0].Length())
            {
                DAT_10803ab8[0].Normalized3();
                DAT_10803ab8[0].Copy(DAT_10803ab8[0] * 15.0f);
            }
            if (15.0f < velocity.Length())
            {
                velocity.Normalized3();
                velocity.Copy(velocity * 15.0f);
            }
        }
        else if (hitKind == 2)
        {
            if (DAT_10803ab8[0].z < 0.0f)
                DAT_10803ab8[0].z *= -1.0f;
            DAT_10803ab8[0].z += 15.0f;
            if (40.0f < DAT_10803ab8[0].z)
                DAT_10803ab8[0].z = 40.0f;
            DAT_1020c2fc = (int)DAT_10803ab8[0].z;
            DAT_1020c304 = 0;
        }
        float turn = -velocityHeading - normalHeading * -2.0f * tilt;
        velocityHeading = turn;
        DAT_10803ab8[0].RotateZ(turn);
        velocity.RotateZ(turn);
        int heldFrames = DAT_1020c2f8;
        if (0.01f < contact.z || contact.z < -0.01f)
        {
            float lift = (DAT_10803ab8[0].z - 1.0f) * 1.85f;
            int spin;
            if (heldFrames == 0)
                spin = (int)((lift + (float)DAT_1020c2fc * 0.15f) / 2.05f);
            else
                spin = (int)((lift + (float)DAT_1020c2fc * 0.15f) / 1.35f);
            DAT_1020c2fc = spin;
            int allowed = abs(previousSpin) - 2;
            if (allowed < 1)
                allowed = 0;
            if (allowed <= abs(DAT_1020c2fc))
            {
                bool negative = DAT_1020c2fc < 0;
                if (heldFrames == 0)
                {
                    if (previousSpin > 0)
                        previousSpin--;
                    else if (previousSpin < 0)
                        previousSpin++;
                    int damped = abs(previousSpin) - 2;
                    if (damped < 1)
                        damped = 0;
                    DAT_1020c2fc = damped;
                }
                else
                {
                    DAT_1020c2fc = allowed;
                }
                if (negative)
                    DAT_1020c2fc = -DAT_1020c2fc;
            }
            if (abs(DAT_1020c2fc) < 2 && DAT_10e40204._43 < 10.0f)
                DAT_1020c304 = 1;
        }
        previousSpin = DAT_1020c2fc;
        step++;
    } while (step < FB_MAX_STEPS + 1);
    if (DAT_1020c2fc < FB_SPIN_LIMIT + 1)
    {
        if (DAT_1020c2fc < -FB_SPIN_LIMIT)
            DAT_1020c2fc = -FB_SPIN_LIMIT;
    }
    else
    {
        DAT_1020c2fc = FB_SPIN_LIMIT;
    }
    DAT_10daaae8[FOOTBALL_BALL_SLOT].m_0141 = 0;
    if (DAT_1020c2d0 < FB_CELEBRATION_RESET)
    {
        if (8.0f < DAT_10803ab8[0].Length2D())
        {
            DAT_10803ab8[0].Normalized3();
            DAT_10803ab8[0].Copy(DAT_10803ab8[0] * 8.0f);
        }
        if (8.0f < velocity.Length())
        {
            velocity.Normalized3();
            velocity.Copy(velocity * 8.0f);
        }
        DAT_10315426 = 0;
        DAT_1021985c = 0;
        DAT_10daaae8[FOOTBALL_BALL_SLOT].m_0141 = 1;
    }
    ballPos.Copy(DAT_10e40204.GetTranslation());
    if (0.0f <= ballPos.z + velocity.z)
    {
        if (2000.0f < ballPos.z + velocity.z + 60.0f)
            velocity.z = (2000.0f - ballPos.z) - 60.0f;
    }
    else
    {
        velocity.z = -ballPos.z;
    }
    if ((float)DAT_10210ff0 <= (ballPos.x + velocity.x) - 30.0f)
    {
        if ((float)DAT_10210ff4 < ballPos.x + velocity.x + 30.0f)
            velocity.x = ((float)DAT_10210ff4 - ballPos.x) - 30.0f;
    }
    else
    {
        velocity.x = ((float)DAT_10210ff0 - ballPos.x) + 30.0f;
    }
    float nextY = velocity.y + ballPos.y;
    float limitY = (float)DAT_10210ffc;
    if (limitY <= nextY - 30.0f)
    {
        limitY = (float)DAT_10210ff8;
        if (limitY < nextY + 30.0f)
            velocity.y = (limitY - ballPos.y) - 30.0f;
    }
    else
    {
        velocity.y = (limitY - ballPos.y) + 30.0f;
    }
    DAT_10db6ee8[FOOTBALL_BALL_SLOT].SetPosition(ballPos.x + velocity.x, velocity.y + ballPos.y, ballPos.z + velocity.z);
    ballPos.Copy(DAT_10e40204.GetTranslation());
    DAT_10db6ee8[FOOTBALL_BALL_SLOT].heading = atan2f(velocity.x, velocity.y);
    DAT_10db6ee8[FOOTBALL_BALL_SLOT].targetHeading = DAT_10db6ee8[FOOTBALL_BALL_SLOT].heading;
    DAT_10255514 = velocity.Length() / 30.0f + DAT_10255514;
    for (; FB_TWO_PI < DAT_10255514; DAT_10255514 -= FB_TWO_PI)
    {
    }
    {
        Matrix4 lift = TranslationMatrix(0.0f, 0.0f, FB_BALL_RADIUS);
        Matrix4 roll = RotationX(DAT_10255514);
        Matrix4 drop = TranslationMatrix(0.0f, 0.0f, -FB_BALL_RADIUS);
        DAT_1021c644[0]->matrix = drop * roll * lift;
    }
    {
        Matrix4 placement;
        Vec4 effectPos;
        placement.SetIdentity();
        placement.SetTranslation(DAT_10e40204.GetTranslation());
        placement.m[14] += FB_BALL_RADIUS;
        DAT_10daaa80.Start(FB_EFFECT_BALL, effectPos, -1, -1, 0, 0, 0, &placement, 0, 0);
    }
    if (DAT_1025511b != 0)
    {
        DAT_1025511b = 0;
        DAT_1020c304 = 0;
        float aim = atan2f(DAT_10e40204._41 - DAT_10db6ee8[0].matrix._41, DAT_10e40204._42 - DAT_10db6ee8[0].matrix._42);
        velocity.Copy(DAT_10e40204.GetTranslation());
        velocity.z += FB_BALL_RADIUS;
        velocity.Copy(velocity - DAT_10837378);
        if (velocity.Length2D() < 0.1f)
        {
            velocity.x = DAT_10803ab8[0].x;
            velocity.y = DAT_10803ab8[0].y;
            if (0.0f <= velocity.z)
            {
                if (velocity.Length2D() <= velocity.z)
                    velocity.z = velocity.Length2D();
            }
            else if (velocity.z <= -velocity.Length2D())
            {
                velocity.z = -velocity.Length2D();
            }
        }
        velocity.Normalized3();
        if (velocity.x * velocity.x <= velocity.y * velocity.y)
        {
            if (velocity.y < 0.0f)
                velocity.Copy(velocity.Divided(velocity.y) * -1.0f);
            else
                velocity.Copy(velocity.Divided(velocity.y));
        }
        else if (velocity.x < 0.0f)
        {
            velocity.Copy(velocity.Divided(velocity.x) * -1.0f);
        }
        else
        {
            velocity.Copy(velocity.Divided(velocity.x));
        }
        float power;
        int kick = FB_KICK_PLAIN;
        switch (DAT_104c00e8)
        {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 0x27:
            power = 30.0f;
            kick = FB_KICK_PLAIN;
            break;
        case 5:
        case 6:
        case 8:
        case 10:
            power = 35.0f;
            kick = FB_KICK_PASS;
            break;
        case 7:
        case 9:
            power = 40.0f;
            kick = FB_KICK_FLAT_PASS;
            break;
        case 0xb:
        case 0xc:
        case 0xd:
        case 0xe:
        case 0xf:
            power = 45.0f;
            kick = FB_KICK_HIGH_SHOT;
            break;
        case 0x12:
            power = 50.0f;
            kick = FB_KICK_HARD_SHOT;
            break;
        case 0x25:
            power = 30.0f;
            kick = FB_KICK_LOB_UP;
            break;
        case 0x24:
            power = 20.0f;
            kick = FB_KICK_LOB_DOWN;
            break;
        case 0x10:
        case 0x14:
            power = 50.0f;
            kick = FB_KICK_HIGH_SHOT;
            break;
        case 0x23:
        case 0x26:
        case 0x29:
        case 0x2a:
            power = 55.0f;
            kick = FB_KICK_HARD_SHOT;
            break;
        case 0x15:
            power = 55.0f;
            kick = FB_KICK_CHIP;
            break;
        case 0x17:
            power = 50.0f;
            kick = FB_KICK_SLOW_LOB;
            break;
        case 0x19:
            power = 60.0f;
            kick = FB_KICK_SLIDE;
            break;
        case 0x28:
            power = 20.0f;
            kick = FB_KICK_LOB_UP;
            break;
        case 0x16:
            power = 55.0f;
            kick = FB_KICK_SLOW_LOB_B;
            break;
        case 0x13:
        case 0x18:
            power = 50.0f;
            kick = FB_KICK_SLIDE;
            break;
        default:
            power = 27.0f;
            break;
        }
        float strength = power / velocity.Length2D();
        velocity.Copy(velocity * strength);
        float turn = atan2f(velocity.x, velocity.y) - aim;
        FootballWrapAngle(turn);
        float facing = turn * 0.125f + aim;
        velocity.y = velocity.Length2D();
        velocity.x = 0.0f;
        switch (kick)
        {
        case FB_KICK_PASS:
        {
            velocity.z *= 1.5f;
            DAT_1020c2f8 += 0x14;
            float toFirst = atan2f(DAT_10db6ee8[DAT_1020c2b0[3]].matrix._41 - DAT_10db6ee8[0].matrix._41,
                                   DAT_10db6ee8[DAT_1020c2b0[3]].matrix._42 - DAT_10db6ee8[0].matrix._42);
            float toSecond = atan2f(DAT_10db6ee8[DAT_1020c2b0[4]].matrix._41 - DAT_10db6ee8[0].matrix._41,
                                    DAT_10db6ee8[DAT_1020c2b0[4]].matrix._42 - DAT_10db6ee8[0].matrix._42);
            float firstOff = toFirst - facing;
            float secondOff = toSecond - facing;
            FootballWrapAngle(firstOff);
            FootballWrapAngle(secondOff);
            if (DAT_10db6ee8[DAT_1020c2b0[3]].matrix._42 < DAT_10db6ee8[0].matrix._42)
                firstOff = FB_PI;
            if (DAT_10db6ee8[DAT_1020c2b0[4]].matrix._42 < DAT_10db6ee8[0].matrix._42)
                secondOff = FB_PI;
            if (firstOff < 2.82735f || secondOff < 2.82735f)
            {
                if (firstOff * firstOff < secondOff * secondOff)
                    facing = firstOff * 0.75f + facing;
                else
                    facing = secondOff * 0.75f + facing;
            }
            break;
        }
        case FB_KICK_FLAT_PASS:
            velocity.z *= 1.1f;
            DAT_1020c2f8 += 5;
            break;
        case FB_KICK_HIGH_SHOT:
        {
            velocity.z *= 0.95f;
            DAT_1020c2f8 += 0x1e;
            float toGoal = atan2f(-DAT_10e40204._41, 1375.0f - DAT_10e40204._42) - facing;
            FootballWrapAngle(toGoal);
            if (1375.0f < DAT_10e40204._42)
                toGoal = FB_PI;
            facing = toGoal * 0.75f + facing;
            break;
        }
        case FB_KICK_HARD_SHOT:
        {
            velocity.z *= 0.75f;
            DAT_1020c2f8 += 0x2d;
            DAT_1020c308 += 7;
            float toGoal = atan2f(-DAT_10e40204._41, 1375.0f - DAT_10e40204._42) - facing;
            FootballWrapAngle(toGoal);
            if (DAT_10e40204._42 <= 1375.0f)
            {
                if (3.7698002f <= toGoal)
                    break;
            }
            else
            {
                toGoal = FB_PI;
            }
            facing = toGoal * 0.96f + facing;
            break;
        }
        case FB_KICK_LOB_UP:
            if (0.0f < velocity.z)
                velocity.z *= -1.0f;
            velocity.z += velocity.y;
            DAT_1020c2f8 += 0xf;
            DAT_1020c308 += 0x5a;
            velocity.y *= 0.5f;
            break;
        case FB_KICK_LOB_DOWN:
            if (velocity.z < 0.0f)
                velocity.z *= -1.0f;
            velocity.z -= velocity.y;
            DAT_1020c2f8 += 0xf;
            DAT_1020c308 += 100;
            velocity.y *= 0.5f;
            break;
        case FB_KICK_CHIP:
        {
            DAT_1020c2f8 += 0x2d;
            float toGoal = atan2f(-DAT_10e40204._41, 1375.0f - DAT_10e40204._42) - facing;
            FootballWrapAngle(toGoal);
            if (1375.0f < DAT_10e40204._42)
                toGoal = FB_PI;
            facing = toGoal + facing;
            break;
        }
        case FB_KICK_SLOW_LOB:
            velocity.z += 10.0f;
            DAT_1020c308 += 0x5a;
            DAT_1020c2f8 += 0x1e;
            break;
        case FB_KICK_SLIDE:
        {
            if (0.0f < velocity.z)
                velocity.z *= -1.0f;
            velocity.y = velocity.z + velocity.y;
            DAT_1020c2f8 += 0x1e;
            DAT_1020c308 += 0x78;
            velocity.z = 0.0f;
            float toGoal = atan2f(-DAT_10e40204._41, 1375.0f - DAT_10e40204._42) - facing;
            FootballWrapAngle(toGoal);
            if (1375.0f < DAT_10e40204._42)
                toGoal = FB_PI;
            facing = toGoal * 0.9f + facing;
            break;
        }
        case FB_KICK_SLOW_LOB_B:
            velocity.z += 5.0f;
            DAT_1020c308 += 0x78;
            DAT_1020c2f8 += 0x1e;
            break;
        }
        DAT_1020c2f0[0].footballKick = kick;
        velocity.RotateZ(-facing);
        if (1.0f < velocity.Length())
        {
            float slope = acosf(velocity.Length2D() / velocity.Length());
            if (0.8f < floorf(slope))
            {
                int lowering = 0;
                if (velocity.z < 0.0f)
                    lowering = -1;
                float heading = atan2f(velocity.x, velocity.y) + FB_PI;
                velocity.y = -velocity.Length();
                velocity.x = 0.0f;
                velocity.z = 0.0f;
                velocity.RotateX(0.8f);
                velocity.RotateZ(-heading);
                if (lowering != 0)
                    velocity.z *= -1.0f;
            }
        }
        float climb = velocity.z;
        if (floorf(velocity.z) < 7.5f)
        {
            velocity.z = -7.5f;
            if (0.0f <= climb)
                velocity.z = 7.5f;
        }
        float carried = DAT_10803ab8[0].Length() * 0.75f;
        float blend = velocity.Length() + carried;
        if (1.0f <= blend)
        {
            float shotHeading = atan2f(velocity.x, velocity.y);
            float aimHeading = atan2f(DAT_10803ab8[0].x, DAT_10803ab8[0].y);
            float share;
            if (kick == FB_KICK_HARD_SHOT || kick == FB_KICK_CHIP || kick == FB_KICK_SLIDE)
                share = DAT_10803ab8[0].Length() * 0.15f;
            else
                share = DAT_10803ab8[0].Length() * 0.25f;
            shotHeading = (aimHeading - shotHeading) * (share / blend) + shotHeading;
            DAT_10803ab8[0].Copy(DAT_10803ab8[0] * 0.15f + velocity * 0.85f);
            DAT_10803ab8[0].y = -DAT_10803ab8[0].Length2D();
            DAT_10803ab8[0].x = 0.0f;
            DAT_10803ab8[0].RotateZ(FB_PI - shotHeading);
        }
        else
        {
            DAT_10803ab8[0].Copy(DAT_10803ab8[0] * 0.15f + velocity * 0.85f);
        }
        DAT_1020c2fc += (int)velocity.z;
        if (DAT_1020c2fc < FB_SPIN_LIMIT + 1)
        {
            if (DAT_1020c2fc < -FB_SPIN_LIMIT)
                DAT_1020c2fc = -FB_SPIN_LIMIT;
        }
        else
        {
            DAT_1020c2fc = FB_SPIN_LIMIT;
        }
    }
    ballPos.Copy(DAT_10e40204.GetTranslation());
    for (int member = FOOTBALL_TEAM0_FIRST; member < FB_ROSTER_END; member++)
    {
        int slot = DAT_1020c2b0[member];
        float stepLength;
        if (member == FOOTBALL_ROSTER_HUMAN)
        {
            target.Copy(DAT_10db6ee8[0].matrix.GetTranslation());
            stepLength = DAT_10802bf8[0].step.Length();
        }
        else
        {
            target.Copy(DAT_10db6ee8[slot].matrix.GetTranslation());
            stepLength = DAT_10802bf8[slot].step.Length();
            if (DAT_1020b430[slot].soccer.delayA != 0)
                continue;
        }
        int radiusIndex = member - FOOTBALL_ROSTER_HUMAN;
        float reach = (float)DAT_10211040[radiusIndex / 4][radiusIndex % 4];
        if (!((ballPos - target).Length2D() < reach && target.z - 15.0f < ballPos.z &&
              ballPos.z < (float)DAT_10211040[radiusIndex / 4 + 2][radiusIndex % 4] + reach + target.z))
            continue;

        DAT_1020b430[slot].soccer.delayA = 7;
        DAT_1025511b = 0;
        DAT_1020c304 = 0;
        float aim = atan2f(ballPos.x - target.x, ballPos.y - target.y);
        velocity.Copy(ballPos);
        velocity.z += FB_BALL_RADIUS;
        velocity.Copy(velocity - target);
        if (target.z < ballPos.z - FB_BALL_RADIUS)
            velocity.z = 15.0f;
        if (velocity.Length2D() < 0.1f)
        {
            velocity.x = DAT_10803ab8[0].x;
            velocity.y = DAT_10803ab8[0].y;
            if (0.0f <= velocity.z)
            {
                if (velocity.Length2D() <= velocity.z)
                    velocity.z = velocity.Length2D();
            }
            else if (velocity.z <= -velocity.Length2D())
            {
                velocity.z = -velocity.Length2D();
            }
        }
        velocity.Normalized3();
        if (velocity.x * velocity.x <= velocity.y * velocity.y)
        {
            if (velocity.y < 0.0f)
                velocity.Copy(velocity.Divided(velocity.y) * -1.0f);
            else
                velocity.Copy(velocity.Divided(velocity.y));
        }
        else if (velocity.x < 0.0f)
        {
            velocity.Copy(velocity.Divided(velocity.x) * -1.0f);
        }
        else
        {
            velocity.Copy(velocity.Divided(velocity.x));
        }
        float power = DAT_10803ab8[0].Length() * 0.5f + stepLength;
        if (7.5f <= power)
        {
            if (25.0f < power)
                power = 25.0f;
        }
        else
        {
            power = 7.5f;
        }
        velocity.Copy(velocity * (power / velocity.Length2D()));
        float turn = atan2f(velocity.x, velocity.y) - aim;
        FootballWrapAngle(turn);
        float facing = turn * 0.125f + aim;
        velocity.y = velocity.Length2D();
        velocity.x = 0.0f;
        bool shot = false;
        DAT_1020c2f0[0].footballKick = 0;
        if (member >= FOOTBALL_TEAM0_FIRST)
        {
            if ((member > 4 && DAT_1020c30c == 1) || (member < 5 && DAT_1020c30c == 2))
                FootballSayCut(slot);
            int role = DAT_1020b430[slot].soccer.role;
            if (role == SOCCER_ROLE_CHASER)
            {
                target.Copy(DAT_10db6ee8[DAT_1020b430[slot].soccer.roleOwner[1]].matrix.GetTranslation());
                if (member < 5 ? target.y < DAT_10e40204._43 : DAT_10e40204._43 < target.y)
                    target.Copy(DAT_10db6ee8[DAT_1020b430[slot].soccer.roleOwner[2]].matrix.GetTranslation());
                velocity.z *= 1.5f;
                float length = velocity.Length2D();
                if (length < 0.5f)
                    length = 1.0f;
                if (length < 17.0f)
                {
                    velocity.x = velocity.x / length * 17.0f;
                    velocity.y = velocity.y / length * 17.0f;
                }
                DAT_1020c2f8 = 0xf;
                turn = atan2f(target.x - DAT_10e40204._41, target.y - DAT_10e40204._42) - facing;
                FootballWrapAngle(turn);
                DAT_1020c310 = 0x1e;
                DAT_1020c30c = 1;
                facing = turn * 0.9f + facing;
                if (4 < member)
                    DAT_1020c30c = 2;
                if (DAT_1020b430[slot].soccer.delayB == 0)
                {
                    DAT_1020b430[slot].soccer.delayB = FB_VOICE_COOLDOWN;
                    FootballSayPass(slot);
                }
            }
            else if (role == SOCCER_ROLE_BACK)
            {
                target.Copy(DAT_10db6ee8[DAT_1020b430[slot].soccer.roleOwner[2]].matrix.GetTranslation());
                if ((DAT_10e40204.GetTranslation() - target).Length2D() < 800.0f && DAT_10e40204._42 < target.y)
                {
                    velocity.z *= 1.5f;
                    float length = velocity.Length2D();
                    if (length < 0.5f)
                        length = 1.0f;
                    if (length < 17.0f)
                    {
                        velocity.x = velocity.x / length * 17.0f;
                        velocity.y = velocity.y / length * 17.0f;
                    }
                    DAT_1020c2f8 = 0xf;
                    turn = atan2f(target.x - DAT_10e40204._41, target.y - DAT_10e40204._42) - facing;
                    FootballWrapAngle(turn);
                    DAT_1020c310 = 0x1e;
                    DAT_1020c30c = 1;
                    facing = turn * 0.9f + facing;
                    if (4 < member)
                        DAT_1020c30c = 2;
                    if (DAT_1020b430[slot].soccer.delayB == 0)
                    {
                        DAT_1020b430[slot].soccer.delayB = FB_VOICE_COOLDOWN;
                        FootballSayCrossPass(slot);
                    }
                }
                else
                {
                    bool corner;
                    if (member < 5 || DAT_10e40204._42 <= (float)DAT_10211000[3].y)
                    {
                        corner = member < 5 && DAT_10e40204._42 < (float)DAT_10211000[0].y &&
                                 floorf(DAT_10e40204._41) < (float)DAT_10211000[1].x;
                    }
                    else
                    {
                        corner = floorf(DAT_10e40204._41) < (float)DAT_10211000[1].x;
                    }
                    if (corner)
                    {
                        target.Copy(DAT_10e40204.GetTranslation());
                        if (0.0f <= DAT_10e40204._41)
                            target.x = (float)DAT_10211000[1].x + 50.0f;
                        else
                            target.x = (float)DAT_10211000[0].x - 50.0f;
                        turn = atan2f(target.x - DAT_10e40204._41, target.y - DAT_10e40204._42) - facing;
                    }
                    else
                    {
                        turn = FB_PI;
                        if (member < 5)
                            turn = 0.0f;
                        turn = turn - facing;
                    }
                    FootballWrapAngle(turn);
                    facing = turn + facing;
                }
            }
            else if (role == SOCCER_ROLE_FORWARD)
            {
                target.Fill(0.0f);
                bool toMate = member < 5 && DAT_10db6ee8[slot].matrix._42 < DAT_10db6ee8[0].matrix._42 &&
                              DAT_10db6ee8[0].matrix._42 < DAT_10db6ee8[slot].matrix._42 + 800.0f &&
                              DAT_10db6ee8[0].matrix._42 < (float)DAT_10211000[2].y;
                if (toMate)
                {
                    velocity.z *= 1.5f;
                    float length = velocity.Length2D();
                    if (length < 0.5f)
                        length = 1.0f;
                    if (length < 17.0f)
                    {
                        velocity.x = velocity.x / length * 17.0f;
                        velocity.y = velocity.y / length * 17.0f;
                    }
                    DAT_1020c2f8 = 0xf;
                    turn = atan2f(target.x - DAT_10e40204._41, target.y - DAT_10e40204._42) - facing;
                    FootballWrapAngle(turn);
                    DAT_1020c310 = 0x1e;
                    DAT_1020c30c = 1;
                    facing = turn * 0.9f + facing;
                    if (4 < member)
                        DAT_1020c30c = 2;
                    if (DAT_1020b430[slot].soccer.delayB == 0)
                    {
                        DAT_1020b430[slot].soccer.delayB = FB_VOICE_COOLDOWN;
                        if (slot == FB_SLOT_A)
                            FootballSayEither(slot, FB_PASS_RECEIVE, FB_PASS_PASS_B);
                        else if (slot == FB_SLOT_B)
                            FootballSayEither(slot, FB_SHOT_PUT_IN_GOAL, FB_PASS_PASS_D);
                    }
                }
                else
                {
                    target.y = (float)(member > 4 ? DAT_10211000[0].code : DAT_10211000[3].code);
                    if ((DAT_10e40204.GetTranslation() - target).Length2D() < 900.0f &&
                        ((target.y < 0.0f && target.y < DAT_10e40204._42) || (0.0f < target.y && DAT_10e40204._42 < target.y)))
                    {
                        shot = true;
                        float length = velocity.Length2D();
                        if (length < 0.5f)
                            length = 1.0f;
                        DAT_1020c2f8 = 0x14;
                        velocity.x = velocity.x / length * 45.0f;
                        velocity.y = velocity.y / length * 45.0f;
                        velocity.z *= 0.35f;
                        turn = atan2f(target.x - DAT_10e40204._41, target.y - DAT_10e40204._42) - facing;
                        FootballWrapAngle(turn);
                        facing = turn * 0.95f + facing;
                        if (DAT_1020b430[slot].soccer.delayB == 0)
                        {
                            DAT_1020b430[slot].soccer.delayB = FB_VOICE_COOLDOWN;
                            FootballSayShot(slot);
                        }
                    }
                    else
                    {
                        bool corner;
                        if (member < 5 || (float)DAT_10211000[0].y <= DAT_10e40204._42)
                        {
                            corner = member < 5 && (float)DAT_10211000[3].y < DAT_10e40204._42 &&
                                     floorf(DAT_10e40204._41) < (float)DAT_10211000[1].x;
                        }
                        else
                        {
                            corner = floorf(DAT_10e40204._41) < (float)DAT_10211000[1].x;
                        }
                        if (corner)
                        {
                            target.Copy(DAT_10e40204.GetTranslation());
                            if (0.0f <= DAT_10e40204._41)
                                target.x = (float)DAT_10211000[1].x + 50.0f;
                            else
                                target.x = (float)DAT_10211000[0].x - 50.0f;
                        }
                        turn = atan2f(target.x - DAT_10e40204._41, target.y - DAT_10e40204._42) - facing;
                        FootballWrapAngle(turn);
                        facing = turn + facing;
                    }
                }
            }
        }
        facing = (float)(FUN_1013f880() % 8000 - 4000) / 100000.0f + facing;
        velocity.RotateZ(-facing);
        if (1.0f < velocity.Length())
        {
            float slope = acosf(velocity.Length2D() / velocity.Length());
            if (1.0472f < floorf(slope))
            {
                int lowering = 0;
                if (velocity.z < 0.0f)
                    lowering = -1;
                float heading = atan2f(velocity.x, velocity.y) + FB_PI;
                velocity.y = -velocity.Length();
                velocity.x = 0.0f;
                velocity.z = 0.0f;
                velocity.RotateX(1.0472f);
                velocity.RotateZ(-heading);
                if (lowering != 0)
                    velocity.z *= -1.0f;
            }
        }
        float climb = velocity.z;
        if (floorf(velocity.z) < 7.5f)
        {
            velocity.z = -7.5f;
            if (0.0f <= climb)
                velocity.z = 7.5f;
        }
        if (member < 5)
        {
            if (DAT_10c635f8 == 0)
            {
                velocity.x *= 1.2f;
                velocity.y *= 1.2f;
            }
            else if (DAT_10c635f8 == 2)
            {
                velocity.x *= 1.3f;
                velocity.y *= 1.3f;
            }
            else
            {
                velocity.x *= 1.1f;
                velocity.y *= 1.1f;
            }
        }
        else if (DAT_10c635f8 == 0)
        {
            velocity.x *= 1.4f;
            velocity.y *= 1.4f;
        }
        else if (DAT_10c635f8 != 2)
        {
            velocity.x *= 1.2f;
            velocity.y *= 1.2f;
        }
        else
        {
            velocity.x *= 1.6f;
            velocity.y *= 1.6f;
        }
        float carried = DAT_10803ab8[0].Length() * 0.75f;
        float blend = velocity.Length() + carried;
        if (1.0f <= blend)
        {
            float shotHeading = atan2f(velocity.x, velocity.y);
            float aimHeading = atan2f(DAT_10803ab8[0].x, DAT_10803ab8[0].y);
            float share = DAT_10803ab8[0].Length() * (shot ? 0.25f : 0.5f);
            shotHeading = (aimHeading - shotHeading) * (share / blend) + shotHeading;
            DAT_10803ab8[0].Copy(DAT_10803ab8[0] * 0.15f + velocity * 0.85f);
            DAT_10803ab8[0].y = -DAT_10803ab8[0].Length2D();
            DAT_10803ab8[0].x = 0.0f;
            DAT_10803ab8[0].RotateZ(FB_PI - shotHeading);
        }
        else
        {
            DAT_10803ab8[0].Copy(DAT_10803ab8[0] * 0.15f + velocity * 0.85f);
        }
        DAT_1020c2fc += (int)velocity.z;
        if (DAT_1020c2fc < FB_CPU_SPIN_LIMIT + 1)
        {
            if (DAT_1020c2fc < -FB_CPU_SPIN_LIMIT)
                DAT_1020c2fc = -FB_CPU_SPIN_LIMIT;
        }
        else
        {
            DAT_1020c2fc = FB_CPU_SPIN_LIMIT;
        }
        if (velocity.Length() < DAT_10803ab8[0].Length())
        {
            DAT_10803ab8[0].Normalized3();
            DAT_10803ab8[0].Copy(DAT_10803ab8[0] * velocity.Length());
        }
        FUN_10065b10(FOOTBALL_BALL_SLOT, FB_SOUND_KICK, 0, 0);
        break;
    }

    ballPos.Copy(DAT_10e40204.GetTranslation());
    bool inFirstNet = ballPos.y < (float)DAT_10211000[1].y && (float)DAT_10211000[0].y < ballPos.y &&
                      ballPos.x < (float)DAT_10211000[1].x && (float)DAT_10211000[0].x < ballPos.x &&
                      ballPos.z < (float)DAT_10211000[1].z && (float)DAT_10211000[0].z < ballPos.z;
    bool inSecondNet = ballPos.y < (float)DAT_10211000[3].y && (float)DAT_10211000[2].y < ballPos.y &&
                       ballPos.x < (float)DAT_10211000[3].x && (float)DAT_10211000[2].x < ballPos.x &&
                       ballPos.z < (float)DAT_10211000[3].z && (float)DAT_10211000[2].z < ballPos.z;
    if (inFirstNet || inSecondNet)
    {
        DAT_1020c2d0++;
        if (DAT_1020c2d0 < FB_GOAL_FRAMES)
            return 0;
        if (inFirstNet)
        {
            DAT_1020c324++;
            DAT_1020c2e4 = 0xf;
            DAT_10c65330 = DAT_1020c324;
        }
        else
        {
            DAT_1020c32c++;
            DAT_1020c2ec = 0xf;
            DAT_10c65060 = DAT_1020c32c;
        }
        DAT_1020c2d0 = FB_CELEBRATION_FRAMES;
        DAT_10803ab8[0].Copy(DAT_10803ab8[0] * 0.3f);
        DAT_1020c2f8 = 0;
        DAT_1020c308 = 0x78;
        return 0;
    }
    if (DAT_1020c2d0 < 0)
        DAT_1020c2d0++;
    else
        DAT_1020c2d0 = 0;
    return 0;
}
