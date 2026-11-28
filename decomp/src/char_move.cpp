// Character movement vectors. The character records are 0x249c bytes; this file views them from their model part
// (0x10db6ee8, 0x10 bytes into each Character of world.h), where all the fields used here fall inside one record.

#include "last_position.h"
#include <math.h>
#include <string.h>
#include "vec4.h"

#include "matrix4.h"

#include "anim_model.h"

#include "node.h"
#include "map_vars.h"

typedef char check_charrecord[sizeof(AnimModel) == 0x249c ? 1 : -1];

// 0x100241e0 (Character_SetMove). Sets the movement vector of character `index` from (x, y, z): half of it in the
// state where m_2498 is 1 and m_238c is not 1, otherwise 0.7 of it, doubled to 1.4 while the character is nearly
// standing still (speed vector shorter than 1). Called by the enemy and player movement code. Returns 0.
extern "C" int FUN_100241e0(int index, float x, float y, float z)
{
    if (DAT_10db6ee8[index].m_238c != 1 && DAT_10db6ee8[index].mode == 1)
    {
        DAT_10db6ee8[index].move.x = x * 0.5f;
        DAT_10db6ee8[index].move.y = y * 0.5f;
        DAT_10db6ee8[index].move.z = z * 0.5f;
        return 0;
    }
    DAT_10db6ee8[index].move.x = x * 0.7f;
    DAT_10db6ee8[index].move.y = y * 0.7f;
    DAT_10db6ee8[index].move.z = z * 0.7f;
    if (DAT_10db6ee8[index].velocity.LengthSquared3() < 1.0f)
    {
        DAT_10db6ee8[index].move.x = x * 1.4f;
        DAT_10db6ee8[index].move.y = y * 1.4f;
        DAT_10db6ee8[index].move.z = z * 1.4f;
    }
    return 0;
}

#include "char_state.h"

#include "char_tables.h"
#include "char_vars.h"
#include "world.h"
extern "C" int FUN_1013f880(void);                                          // the game's random number
extern "C" int FUN_10065b10(int index, int sound, int always, int optional);  // Character_PlaySound
extern "C" int FUN_1000d620(int index, int key, int frames);                // KeyPrompt_Show
extern "C" int FUN_1001b670(int index, float range, int count, ...);       // Enemies_WakeNearby
extern "C" int FUN_10043cc0(int index);                                     // Char_ResetState

// atan2 on floats (an inline helper: both arguments are rounded to float first, the second one first).
inline float Atan2(float y, float x)
{
    return (float)atan2(y, x);
}

// 0x100b58a0 (Npc_TalkWhenNear). Idles NPC `index` like Npc_Idle (voice sounds now and then, with talk prompt line - 1,
// or line + 1979 for lines past 20) and starts talking once the player comes closer than the talk id's hundreds (in
// units, or at once when DAT_10210b30 is set; `wakeEnemies` also wakes the enemies within 800): it turns to the
// player, speeds up its motion, plays voice 0x20c, shows prompt 9 (line 10) or 15 and its state is reset.
// Returns 0.
extern "C" int FUN_100b58a0(int index, int wakeEnemies)
{
    float distance = (DAT_10db6ee8[0].matrix.GetTranslation() - DAT_10db6ee8[index].matrix.GetTranslation()).Length2D();
    if (DAT_10210b30[index] != 0 || distance < (float)(DAT_1020b430[index].talk / 100 * 100))
    {
        DAT_10210b30[index] = 0;
        if (wakeEnemies != 0)
            FUN_1001b670(index, 800.0f, 0);
        DAT_1020b430[index].sawPlayer = 1;
    }
    if (DAT_1020b430[index].state == 0)
    {
        DAT_1020b430[index].state = 1;
        return 0;
    }
    if (DAT_1020b430[index].state == 1)
    {
        DAT_10db6ee8[index].PlayMotion(2, 0, 5, 1.0f);
        if (DAT_1020b430[index].wait > 0)
            DAT_1020b430[index].wait--;
        else if (DAT_1020b430[index].talk % 100 != 0 && FUN_1013f880() % 10000 < 150)
        {
            FUN_10065b10(index, FUN_1013f880() % 3 + 0x209, 0, 0);
            int line = DAT_1020b430[index].talk % 100;
            if (line > 20)
                FUN_1000d620(index, line + 1979, 60);
            else
                FUN_1000d620(index, line - 1, 60);
            DAT_1020b430[index].wait = 75;
        }
        if (DAT_1020b430[index].sawPlayer != 0)
        {
            DAT_10db6ee8[index].targetHeading =
                Atan2(DAT_10db6ee8[0].matrix.m[12] - DAT_10db6ee8[index].matrix.m[12],
                      DAT_10db6ee8[0].matrix.m[13] - DAT_10db6ee8[index].matrix.m[13]) + 3.1415f;
            DAT_10db6ee8[index].AddMotionSpeed(20.0f);
            FUN_10065b10(index, 0x20c, 0, 0);
            if (DAT_1020b430[index].talk % 100 == 10)
                FUN_1000d620(index, 9, 60);
            else
                FUN_1000d620(index, 15, 60);
            FUN_10043cc0(index);
        }
    }
    return 0;
}

// 0x100b5ae0 (Npc_Idle). Keeps NPC `index` standing still in idle motion 2. Now and then (a 3 % chance per frame,
// then at least 75 frames apart) an NPC with a talk line plays one of the voice sounds 0x209..0x20b and shows its
// talk prompt (key script * 1000 + line - 1) for 60 frames. Returns 0.
extern "C" int FUN_100b5ae0(int index)
{
    if (DAT_10210b30[index] != 0 || DAT_1020b430[index].sawPlayer != 0)
    {
        DAT_10210b30[index] = 0;
        DAT_1020b430[index].sawPlayer = 0;
    }
    if (DAT_1020b430[index].state == 0)
    {
        DAT_1020b430[index].state = 1;
        return 0;
    }
    if (DAT_1020b430[index].state == 1)
    {
        DAT_10db6ee8[index].PlayMotion(2, 0, 5, 1.0f);
        DAT_10db6ee8[index].m_0267 = 1;
        DAT_10db6ee8[index].m_23d8.Fill(0);
        DAT_10db6ee8[index].move.Fill(0);
        DAT_10db6ee8[index].velocity.Fill(0);
        DAT_10255a88[index] = 0;
        if (DAT_1020b430[index].wait > 0)
        {
            DAT_1020b430[index].wait--;
            return 0;
        }
        if (DAT_1020b430[index].talk % 100 != 0 && FUN_1013f880() % 10000 < 300)
        {
            FUN_10065b10(index, FUN_1013f880() % 3 + 0x209, 0, 0);
            FUN_1000d620(index, DAT_1020b430[index].talk / 100 * 1000 - 1 + DAT_1020b430[index].talk % 100, 60);
            DAT_1020b430[index].wait = 75;
        }
    }
    return 0;
}

// 0x10045c30 (Npc_WaitFacingAway). A character that waits with its back to the player: on the first frame it starts
// idle motion 2 and takes its reaction distance from the talk id (line * 10). Then it keeps turning so that it
// faces away from the player (idle motion 2 once it is within about one degree, else motion 0). When the player
// comes closer than that distance (or the character was told to react, m_0038, and has a script number) its state
// is reset to behaviour 0x1d with talk id 999 and it starts walking away (16 units along its facing). Returns 0.
extern "C" int FUN_10045c30(int index)
{
    float distance = (DAT_10db6ee8[0].matrix.GetTranslation() - DAT_10db6ee8[index].matrix.GetTranslation()).Length2D();
    CharState *state = &DAT_1020b430[index];
    if (DAT_1020b430[index].state == 0)
    {
        DAT_1020b430[index].state = 1;
        DAT_10db6ee8[index].mode = 0;
        DAT_1020b430[index].range = DAT_1020b430[index].talk % 100 * 10;
        DAT_10db6ee8[index].PlayMotion(2, 0, 5, 1.0f);
        return 0;
    }
    {
        float turn = DAT_10db6ee8[index].heading - DAT_10db6ee8[index].targetHeading;
        while (turn > 3.1415f)
            turn -= 6.283f;
        while (turn < -3.1415f)
            turn += 6.283f;
        float squared = turn * turn;
        if (squared < 0.000304503f)
            DAT_10db6ee8[index].PlayMotion(2, 0, 5, 1.0f);
        else
            DAT_10db6ee8[index].PlayMotion(0, 0, 5, 1.0f);
    }
    DAT_10db6ee8[index].targetHeading =
        Atan2(DAT_10db6ee8[0].matrix.m[12] - DAT_10db6ee8[index].matrix.m[12],
              DAT_10db6ee8[0].matrix.m[13] - DAT_10db6ee8[index].matrix.m[13]) + 3.1415f;
    if (distance < (float)DAT_1020b430[index].range ||
        (DAT_1020b430[index].sawPlayer != 0 && DAT_1020b430[index].talk / 100 != 0))
    {
        memset(state, 0, sizeof(CharState));
        DAT_1020b430[index].state = 2;
        DAT_1020b430[index].wait = 0x1d;
        DAT_1020b430[index].talk = 999;
        DAT_1020b430[index].behaviour = 0xf;
        DAT_10210b30[index] = 0;
        DAT_10db6ee8[index].mode = 1;
        Vec4 step;
        step.y = -16.0f;
        step.RotateZ(-DAT_10db6ee8[index].heading);
        FUN_100241e0(index, step.x, step.y, 0);
        DAT_10db6ee8[index].velocity.Copy(step);
        DAT_10db6ee8[index].AddMotionSpeed(20.0f);
    }
    return 0;
}

extern "C" int FUN_10043e20(const Vec4 *target, const Vec4 *eye, float heading, float range, float angleRange,
                            float nearRange, int checkWalls);           // Char_CanSee

// 0x100b1520 (Npc_WatchForPlayer). A guard-like NPC that idles (motion 2) until it sees the player: from its head
// (75 above its origin) towards the player's head (50 above), within its talk id's line * 10 units, 63 degrees either
// side of its facing or 250 units. When it has seen the player (remembered in m_0038) or was told to react, it
// shows prompt 15 for 60 frames, plays voice 0x20c, wakes the enemies within 1500 and starts turning: from then on
// it faces the player (idle motion 2 once within one degree, else motion 0) and after 30 frames its state is reset.
// Returns 0.
extern "C" int FUN_100b1520(int index)
{
    Vec4 player;
    Vec4 self;
    player.Copy(DAT_10db6ee8[0].matrix.GetTranslation());
    self.Copy(DAT_10db6ee8[index].matrix.GetTranslation());
    (player - self).Length();      // computed and not used
    (player - self).Length2D();    // computed and not used
    if (DAT_1020b430[index].state == 0)
    {
        DAT_10db6ee8[index].PlayMotion(2, 0, 5, 1.0f);
        DAT_1020b430[index].state = 1;
    }
    else if (DAT_1020b430[index].state == 1)
    {
        DAT_10db6ee8[index].PlayMotion(2, 0, 5, 1.0f);
        player.z += 50.0f;
        self.z += 75.0f;
        if ((DAT_1020b430[index].sawPlayer |= FUN_10043e20(&self, &player, DAT_10db6ee8[index].heading,
                                                         (float)(DAT_1020b430[index].talk * 10), 1.099525f, 250.0f, 1)) != 0 ||
            DAT_10210b30[index] != 0)
        {
            FUN_1000d620(index, 15, 60);
            FUN_10065b10(index, 0x20c, 0, 0);
            DAT_1020b430[index].state = 2;
            DAT_1020b430[index].wait = 0;
            FUN_1001b670(index, 1500.0f, 0);
        }
    }
    else if (DAT_1020b430[index].state == 2)
    {
        float target = Atan2(player.x - self.x, player.y - self.y) + 3.1415f;
        DAT_1020b430[index].wait++;
        DAT_10db6ee8[index].targetHeading = target;
        float turn = DAT_10db6ee8[index].heading - target;
        while (turn > 3.1415f)
            turn -= 6.283f;
        while (turn < -3.1415f)
            turn += 6.283f;
        if (turn < 0.01745f)
            DAT_10db6ee8[index].PlayMotion(2, 0, 5, 1.0f);
        else
            DAT_10db6ee8[index].PlayMotion(0, 0, 5, 1.0f);
        if (DAT_1020b430[index].wait == 0x1e)
            FUN_10043cc0(index);
    }
    DAT_10210b30[index] = 0;
    return 0;
}

#include "effect_manager.h"

#include "char_slot.h"


// 0x100b5c30 (Npc_TurnWhenSeen). Npc_AlertWhenSeen below with one step more: when it reacts, the character also
// turns at once to face the player (heading and target heading). Returns 0.
int FUN_100b5c30(int index)
{
    Vec4 player;
    Vec4 self;
    player.Copy(DAT_10db6ee8[0].matrix.GetTranslation());
    self.Copy(DAT_10db6ee8[index].matrix.GetTranslation());
    float distance = (player - self).Length2D();
    DAT_10daaae8[index].m_0141 = 1;
    int visible = 0;
    if (DAT_104c0380 % 10 == index % 10)
    {
        Vec4 from;
        Vec4 to;
        Vec4 hit;
        from.Copy(self);
        from.z += 50.0f;
        to.Copy(player);
        to.z += 50.0f;
        if (DAT_10255f08 == 0 || DAT_10255f08->Collide(0, 4, &from, &to, &hit) == 0)
            visible = 1;
    }
    if (DAT_1020b430[index].state == 0)
    {
        DAT_10db6ee8[index].SetPosition(self.x, self.y, self.z);
        DAT_10db6ee8[index].alpha = 0;
        DAT_10db6ee8[index].mode = 0;
        DAT_1020b430[index].state = 1;
    }
    else if (DAT_1020b430[index].state == 1)
    {
        DAT_10db6ee8[index].alpha = 0;
        if (distance < (float)(DAT_1020b430[index].talk % 100 * 10) && visible != 0)
        {
            DAT_102199b8[index] = 2;
            float target = atan2f(player.x - self.x, player.y - self.y) + 3.1415f;
            DAT_10db6ee8[index].targetHeading = target;
            DAT_10db6ee8[index].heading = target;
            if ((DAT_1020b430[index].talk / 100 & 2) == 0)
                FUN_10065b10(index, 0x2d0, 0, 0);
            if ((DAT_1020b430[index].talk / 100 & 1) == 0)
            {
                Matrix4 matrix;
                Vec4 position;
                matrix.SetIdentity();
                matrix.SetTranslation(DAT_10db6ee8[index].matrix.GetTranslation());
                DAT_10daaa80.Start(0x3f4, position, -1, -1, 0, 0, 0, &matrix, 0, 0);
            }
            FUN_10043cc0(index);
        }
    }
    return 0;
}

// 0x100b5f80 (Npc_AlertWhenSeen). Places character `index` where it stands on its first frame; afterwards, once
// every ten frames (the frame counter's units equal the character's), it checks whether nothing blocks the line from
// its head to the player's head (50 above each origin). When the player is that visible and nearer than the talk
// id's line * 10 units, it sets its group timer to 2, plays voice 0x2d0 (unless the talk id's hundreds have bit 2),
// starts effect 0x3f4 at its position (unless they have bit 1) and its state is reset. Returns 0.
int FUN_100b5f80(int index)
{
    Vec4 player;
    Vec4 self;
    player.Copy(DAT_10db6ee8[0].matrix.GetTranslation());
    self.Copy(DAT_10db6ee8[index].matrix.GetTranslation());
    float distance = (player - self).Length2D();
    DAT_10daaae8[index].m_0141 = 1;
    int visible = 0;
    if (DAT_104c0380 % 10 == index % 10)
    {
        Vec4 from;
        Vec4 to;
        Vec4 hit;
        from.Copy(self);
        from.z += 50.0f;
        to.Copy(player);
        to.z += 50.0f;
        if (DAT_10255f08 == 0 || DAT_10255f08->Collide(0, 4, &from, &to, &hit) == 0)
            visible = 1;
    }
    if (DAT_1020b430[index].state == 0)
    {
        DAT_10db6ee8[index].SetPosition(self.x, self.y, self.z);
        DAT_10db6ee8[index].alpha = 0;
        DAT_10db6ee8[index].mode = 0;
        DAT_1020b430[index].state = 1;
    }
    else if (DAT_1020b430[index].state == 1)
    {
        DAT_10db6ee8[index].alpha = 0;
        if (distance < (float)(DAT_1020b430[index].talk % 100 * 10) && visible != 0)
        {
            DAT_102199b8[index] = 2;
            if ((DAT_1020b430[index].talk / 100 & 2) == 0)
                FUN_10065b10(index, 0x2d0, 0, 0);
            if ((DAT_1020b430[index].talk / 100 & 1) == 0)
            {
                Matrix4 matrix;
                Vec4 position;
                matrix.SetIdentity();
                matrix.SetTranslation(DAT_10db6ee8[index].matrix.GetTranslation());
                DAT_10daaa80.Start(0x3f4, position, -1, -1, 0, 0, 0, &matrix, 0, 0);
            }
            FUN_10043cc0(index);
        }
    }
    return 0;
}

extern "C" int FUN_1003ad20(int sound, const Vec4 *position, float volume);  // Sound_PlayEffect

// 0x100b5550 (Npc_GreetWhenNear). An idling NPC that greets: it plays voice 0x2a3 (speeding up its motion) now and
// then while m_238c is set, and voice 0x209..0x20b with its talk prompt (line - 1, or line + 1979 past 20) every
// 75 frames or more. While it has not reacted yet it reacts when told to (DAT_10210b30), when it already saw the
// player (m_0038) or when the player comes within the talk id's hundreds * 100 units: it wakes the enemies within
// 800, plays effect 0x20c at its position (when within 2500 of the camera target, or always for characters below
// 0x3a), shows prompt 15 for 60 frames and turns to face the player for 30 frames before its state is reset.
// Returns 0.
extern "C" int FUN_100b5550(int index)
{
    Vec4 player;
    Vec4 self;
    player.Copy(DAT_10db6ee8[0].matrix.GetTranslation());
    self.Copy(DAT_10db6ee8[index].matrix.GetTranslation());
    float distanceSquared = (player - self).LengthSquared3();
    if (DAT_1020b430[index].state < 2 &&
        (DAT_10210b30[index] != 0 || DAT_1020b430[index].sawPlayer != 0 ||
         distanceSquared < (float)(DAT_1020b430[index].talk / 100 * (DAT_1020b430[index].talk / 100) * 10000)))
    {
        FUN_1001b670(index, 800.0f, 0);
        DAT_10210b30[index] = 0;
        DAT_1020b430[index].state = 2;
        DAT_1020b430[index].wait = 0;
        float cameraSquared = (DAT_10db6ee8[index].matrix.GetTranslation() - DAT_118e6cb8).LengthSquared3();
        if (cameraSquared < 6250000.0f || index < 0x3a)
        {
            Vec4 position;
            position.Set(DAT_10db6ee8[index].matrix.m[12], DAT_10db6ee8[index].matrix.m[13],
                         DAT_10db6ee8[index].matrix.m[14], 0);
            FUN_1003ad20(0x20c, &position, 1.0f);
        }
        if (DAT_104bf8f8[index] == 0 || DAT_1021d138[index] != 15)
        {
            DAT_104bf8f8[index] = 1;
            DAT_1021d138[index] = 15;
        }
        DAT_1021f740[index] = 60;
    }
    if (DAT_1020b430[index].state == 0)
    {
        DAT_1020b430[index].state = 1;
        DAT_10db6ee8[index].PlayMotion(2, 0, 5, 1.0f);
        return 0;
    }
    if (DAT_1020b430[index].state == 1)
    {
        if (DAT_10db6ee8[index].m_238c != 0 && FUN_1013f880() % 10000 < 100)
        {
            FUN_10065b10(index, 0x2a3, 0, 0);
            DAT_10db6ee8[index].AddMotionSpeed(24.0f);
        }
        if (DAT_1020b430[index].talk % 100 > 0 && DAT_1020b430[index].wait == 0 && FUN_1013f880() % 10000 < 200)
        {
            FUN_10065b10(index, FUN_1013f880() % 3 + 0x209, 0, 0);
            int line = DAT_1020b430[index].talk % 100;
            if (line > 20)
                FUN_1000d620(index, line + 1979, 60);
            else
                FUN_1000d620(index, line - 1, 60);
            DAT_1020b430[index].wait = 75;
        }
        if (DAT_1020b430[index].wait > 0)
        {
            DAT_1020b430[index].wait--;
            return 0;
        }
    }
    else if (DAT_1020b430[index].state == 2)
    {
        DAT_10db6ee8[index].targetHeading = Atan2(player.x - self.x, player.y - self.y) + 3.1415f;
        if (++DAT_1020b430[index].wait > 30)
            FUN_10043cc0(index);
    }
    return 0;
}

// 0x100b3430 (Npc_RiseWhenSeen). A character that waits 400 units below the ground where it was placed. Once every ten
// frames it checks the line from 350 below its origin to the player's head (50 above the player's origin). The talk
// id's units say what happens when that line is clear: 0 = within 800 units it rises (state 2: m_0d4c grows to 1
// over 15 frames, m_2498 set, voice 0x208) and its state is reset; 1 or 2 = within 1300 units it moves back up and
// starts again with the talk id's remaining digits and m_003c set to 5 or 6. Returns 0.
extern "C" int FUN_100b3430(int index)
{
    Vec4 player;
    Vec4 self;
    player.Copy(DAT_10db6ee8[0].matrix.GetTranslation());
    self.Copy(DAT_10db6ee8[index].matrix.GetTranslation());
    float distance = (player - self).Length2D();
    DAT_10daaae8[index].m_0141 = 1;
    int visible = 0;
    if (DAT_104c0380 % 10 == index % 10)
    {
        Vec4 from;
        Vec4 to;
        Vec4 hit;
        from.Copy(self);
        from.z -= 350.0f;
        to.Copy(player);
        to.z += 50.0f;
        if (DAT_10255f08 == 0 || DAT_10255f08->Collide(0, 4, &from, &to, &hit) == 0)
            visible = 1;
    }
    if (DAT_1020b430[index].state == 0)
    {
        DAT_10db6ee8[index].SetPosition(self.x, self.y, self.z + 400.0f);
        DAT_10db6ee8[index].alpha = 0;
        DAT_10db6ee8[index].mode = 0;
        DAT_1020b430[index].state = 1;
        return 0;
    }
    if (DAT_1020b430[index].state == 1)
    {
        DAT_10db6ee8[index].alpha = 0;
        if (DAT_1020b430[index].talk % 10 == 0)
        {
            if (distance < 800.0f && visible != 0)
            {
                DAT_1020b430[index].state = 2;
                DAT_1020b430[index].wait = 0;
                DAT_10db6ee8[index].mode = 1;
                FUN_10065b10(index, 0x208, 0, 0);
                return 0;
            }
        }
        else if (DAT_1020b430[index].talk % 10 == 1)
        {
            if (distance < 1300.0f && visible != 0)
            {
                DAT_1020b430[index].talk /= 10;
                DAT_1020b430[index].behaviour = 5;
                DAT_1020b430[index].state = 0;
                DAT_1020b430[index].wait = 0;
                DAT_10db6ee8[index].SetPosition(self.x, self.y, self.z - 400.0f);
                return 0;
            }
        }
        else if (DAT_1020b430[index].talk % 10 == 2)
        {
            if (distance < 1300.0f && visible != 0)
            {
                DAT_1020b430[index].talk /= 10;
                DAT_1020b430[index].behaviour = 6;
                DAT_1020b430[index].state = 0;
                DAT_1020b430[index].wait = 0;
                DAT_10db6ee8[index].SetPosition(self.x, self.y, self.z - 400.0f);
                return 0;
            }
        }
    }
    else if (DAT_1020b430[index].state == 2)
    {
        DAT_1020b430[index].wait++;
        DAT_10db6ee8[index].alpha = (float)DAT_1020b430[index].wait / 15.0f;
        if (DAT_1020b430[index].wait == 15)
        {
            DAT_10db6ee8[index].alpha = 1.0f;
            FUN_10043cc0(index);
            DAT_10daaae8[index].m_0141 = 0;
        }
    }
    return 0;
}

// 0x100b1ff0 (Npc_FaceAndTalk). An NPC that keeps facing the player (motion 2 once within one degree, else motion
// 0) and now and then (when its talk line is set and no prompt is showing) shows talk prompt line - 1 for 60
// frames, then waits 120 frames. Once it saw the player (m_0038) or is told to react (DAT_10210b30), it shows
// prompt 15, plays voice 0x20c, wakes the enemies within 1500 and turns to the player for 30 frames before its state
// is reset. Returns 0.
extern "C" int FUN_100b1ff0(int index)
{
    Vec4 player;
    Vec4 self;
    player.Copy(DAT_10db6ee8[0].matrix.GetTranslation());
    self.Copy(DAT_10db6ee8[index].matrix.GetTranslation());
    (player - self).Length();      // computed and not used
    (player - self).Length2D();    // computed and not used
    if (DAT_1020b430[index].state == 0)
    {
        DAT_10db6ee8[index].PlayMotion(2, 0, 5, 1.0f);
        DAT_1020b430[index].state = 1;
    }
    else if (DAT_1020b430[index].state == 1)
    {
        if (DAT_1020b430[index].sawPlayer != 0 || DAT_10210b30[index] != 0)
        {
            FUN_1000d620(index, 15, 60);
            FUN_10065b10(index, 0x20c, 0, 0);
            DAT_1020b430[index].state = 2;
            DAT_1020b430[index].wait = 0;
            FUN_1001b670(index, 1500.0f, 0);
        }
        float target = Atan2(player.x - self.x, player.y - self.y) + 3.1415f;
        DAT_10db6ee8[index].targetHeading = target;
        float turn = DAT_10db6ee8[index].heading - target;
        while (turn > 3.1415f)
            turn -= 6.283f;
        while (turn < -3.1415f)
            turn += 6.283f;
        float squared = turn * turn;
        if (squared < 0.000304503f)
            DAT_10db6ee8[index].PlayMotion(2, 0, 5, 1.0f);
        else
            DAT_10db6ee8[index].PlayMotion(0, 0, 5, 1.0f);
        if (DAT_1020b430[index].talk > 0 && DAT_1020b430[index].range == 0 && FUN_1013f880() % 10000 < 50)
        {
            DAT_1020b430[index].range = 120;
            FUN_1000d620(index, DAT_1020b430[index].talk - 1, 60);
        }
        if (DAT_1020b430[index].range > 0)
        {
            DAT_1020b430[index].range--;
            DAT_10210b30[index] = 0;
            return 0;
        }
    }
    else if (DAT_1020b430[index].state == 2)
    {
        float target = Atan2(player.x - self.x, player.y - self.y) + 3.1415f;
        DAT_10db6ee8[index].targetHeading = target;
        float turn = DAT_10db6ee8[index].heading - target;
        DAT_1020b430[index].wait++;
        while (turn > 3.1415f)
            turn -= 6.283f;
        while (turn < -3.1415f)
            turn += 6.283f;
        if (turn < 0.01745f)
            DAT_10db6ee8[index].PlayMotion(2, 0, 5, 1.0f);
        else
            DAT_10db6ee8[index].PlayMotion(0, 0, 5, 1.0f);
        if (DAT_1020b430[index].wait == 0x1e)
            FUN_10043cc0(index);
    }
    DAT_10210b30[index] = 0;
    return 0;
}


// 0x100b6290 (Npc_RunToMarker). A messenger: on its first frame it claims the nearest free marker among characters
// 0x5c..0x7b (kind ',' becomes 'Q'; with none it restarts with m_003c 1). It idles (motion 2) until it sees the
// player (Char_CanSee as in Npc_WatchForPlayer) or is told to react: then it shows prompt 15, plays voice 0x20c,
// wakes the enemies within 1500, turns to the player and speeds up its motion. After 10 frames it runs (motion 1)
// towards the marker at 24 units a frame; within 150 units it is placed 75 units short of the marker, frees it and
// starts over with m_003c 0x1f. A marker that was moved away (x below -20000) resets its state. Returns 0.
extern "C" int FUN_100b6290(int index)
{
    Vec4 player;
    Vec4 self;
    player.Copy(DAT_10db6ee8[0].matrix.GetTranslation());
    self.Copy(DAT_10db6ee8[index].matrix.GetTranslation());
    if (DAT_1020b430[index].state == 0)
    {
        DAT_10db6ee8[index].PlayMotion(2, 0, 5, 1.0f);
        DAT_1020b430[index].state = 1;
        Vec4 marker;
        float nearestDistance = 99999.0f;
        int nearest = -1;
        for (int i = 0x5c; i < 0x7c; i++)
        {
            if (DAT_1021f3c0[i] == ',')
            {
                marker.Copy(DAT_10db6ee8[i].matrix.GetTranslation());
                if (nearest == -1)
                {
                    nearest = i;
                    nearestDistance = (self - marker).Length2D();
                }
                else if ((self - marker).Length2D() < nearestDistance)
                {
                    nearest = i;
                    nearestDistance = (self - marker).Length2D();
                }
            }
        }
        if (nearest != -1)
        {
            DAT_1021f3c0[nearest] = 'Q';
            DAT_1020b430[index].action = nearest;
        }
        else
        {
            DAT_1020b430[index].state = 0;
            DAT_1020b430[index].behaviour = 1;
        }
    }
    else if (DAT_1020b430[index].state == 1)
    {
        DAT_10db6ee8[index].PlayMotion(2, 0, 5, 1.0f);
        player.z += 50.0f;
        self.z += 75.0f;
        if ((DAT_1020b430[index].sawPlayer |= FUN_10043e20(&self, &player, DAT_10db6ee8[index].heading,
                                                         (float)(DAT_1020b430[index].talk * 10), 1.099525f, 250.0f, 1)) != 0 ||
            DAT_10210b30[index] != 0)
        {
            FUN_1000d620(index, 15, 60);
            FUN_10065b10(index, 0x20c, 0, 0);
            DAT_1020b430[index].state = 2;
            DAT_1020b430[index].wait = 0;
            FUN_1001b670(index, 1500.0f, 0);
            DAT_10db6ee8[index].targetHeading = Atan2(player.x - self.x, player.y - self.y) + 3.1415f;
            DAT_10db6ee8[index].AddMotionSpeed(20.0f);
        }
    }
    else if (DAT_1020b430[index].state == 2)
    {
        if (++DAT_1020b430[index].wait > 10)
        {
            Vec4 marker;
            Vec4 step;
            marker.Copy(DAT_10db6ee8[DAT_1020b430[index].action].matrix.GetTranslation());
            step.Copy(marker - self);
            step.Normalized3();
            step.Copy(step * 24.0f);
            DAT_10db6ee8[index].PlayMotion(1, 0, 5, 1.0f);
            DAT_10db6ee8[index].targetHeading = atan2f(step.x, step.y) + 3.1415f;
            if ((self - marker).Length2D() < 150.0f)
            {
                step.Copy(self - marker);
                step.Normalized3();
                step.Copy(step * 75.0f);
                step.Copy(step + marker);
                DAT_10db6ee8[index].SetPosition(step.x, step.y, marker.z);
                DAT_1021f3c0[DAT_1020b430[index].action] = ',';
                DAT_1020b430[index].state = 0;
                DAT_1020b430[index].wait = 0;
                DAT_1020b430[index].talk = 0;
                DAT_1020b430[index].sawPlayer = 0;
                DAT_1020b430[index].behaviour = 0x1f;
            }
            else
                FUN_100241e0(index, step.x, step.y, 0);
            if (marker.x < -20000.0f)
                FUN_10043cc0(index);
        }
    }
    DAT_10210b30[index] = 0;
    return 0;
}


// 0x100b23b0 (Npc_ChildFollow). A child that idles (motion 2) until it sees the player (within its talk id's line
// * 10 units, 18 degrees either side or 150 units, walls ignored) or is told to react: it shows prompt 15, plays
// voice 0x20c and wakes the enemies of group 15 within 1500. It then turns to the player for 30 frames and runs to
// them (motion 1, 16 units a frame, faster while blocked by the map). Once within 450 units it checks every ten
// frames whether the line from 50 units behind it to 50 units short of the player is clear and it can stand; then its
// state is reset. Returns 0.
extern "C" int FUN_100b23b0(int index)
{
    Vec4 player;
    Vec4 self;
    player.Copy(DAT_10db6ee8[0].matrix.GetTranslation());
    self.Copy(DAT_10db6ee8[index].matrix.GetTranslation());
    (player - self).Length();      // computed and not used
    float distance = (player - self).Length2D();
    if (DAT_1020b430[index].state == 0)
    {
        DAT_10db6ee8[index].PlayMotion(2, 0, 5, 1.0f);
        DAT_1020b430[index].state = 1;
        DAT_10210b30[index] = 0;
        return 0;
    }
    if (DAT_1020b430[index].state == 1)
    {
        DAT_10db6ee8[index].PlayMotion(2, 0, 5, 1.0f);
        player.z += 50.0f;
        self.z += 75.0f;
        if ((DAT_1020b430[index].sawPlayer |= FUN_10043e20(&self, &player, DAT_10db6ee8[index].heading,
                                                         (float)(DAT_1020b430[index].talk * 10), 0.31415f, 150.0f, 0)) != 0 ||
            DAT_10210b30[index] != 0)
        {
            FUN_1000d620(index, 15, 60);
            FUN_10065b10(index, 0x20c, 0, 0);
            DAT_1020b430[index].state = 2;
            DAT_1020b430[index].wait = 0;
            FUN_1001b670(index, 1500.0f, 1, 15);
            DAT_10210b30[index] = 0;
            return 0;
        }
    }
    else if (DAT_1020b430[index].state == 2)
    {
        float target = Atan2(player.x - self.x, player.y - self.y) + 3.1415f;
        DAT_1020b430[index].wait++;
        DAT_10db6ee8[index].targetHeading = target;
        float turn = DAT_10db6ee8[index].heading - target;
        while (turn > 3.1415f)
            turn -= 6.283f;
        while (turn < -3.1415f)
            turn += 6.283f;
        if (turn < 0.01745f)
            DAT_10db6ee8[index].PlayMotion(2, 0, 5, 1.0f);
        else
            DAT_10db6ee8[index].PlayMotion(0, 0, 5, 1.0f);
        if (DAT_1020b430[index].wait == 0x1e)
        {
            DAT_1020b430[index].wait = 0;
            DAT_1020b430[index].state = 3;
            DAT_10210b30[index] = 0;
            return 0;
        }
    }
    else if (DAT_1020b430[index].state == 3)
    {
        DAT_10db6ee8[index].m_008d = 1;
        DAT_10db6ee8[index].PlayMotion(1, 0, 5, 1.0f);
        Vec4 step;
        step.Copy(player - self);
        step.z = 0;
        step.Normalized3();
        step.Copy(step * 16.0f);
        DAT_10db6ee8[index].targetHeading = atan2f(step.x, step.y) + 3.1415f;
        FUN_100241e0(index, step.x, step.y, 0);
        step.Copy(step * 6.0f);
        if (DAT_10db6ee8[index].m_238c != 0 && DAT_10db6ee8[index].IsClearOfMapLimited(&step) == 0)
            DAT_10db6ee8[index].AddMotionSpeed(30.0f);
        if (distance < 450.0f)
        {
            Vec4 from;
            Vec4 to;
            Vec4 hit;
            from.Copy(self - player);
            from.z = 0;
            from.Normalized3();
            from.Copy(from * 50.0f);
            to.Copy(player - from);
            from.Copy(from + self);
            DAT_11b002c0 = DAT_10db6ee8[index].collisionFilter;
            to.z += 50.0f;
            from.z += 50.0f;
            if (DAT_104c0380 % 10 == index % 10 && DAT_10255f08 != 0 &&
                DAT_10255f08->Collide(0, 4, &from, &to, &hit) == 0 && DAT_10db6ee8[index].IsClearOfMapLimited(0) != 0 &&
                DAT_10db6ee8[index].IsClearAny(0) != 0)
                FUN_10043cc0(index);
            DAT_11b002c0 = 0;
        }
    }
    DAT_10210b30[index] = 0;
    return 0;
}

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

extern "C" int FUN_1001d8b0(int index, int effect, Vec4 position, int parent, int unused, const Matrix4 *owner,
                            int value, int frame, const Matrix4 *matrix, const void *options);  // Char_StartEffect
extern "C" int FUN_100246b0(const Vec4 *from, const Vec4 *to, Vec4 *hit, int testOnly, const char *name);  // Collision_RayCast

// 0x100b4f00 (Npc_PopUp). A character that bobs up and down where it was placed (motion = the talk id's tens,
// 4 units over a 25-frame cycle) and pops up out of reach: once it has risen above its start height plus the talk
// id's hundreds * 50 it sinks back (dropping at most 20 units a frame, with effect 0x2d5 every frame but each
// third after 43 frames) until it is back at the start. When it is told to react, the player comes within the talk
// id's units * 100 or it already saw the player, it shows prompt 15, jumps forward (20 units along its heading,
// voice 0x20c) and keeps moving until it lands somewhere it can stand with the player in sight (state 4), where
// it stops and its state is reset once its motion is done. Returns 0.
int FUN_100b4f00(int index)
{
    Vec4 player;
    Vec4 self;
    player.Copy(DAT_10db6ee8[0].matrix.GetTranslation());
    self.Copy(DAT_10db6ee8[index].matrix.GetTranslation());
    float distance = (player - self).Length();
    if (DAT_1020b430[index].state < 3 &&
        (DAT_10210b30[index] != 0 || distance < (float)(DAT_1020b430[index].talk % 10 * 100) ||
         DAT_1020b430[index].sawPlayer != 0))
    {
        DAT_10210b30[index] = 0;
        DAT_1020b430[index].state = 3;
        DAT_1020b430[index].wait = 0;
        if (DAT_104bf8f8[index] == 0 || DAT_1021d138[index] != 15)
        {
            DAT_104bf8f8[index] = 1;
            DAT_1021d138[index] = 15;
        }
        DAT_1021f740[index] = 60;
        DAT_10db6ee8[index].mode = 1;
    }
    if (DAT_1020b430[index].state == 0)
    {
        DAT_1020b430[index].state = 1;
        DAT_1020b430[index].range = (int)self.z;
        DAT_10db6ee8[index].mode = 0;
        DAT_10db6ee8[index].m_008d = 1;
    }
    else if (DAT_1020b430[index].state == 1)
    {
        DAT_1020b430[index].wait++;
        DAT_10db6ee8[index].PlayMotion(DAT_1020b430[index].talk / 10 % 10, 0, 5, 1.0f);
        int phase = DAT_1020b430[index].wait % 25;
        DAT_1020b430[index].wait = phase;
        if (phase > 7 && phase < 18)
            DAT_10db6ee8[index].SetPosition(self.x, self.y, self.z + 4.0f);
        else if (phase < 8)
            DAT_10db6ee8[index].SetPosition(self.x, self.y, (float)phase / 8.0f * 4.0f + self.z);
        else if (phase > 17)
            DAT_10db6ee8[index].SetPosition(self.x, self.y, (float)(25 - phase) / 8.0f * 4.0f + self.z);
        if (self.z > (float)(DAT_1020b430[index].range + DAT_1020b430[index].talk / 100 * 50))
        {
            DAT_1020b430[index].state = 2;
            DAT_1020b430[index].wait = 0;
        }
    }
    else if (DAT_1020b430[index].state == 2)
    {
        DAT_10db6ee8[index].PlayMotion(2, 0, 5, 1.0f);
        float start = (float)DAT_1020b430[index].range;
        DAT_1020b430[index].wait++;
        if (start + 50.0f > self.z)
        {
            float speed = (self.z - start) / 50.0f * 20.0f;
            float drop = 1.0f < speed ? speed : 1.0f;
            DAT_10db6ee8[index].SetPosition(self.x, self.y, self.z - drop);
        }
        else
            DAT_10db6ee8[index].SetPosition(self.x, self.y,
                                            self.z - (float)max(min(DAT_1020b430[index].wait - 40, 20), 0));
        if (DAT_1020b430[index].wait > 43 && DAT_1020b430[index].wait % 3 != 0)
        {
            Vec4 offset;
            Matrix4 matrix;
            offset.Fill(0.0f);
            matrix.SetIdentity();
            matrix.SetTranslation(self);
            FUN_1001d8b0(index, 0x2d5, offset, -1, -1, 0, 0, 0, &matrix, 0);
        }
        if (self.z < (float)DAT_1020b430[index].range)
        {
            DAT_1020b430[index].state = 1;
            DAT_1020b430[index].wait = 0;
        }
    }
    else if (DAT_1020b430[index].state == 3)
    {
        DAT_10db6ee8[index].PlayMotion(2, 0, 5, 1.0f);
        if (DAT_1020b430[index].wait == 0)
        {
            DAT_10802bf8[index].position.Fill(0.0f);
            DAT_10802bf8[index].position.y = -20.0f;
            DAT_10802bf8[index].position.RotateZ(3.1415f - DAT_10db6ee8[index].heading);
            DAT_10db6ee8[index].velocity.Copy(DAT_10802bf8[index].position);
            DAT_10db6ee8[index].AddMotionSpeed(20.0f);
            FUN_10065b10(index, 0x20c, 0, 0);
            DAT_10db6ee8[index].m_008d = 1;
            DAT_1020b430[index].wait++;
        }
        else if (DAT_104c0380 % 10 == index % 10)
        {
            Vec4 eye;
            Vec4 head;
            eye.Copy(player);
            head.Copy(self);
            eye.z += 75.0f;
            head.z += 50.0f;
            if (DAT_10db6ee8[index].IsClearOfMapLimited(0) != 0 && FUN_100246b0(&eye, &head, 0, 1, 0) == 0)
            {
                DAT_10db6ee8[index].m_008d = 0;
                DAT_1020b430[index].state = 4;
            }
        }
        FUN_100241e0(index, DAT_10802bf8[index].position.x, DAT_10802bf8[index].position.y,
                     DAT_10802bf8[index].position.z);
    }
    else if (DAT_1020b430[index].state == 4)
    {
        FUN_100241e0(index, DAT_10802bf8[index].position.x, DAT_10802bf8[index].position.y,
                     DAT_10802bf8[index].position.z);
        DAT_10db6ee8[index].m_008d = 0;
        if (DAT_10db6ee8[index].m_238c != 0)
            FUN_10043cc0(index);
    }
    return 0;
}

#include "game.h"

extern "C" void FUN_1000ec70(int frames, float r, float g, float b, float a);   // ColourFade_Start

// 0x100b4300 (Npc_RaiseAlarm). A lookout that idles (motion 2) until it sees the player (within the talk id's
// hundreds * 200 units) or is hurt or flagged in DAT_10468730: it shows prompt 15, plays voice 0x20c and wakes the
// enemies within 1500 (and with talk line set, registers flag line + 300). From then on it keeps waking them and
// flashes the screen red every 30 frames; the first two flashes also sound alarm 0x207 unless an earlier enemy of
// kind 0x62 with m_003c 11 or 12 is already raising it. While the scripts run it turns to face the player whenever
// it can see them. Returns 0.
extern "C" int FUN_100b4300(int index)
{
    Vec4 player;
    Vec4 self;
    player.Copy(DAT_10db6ee8[0].matrix.GetTranslation());
    self.Copy(DAT_10db6ee8[index].matrix.GetTranslation());
    (player - self).Length();      // computed and not used
    (player - self).Length2D();    // computed and not used
    if (DAT_1020b430[index].state == 0)
    {
        DAT_10db6ee8[index].PlayMotion(2, 0, 5, 1.0f);
        DAT_10210b30[index] = 0;
        DAT_1020b430[index].state = 1;
        return 0;
    }
    if (DAT_1020b430[index].state == 1)
    {
        DAT_10db6ee8[index].PlayMotion(2, 0, 5, 1.0f);
        player.z += 50.0f;
        self.z += 75.0f;
        if ((DAT_1020b430[index].sawPlayer |= FUN_10043e20(&self, &player, DAT_10db6ee8[index].heading,
                                                         (float)(DAT_1020b430[index].talk / 100 * 200), 1.099525f, 250.0f, 1)) != 0 ||
            DAT_10468730[index] != 0 || DAT_10daaae8[index].stats[STAT_HEALTH] < DAT_10daaae8[index].stats[STAT_MAX_HEALTH])
        {
            FUN_1000d620(index, 15, 60);
            FUN_10065b10(index, 0x20c, 0, 0);
            DAT_1020b430[index].state = 2;
            DAT_1020b430[index].wait = 0;
            FUN_1001b670(index, 1500.0f, 0);
            if (DAT_1020b430[index].talk % 100 != 0)
            {
                DAT_10874848.RegisterEvent(DAT_1020b430[index].talk % 100 + 300);
                DAT_10210b30[index] = 0;
                return 0;
            }
        }
    }
    else if (DAT_1020b430[index].state == 2)
    {
        FUN_1001b670(index, 1500.0f, 0);
        if (++DAT_1020b430[index].wait == 15)
        {
            if (DAT_1020b430[index].range < 2)
            {
                DAT_1020b430[index].range++;
                int i;
                for (i = 0x3a; i < index; i++)
                {
                    if (DAT_10db6ee8[i].matrix.m[12] > -20000.0f && DAT_10210d30[i] == 0 && DAT_10daaae8[i].kind == 0x62 &&
                        (DAT_1020b430[i].behaviour == 11 || DAT_1020b430[i].behaviour == 12) &&
                        (DAT_1020b430[i].sawPlayer != 0 || DAT_1020b430[i].state == 2))
                        break;
                }
                if (i == index)
                    FUN_1003ad20(0x207, 0, 1.0f);
            }
            FUN_1000ec70(15, 1.0f, 0.5f, 0.5f, 1.0f);
        }
        else if (DAT_1020b430[index].wait == 30)
        {
            FUN_1000ec70(15, 1.0f, 1.0f, 1.0f, 1.0f);
            DAT_1020b430[index].wait = 0;
        }
        if (DAT_1032d69c != 0)
        {
            Vec4 from;
            Vec4 to;
            from.Copy(player);
            to.Copy(self);
            from.z += 75.0f;
            to.z += 75.0f;
            Vec4 hit;
            int visible = DAT_10255f08 == 0 || DAT_10255f08->RayCast(0, 4, 0x4000000, &from, &to, &hit, 0) == 0;
            if (visible)
                DAT_10db6ee8[index].targetHeading = atan2f(player.x - self.x, player.y - self.y) + 3.1415f;
            float turn = DAT_10db6ee8[index].heading - DAT_10db6ee8[index].targetHeading;
            while (turn > 3.1415f)
                turn -= 6.283f;
            while (turn < -3.1415f)
                turn += 6.283f;
            turn = (float)fabs(turn);
            if (turn > 0.01745f)
                DAT_10db6ee8[index].PlayMotion(0, 0, 5, 1.0f);
            else
                DAT_10db6ee8[index].PlayMotion(2, 0, 5, 1.0f);
        }
    }
    DAT_10210b30[index] = 0;
    return 0;
}


// 0x100b4930 (Npc_RideAlarm). A lookout that rides along with the nearest marker character (kind ',' with a model,
// among 0x5c..0x7b; with none m_003c becomes 11): it keeps its distance behind the marker (stored in DAT_10255160)
// as the marker turns, moving at most 12 units a frame, faces the marker's way (motion 2 once it stands still within
// one degree, else motion 0) and is reset when the marker goes away. When it sees the player (within the talk id's
// hundreds * 200 units) or is told to react it shows prompt 15, plays voice 0x20c, wakes the enemies within 1500,
// registers flag line + 300 and from then on raises the alarm: waking the enemies every frame and, twice, alarm 0x207
// with a red flash every 30 frames. Returns 0.
extern "C" int FUN_100b4930(int index)
{
    Vec4 player;
    Vec4 self;
    player.Copy(DAT_10db6ee8[0].matrix.GetTranslation());
    self.Copy(DAT_10db6ee8[index].matrix.GetTranslation());
    (player - self).Length();      // computed and not used
    (player - self).Length2D();    // computed and not used
    if (DAT_1020b430[index].state == 0)
    {
        DAT_10db6ee8[index].PlayMotion(2, 0, 5, 1.0f);
        DAT_1020b430[index].state = 1;
        float nearestSquared = 99999.0f;
        int nearest = -1;
        Vec4 marker;
        for (int i = 0x5c; i < 0x7c; i++)
        {
            if (DAT_1021f3c0[i] == ',' && DAT_10db6ee8[i].root != 0)
            {
                marker.Copy(DAT_10db6ee8[i].matrix.GetTranslation());
                if ((marker - self).LengthSquared3() < nearestSquared)
                {
                    nearest = i;
                    nearestSquared = (marker - self).LengthSquared3();
                }
            }
        }
        DAT_1020b430[index].range = nearest;
        DAT_10255160[index][0] = sqrtf(nearestSquared);
        if (nearest == -1)
            DAT_1020b430[index].behaviour = 11;
    }
    else if (DAT_1020b430[index].state == 1)
    {
        Vec4 place;
        Vec4 step;
        place.y = -DAT_10255160[index][0];
        place.RotateZ(3.1415f - DAT_10db6ee8[DAT_1020b430[index].range].heading);
        place.Copy(place + DAT_10db6ee8[DAT_1020b430[index].range].matrix.GetTranslation());
        step.Copy(place - self);
        if (step.Length() > 12.0f)
        {
            step.Normalized3();
            step.Copy(step * 12.0f);
            place.Copy(self + step);
        }
        DAT_10db6ee8[index].SetPosition(place.x, place.y, place.z);
        float turn = DAT_10db6ee8[DAT_1020b430[index].range].heading;
        DAT_10db6ee8[index].targetHeading = turn;
        turn -= DAT_10db6ee8[index].heading;
        while (turn > 3.1415f)
            turn -= 6.283f;
        while (turn < -3.1415f)
            turn += 6.283f;
        turn = (float)fabs(turn);
        if (turn > 0.01756f || (place - self).LengthSquared3() > 4.0f)
            DAT_10db6ee8[index].PlayMotion(0, 0, 5, 1.0f);
        else
            DAT_10db6ee8[index].PlayMotion(2, 0, 5, 1.0f);
        if (DAT_1020b430[index].action == 0)
        {
            player.z += 50.0f;
            self.z += 75.0f;
            if ((DAT_1020b430[index].sawPlayer |= FUN_10043e20(&self, &player, DAT_10db6ee8[index].heading,
                                                             (float)(DAT_1020b430[index].talk / 100 * 200), 1.099525f, 250.0f, 1)) != 0 ||
                DAT_10210b30[index] != 0)
            {
                FUN_1000d620(index, 15, 60);
                FUN_10065b10(index, 0x20c, 0, 0);
                DAT_1020b430[index].wait = 0;
                DAT_1020b430[index].action = 1;
                FUN_1001b670(index, 1500.0f, 0);
                if (DAT_1020b430[index].talk % 100 != 0)
                    DAT_10874848.RegisterEvent(DAT_1020b430[index].talk % 100 + 300);
            }
        }
        if (DAT_10db6ee8[DAT_1020b430[index].range].matrix.m[12] < -20000.0f)
            FUN_10043cc0(index);
    }
    if (DAT_1020b430[index].action != 0)
    {
        FUN_1001b670(index, 1500.0f, 0);
        if (++DAT_1020b430[index].wait == 15)
        {
            if (DAT_1020b430[index].alarmFlashes < 2)
            {
                FUN_1003ad20(0x207, 0, 1.0f);
                FUN_1000ec70(15, 1.0f, 0.5f, 0.5f, 1.0f);
            }
        }
        else if (DAT_1020b430[index].wait == 30)
        {
            if (DAT_1020b430[index].alarmFlashes < 2)
            {
                FUN_1000ec70(15, 1.0f, 1.0f, 1.0f, 1.0f);
                DAT_1020b430[index].alarmFlashes++;
            }
            DAT_1020b430[index].wait = 0;
        }
    }
    DAT_10210b30[index] = 0;
    return 0;
}


// 0x10098be0 (Enemy_ShoveAndBlast). An enemy that idles (motion 2) and now and then (3 in 100, at most every 100
// frames) either attacks or starts walking (talk 2): walking it follows the player (motion 0, 5 units a frame)
// between 120 and 800 units away, else goes back to idling. An attack (motion 4 for 35 frames) keeps it turned away
// from the player and starts effect 0x61 with sound 0x151 after 20 frames; being told to react (DAT_10210b30)
// starts an attack that chains into a second one. Returns 0.
extern "C" int FUN_10098be0(int index)
{
    Vec4 toPlayer;
    Vec4 self;
    Vec4 player;
    self.Copy(DAT_10db6ee8[index].matrix.GetTranslation());
    player.Copy(DAT_10db6ee8[0].matrix.GetTranslation());
    DAT_10db6ee8[index].important = 1;
    toPlayer.Copy(player - self);
    if (DAT_10210b30[index] != 0)
    {
        DAT_10210b30[index] = 0;
        DAT_1020b430[index].talk = 0;
        DAT_1020b430[index].wait = 0;
        DAT_1020b430[index].action = 2;
        DAT_10db6ee8[index].PlayMotion(4, 1, 5, 1.0f);
        DAT_1020b430[index].wait = 100;
        DAT_1020b430[index].talk = 1;
        DAT_1020b430[index].range = 35;
        DAT_1020b430[index].action = 2;
    }
    if (DAT_1020b430[index].talk == 1)
    {
        if (DAT_1020b430[index].range != 0)
            DAT_1020b430[index].range--;
        DAT_10db6ee8[index].FaceAwayFrom(&DAT_10db6ee8[0]);
        if (DAT_1020b430[index].range == 15)
        {
            Vec4 offset;
            offset.Fill(0.0f);
            EffectOptions options;
            options.m_0000 = 0;
            options.m_0004 = DAT_10daaae8[index].stats[STAT_EFFECT_PARAM];
            options.m_0008 = 1.5f;
            options.owner = index;
            options.m_000e = 1;
            FUN_10065b10(index, 0x151, 0, 0);
            FUN_1001d8b0(index, 0x61, offset, -1, -1, 0, 0, 0, &DAT_10db6ee8[index].matrix, &options);
        }
        if (DAT_1020b430[index].range == 0 && DAT_1020b430[index].action == 2)
        {
            DAT_10db6ee8[index].PlayMotion(4, 1, 5, 1.0f);
            DAT_1020b430[index].wait = 100;
            DAT_1020b430[index].talk = 1;
            DAT_1020b430[index].range = 35;
            DAT_1020b430[index].action = 1;
        }
        else if (DAT_1020b430[index].range == 0)
            DAT_1020b430[index].talk = 0;
    }
    else if (DAT_1020b430[index].talk == 0)
    {
        DAT_10db6ee8[index].PlayMotion(2, 0, 5, 1.0f);
        if (DAT_1020b430[index].wait != 0)
            DAT_1020b430[index].wait--;
        DAT_1020b430[index].action = 2;
        if (FUN_1013f880() % 100 <= 2 && DAT_1020b430[index].wait == 0)
        {
            DAT_10db6ee8[index].PlayMotion(4, 1, 5, 1.0f);
            DAT_1020b430[index].wait = 100;
            DAT_1020b430[index].talk = 1;
            DAT_1020b430[index].range = 35;
        }
        else if (FUN_1013f880() % 100 <= 2 && DAT_1020b430[index].wait == 0)
        {
            DAT_1020b430[index].talk = 2;
            DAT_1020b430[index].wait = 100;
        }
    }
    else
    {
        if (DAT_1020b430[index].wait != 0)
            DAT_1020b430[index].wait--;
        DAT_1020b430[index].action = 2;
        if (FUN_1013f880() % 100 <= 2 && DAT_1020b430[index].wait == 0)
        {
            DAT_10db6ee8[index].PlayMotion(4, 1, 5, 1.0f);
            DAT_1020b430[index].wait = 100;
            DAT_1020b430[index].talk = 1;
            DAT_1020b430[index].range = 35;
        }
        else if (toPlayer.Length() > 120.0f && toPlayer.Length() < 800.0f)
        {
            DAT_10db6ee8[index].PlayMotion(0, 0, 5, 1.0f);
            toPlayer.Normalized3();
            toPlayer.Copy(toPlayer * 5.0f);
            DAT_10db6ee8[index].Push(toPlayer.x, toPlayer.y, 0);
            DAT_10db6ee8[index].targetHeading = (float)atan2(toPlayer.x, toPlayer.y) + 3.1415f;
        }
        else
        {
            DAT_1020b430[index].talk = 0;
            DAT_1020b430[index].wait = 100;
            DAT_10db6ee8[index].PlayMotion(2, 0, 14, 1.0f);
        }
    }
    return 0;
}

// 0x1009dfd0 (Enemy_FollowAndBlast). An enemy that walks towards the player (motion 0, 5 units a frame) while
// farther than 120 units, else idles (motion 2). Now and then (3 in 100, at most every 100 frames) it turns away
// from the player and plays attack motion 4 for 60 frames: after 30 of them it plays sound 0x150 and starts effect
// 0x46 (scale 1.5, kind 1) from its own matrix. Being told to react (DAT_10210b30) cancels the attack. Its motion
// playback is left in DAT_1101d880 (cleared at the start). Returns 0.
extern "C" int FUN_1009dfd0(int index)
{
    DAT_1101d880 = 0;
    Vec4 toPlayer;
    Vec4 self;
    Vec4 player;
    self.Copy(DAT_10db6ee8[index].matrix.GetTranslation());
    player.Copy(DAT_10db6ee8[0].matrix.GetTranslation());
    toPlayer.Copy(player - self);
    if (DAT_10210b30[index] != 0)
    {
        DAT_10210b30[index] = 0;
        DAT_1020b430[index].talk = 0;
        DAT_1020b430[index].wait = 0;
    }
    if (DAT_1020b430[index].talk == 1)
    {
        DAT_10db6ee8[index].FaceAwayFrom(&DAT_10db6ee8[0]);
        if (DAT_1020b430[index].range != 0)
            DAT_1020b430[index].range--;
        if (DAT_1020b430[index].range == 0)
            DAT_1020b430[index].talk = 0;
        if (DAT_1020b430[index].range == 30)
        {
            Vec4 offset;
            offset.Fill(0.0f);
            EffectOptions options;
            options.m_0000 = 0;
            options.m_0004 = DAT_10daaae8[index].stats[STAT_EFFECT_PARAM];
            options.m_0008 = 1.5f;
            options.owner = index;
            options.m_000e = 1;
            FUN_10065b10(index, 0x150, 0, 0);
            FUN_1001d8b0(index, 0x46, offset, -1, -1, 0, 0, 0, &DAT_10db6ee8[index].matrix, &options);
        }
    }
    else
    {
        if (DAT_1020b430[index].wait != 0)
            DAT_1020b430[index].wait--;
        if (FUN_1013f880() % 100 <= 2 && DAT_1020b430[index].wait == 0)
        {
            DAT_10db6ee8[index].FaceAwayFrom(&DAT_10db6ee8[0]);
            DAT_10db6ee8[index].PlayMotion(4, 1, 5, 1.0f);
            DAT_1020b430[index].wait = 100;
            DAT_1020b430[index].talk = 1;
            DAT_1020b430[index].range = 60;
        }
        else if (toPlayer.Length() > 120.0f)
        {
            DAT_10db6ee8[index].PlayMotion(0, 0, 5, 1.0f);
            toPlayer.Normalized3();
            toPlayer.Copy(toPlayer * 5.0f);
            DAT_10db6ee8[index].Push(toPlayer.x, toPlayer.y, 0);
            DAT_10db6ee8[index].targetHeading = Atan2(toPlayer.x, toPlayer.y) + 3.1415f;
        }
        else
            DAT_10db6ee8[index].PlayMotion(2, 0, 5, 1.0f);
    }
    DAT_1101d880 = DAT_10db6ee8[index].motion;
    return 0;
}

// 0x1009d100 (Enemy_ChaseAndBlast). Like Enemy_FollowAndBlast: it runs to the player (motion 1, 15 units a frame)
// while farther than 300 units, walks (motion 0, 5 units) while farther than 120, else idles. Now and then (3 in
// 100, at most every 100 frames) it turns away from the player, plays sound 0x14f and starts effect 0x5e (scale 1.5,
// kind 1) from its own matrix, then stands idle for 30 frames. Returns 0.
extern "C" int FUN_1009d100(int index)
{
    Vec4 toPlayer;
    Vec4 self;
    Vec4 player;
    self.Copy(DAT_10db6ee8[index].matrix.GetTranslation());
    player.Copy(DAT_10db6ee8[0].matrix.GetTranslation());
    toPlayer.Copy(player - self);
    if (DAT_1020b430[index].talk == 1)
    {
        DAT_10db6ee8[index].PlayMotion(2, 0, 5, 1.0f);
        if (DAT_1020b430[index].range != 0)
            DAT_1020b430[index].range--;
        if (DAT_1020b430[index].range == 0)
            DAT_1020b430[index].talk = 0;
    }
    else
    {
        if (DAT_1020b430[index].wait != 0)
            DAT_1020b430[index].wait--;
        if (FUN_1013f880() % 100 <= 2 && DAT_1020b430[index].wait == 0)
        {
            DAT_10db6ee8[index].FaceAwayFrom(&DAT_10db6ee8[0]);
            Vec4 offset;
            offset.Fill(0.0f);
            EffectOptions options;
            options.m_0000 = 0;
            options.m_0004 = DAT_10daaae8[index].stats[STAT_EFFECT_PARAM];
            options.m_0008 = 1.5f;
            options.owner = index;
            options.m_000e = 1;
            FUN_10065b10(index, 0x14f, 0, 0);
            FUN_1001d8b0(index, 0x5e, offset, -1, -1, 0, 0, 0, &DAT_10db6ee8[index].matrix, &options);
            DAT_1020b430[index].wait = 100;
            DAT_1020b430[index].talk = 1;
            DAT_1020b430[index].range = 30;
        }
        else if (toPlayer.Length() > 300.0f)
        {
            DAT_10db6ee8[index].PlayMotion(1, 0, 5, 1.0f);
            toPlayer.Normalized3();
            toPlayer.Copy(toPlayer * 5.0f * 3.0f);
            DAT_10db6ee8[index].Push(toPlayer.x, toPlayer.y, 0);
            DAT_10db6ee8[index].targetHeading = Atan2(toPlayer.x, toPlayer.y) + 3.1415f;
        }
        else if (toPlayer.Length() > 120.0f)
        {
            DAT_10db6ee8[index].PlayMotion(0, 0, 5, 1.0f);
            toPlayer.Normalized3();
            toPlayer.Copy(toPlayer * 5.0f);
            DAT_10db6ee8[index].Push(toPlayer.x, toPlayer.y, 0);
            DAT_10db6ee8[index].targetHeading = Atan2(toPlayer.x, toPlayer.y) + 3.1415f;
        }
        else
            DAT_10db6ee8[index].PlayMotion(2, 0, 5, 1.0f);
    }
    return 0;
}

Matrix4 RotationY(float angle);               // 0x101b1bb0

// 0x10048b70 (Char_SwingDoor). A door on a character slot: its model's root matrix is saved on the first frame and
// then turned about Y by the talk id's units * 45 degrees. While open (state 1) it may rise (m_0010 set: m_0014 / 2
// units a frame, counting m_0014 down) and shows effect 0x2db at its position. Closing (state 2, set by the scripts)
// it swings 10 degrees a frame towards 0 or 360 degrees; once shut (with the scripts loaded) its state is reset.
// Returns 0.
int FUN_10048b70(int index)
{
    DAT_10db6ee8[index].mode = 0;
    if (DAT_1020b430[index].state == 0)
    {
        DAT_1020b430[index].range = DAT_1020b430[index].talk % 10 * 45;
        DAT_10868838[index][0] = DAT_10db6ee8[index].root->matrix;
        DAT_1020b430[index].state = 1;
    }
    else if (DAT_1020b430[index].state == 1)
    {
        DAT_10db6ee8[index].root->matrix = DAT_10868838[index][0] * RotationY((float)DAT_1020b430[index].range * 3.1415f / 180.0f);
        if (DAT_1020b430[index].action != 0)
            DAT_10db6ee8[index].matrix.m[14] += (float)(DAT_1020b430[index].alarmFlashes-- / 2);
        Matrix4 matrix;
        Vec4 position;
        matrix.SetIdentity();
        matrix.SetTranslation(DAT_10db6ee8[index].matrix.GetTranslation());
        DAT_10daaa80.Start(0x2db, position, -1, -1, 0, 0, 0, &matrix, 0, 0);
    }
    else if (DAT_1020b430[index].state == 2)
    {
        DAT_10db6ee8[index].mode = 1;
        if (DAT_1020b430[index].range > 180)
        {
            if (DAT_1020b430[index].range < 360)
            {
                DAT_1020b430[index].range += 10;
                if (DAT_1020b430[index].range >= 360)
                    DAT_1020b430[index].range = 0;
            }
        }
        else if (DAT_1020b430[index].range > 0)
        {
            DAT_1020b430[index].range -= 10;
            if (DAT_1020b430[index].range <= 0)
                DAT_1020b430[index].range = 0;
        }
        DAT_10db6ee8[index].root->matrix = DAT_10868838[index][0] * RotationY((float)DAT_1020b430[index].range * 3.1415f / 180.0f);
        if (DAT_1020b430[index].range == 0 && DAT_1032d69c != 0)
        {
            DAT_10db6ee8[index].mode = 1;
            FUN_10043cc0(index);
        }
    }
    return 0;
}


// 0x10048e90 (Enemy_DropIn). An enemy that drops in from above: on its first frame it is refilled if defeated and
// placed 200 units ahead and 650 below where it stands (talk id's units 1 or 2 also set DAT_103153b0 to 150), with
// its group timer set to 1. Then it rises in an arc over 20 frames (up 4 * frames left, back 16 * (t/20)^2) and
// keeps moving that way (1.25 times as fast) until it stands on the map: then it turns to the player when the talk
// id has tens, shows prompt 15 when it has hundreds, and its state is reset. Nothing happens while it is moved
// away (z below -20000). Returns 0.
extern "C" int FUN_10048e90(int index)
{
    Vec4 player;
    Vec4 self;
    player.Copy(DAT_10db6ee8[0].matrix.GetTranslation());
    self.Copy(DAT_10db6ee8[index].matrix.GetTranslation());
    DAT_1021f9c0[index] = 0;
    DAT_10255a88[index] = 0;
    DAT_102550e0[index] = 0;
    if (DAT_1020b430[index].state != 0 && DAT_10daaae8[index].stats[STAT_HEALTH] <= 0)
        DAT_1021f9c0[index] = 1;
    if (DAT_10db6ee8[index].matrix.m[14] < -20000.0f)
        return 0;
    if (DAT_1020b430[index].state == 0)
    {
        DAT_10db6ee8[index].alpha = 1.0f;
        DAT_10db6ee8[index].mode = 0;
        switch (DAT_1020b430[index].talk % 10)
        {
        case 1:
            DAT_103153b0[index] = 150;
            break;
        case 2:
            DAT_103153b0[index] = 150;
            break;
        }
        if (DAT_10daaae8[index].stats[STAT_HEALTH] <= 0)
        {
            DAT_10daaae8[index].stats[STAT_HEALTH] = DAT_10daaae8[index].stats[STAT_MAX_HEALTH];
            DAT_10210d30[index] = 0;
        }
        DAT_1020b430[index].state = 1;
        Vec4 place;
        place.y = 200.0f;
        place.RotateZ(-DAT_10db6ee8[index].heading);
        place.z = -650.0f;
        place.Copy(place + self);
        DAT_10db6ee8[index].SetPosition(place.x, place.y, place.z);
        DAT_102199b8[index] = 1;
        return 0;
    }
    if (DAT_1020b430[index].state == 1)
    {
        DAT_10db6ee8[index].m_008d = 1;
        DAT_10db6ee8[index].PlayMotion(2, 0, 5, 1.0f);
        Vec4 step;
        int t = DAT_1020b430[index].wait;
        if (t >= 20)
            t = 20;
        step.y = (float)t / 20.0f;
        step.y *= step.y;
        step.y *= -16.0f;
        step.RotateZ(-DAT_10db6ee8[index].heading);
        step.z = (float)(20 - DAT_1020b430[index].wait) * 4.0f;
        if (++DAT_1020b430[index].wait == 20)
        {
            DAT_10db6ee8[index].mode = 1;
            DAT_10db6ee8[index].m_238c = 0;
        }
        else if (DAT_1020b430[index].wait < 20)
        {
            step.Copy(step + self);
            DAT_10db6ee8[index].SetPosition(step.x, step.y, step.z);
            DAT_10db6ee8[index].velocity.Copy(step);
            return 0;
        }
        step.Copy(step * 1.25f);
        FUN_100241e0(index, step.x, step.y, 0);
        if (DAT_10db6ee8[index].m_238c != 0 && DAT_10db6ee8[index].IsClearOfMapLimited(0) != 0)
        {
            DAT_10db6ee8[index].m_008d = 0;
            if (DAT_1020b430[index].talk / 10 % 10 != 0)
                DAT_10db6ee8[index].targetHeading = atan2f(player.x - self.x, player.y - self.y) + 3.1415f;
            if (DAT_1020b430[index].talk / 100 != 0)
                FUN_1000d620(index, 15, 60);
            DAT_1021f9c0[index] = 1;
            DAT_10255a88[index] = 1;
            FUN_10043cc0(index);
        }
    }
    return 0;
}

// 0x10049240 (Enemy_DropInWhenNear). Enemy_DropIn that first waits hidden (m_0d4c 0, slot flag m_00a5 set) until the
// player comes within 800 units, then drops in the same way (without waiting to stand on the map, only for its
// motion to settle). Returns 0.
extern "C" int FUN_10049240(int index)
{
    Vec4 player;
    Vec4 self;
    player.Copy(DAT_10db6ee8[0].matrix.GetTranslation());
    self.Copy(DAT_10db6ee8[index].matrix.GetTranslation());
    DAT_1021f9c0[index] = 0;
    DAT_10255a88[index] = 0;
    DAT_102550e0[index] = 0;
    if (DAT_1020b430[index].state != 0 && DAT_10daaae8[index].stats[STAT_HEALTH] <= 0)
        DAT_1021f9c0[index] = 1;
    if (DAT_10db6ee8[index].matrix.m[14] < -20000.0f)
        return 0;
    if (DAT_1020b430[index].state == 0)
    {
        DAT_10db6ee8[index].alpha = 0;
        DAT_1020b430[index].state = 3;
        return 0;
    }
    if (DAT_1020b430[index].state == 1)
    {
        DAT_10db6ee8[index].PlayMotion(2, 0, 5, 1.0f);
        Vec4 step;
        int t = DAT_1020b430[index].wait;
        if (t >= 20)
            t = 20;
        step.y = (float)t / 20.0f;
        step.y *= step.y;
        step.y *= -16.0f;
        step.RotateZ(-DAT_10db6ee8[index].heading);
        step.z = (float)(20 - DAT_1020b430[index].wait) * 4.0f;
        if (++DAT_1020b430[index].wait == 20)
        {
            DAT_10db6ee8[index].mode = 1;
            DAT_10db6ee8[index].m_238c = 0;
        }
        else if (DAT_1020b430[index].wait < 20)
        {
            step.Copy(step + self);
            DAT_10db6ee8[index].SetPosition(step.x, step.y, step.z);
            DAT_10db6ee8[index].velocity.Copy(step);
            return 0;
        }
        step.Copy(step * 1.25f);
        FUN_100241e0(index, step.x, step.y, 0);
        if (DAT_10db6ee8[index].m_238c != 0)
        {
            if (DAT_1020b430[index].talk / 10 % 10 != 0)
                DAT_10db6ee8[index].targetHeading = atan2f(player.x - self.x, player.y - self.y) + 3.1415f;
            if (DAT_1020b430[index].talk / 100 != 0)
                FUN_1000d620(index, 15, 60);
            DAT_1021f9c0[index] = 1;
            DAT_10255a88[index] = 1;
            FUN_10043cc0(index);
        }
    }
    else if (DAT_1020b430[index].state == 2)
    {
        DAT_10daaae8[index].m_0141 = 0;
        DAT_10db6ee8[index].mode = 0;
        switch (DAT_1020b430[index].talk % 10)
        {
        case 1:
            DAT_103153b0[index] = 150;
            break;
        case 2:
            DAT_103153b0[index] = 150;
            break;
        }
        if (DAT_10daaae8[index].stats[STAT_HEALTH] <= 0)
        {
            DAT_10daaae8[index].stats[STAT_HEALTH] = DAT_10daaae8[index].stats[STAT_MAX_HEALTH];
            DAT_10210d30[index] = 0;
        }
        DAT_1020b430[index].state = 1;
        Vec4 place;
        place.y = 200.0f;
        place.RotateZ(-DAT_10db6ee8[index].heading);
        place.z = -650.0f;
        place.Copy(place + self);
        DAT_10db6ee8[index].SetPosition(place.x, place.y, place.z);
        DAT_102199b8[index] = 1;
        DAT_10db6ee8[index].alpha = 1.0f;
        return 0;
    }
    else if (DAT_1020b430[index].state == 3)
    {
        DAT_10db6ee8[index].alpha = 0;
        DAT_10daaae8[index].m_0141 = 1;
        if ((player - self).Length() < 800.0f)
            DAT_1020b430[index].state = 2;
    }
    return 0;
}
