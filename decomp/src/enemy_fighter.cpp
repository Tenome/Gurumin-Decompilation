// The fighter of kind 0x2d (0x100bdad0, a per-kind behaviour of 0x100f9670): an enemy that circles the player,
// picks attack steps (slashes, kicks, a dash, a ranged shot), swings with trail effects and, when beaten, plays a
// defeat sequence (the first fighter, slot 0x3a, takes the camera). See Enemy_UpdateFighter.

#include <math.h>
#include <stdlib.h>

#include "vec4.h"
#include "matrix4.h"
#include "node.h"
#include "effect_manager.h"
#include "anim_model.h"
#include "char_state.h"
#include "char_slot.h"
#include "char_tables.h"
#include "char_vars.h"
#include "game_vars.h"
#include "map_vars.h"
#include "camera_vars.h"
#include "render_vars.h"
#include "world.h"
#include "scene_reset.h"
#include "script_system.h"
#include "texture_manager.h"
#include "game.h"

#include "enemy_fighter.h"
#include "hop_spit_enemy.h"
#include "partner_follow.h"
#include "game_main.h"
#include "enemy_chase_hop.h"
#include "char_spawn.h"
#include "texture_load.h"
#include "value_command.h"
#include "scene_command.h"
#include "route_flier.h"
#include "target_race.h"
#include "path_walk.h"
#include "field_item_menu.h"
#include "dodge_golem.h"
#include "math_float.h"

#define FIGHTER_TWIN 2                    // the twin model and slot record sit two entries on
#define FIGHTER_FIRST_SLOT 0x3a
#define FIGHTER_PI 3.1415f
#define FIGHTER_TWO_PI 6.283f
#define FIGHTER_SWAP_STEPS 7
#define FIGHTER_SWAP_DONE 0x32
#define FIGHTER_BLINK_MAX 0xd2
#define FIGHTER_STAGGER_STAT 6            // CharSlot::stats[6] reaches 100 when it is staggered
#define FIGHTER_SWING_EFFECT 0x457
#define FIGHTER_SWING_SIDE 0x5f0
#define FIGHTER_SWING_OFF 0x5f1
#define FIGHTER_NODE_BODY 0
#define FIGHTER_NODE_LEFT_TOE 1
#define FIGHTER_NODE_LEFT_HAND 2
#define FIGHTER_NODE_RIGHT_TOE 3
#define FIGHTER_NODE_FOOTSTEPS 4
#define FIGHTER_NODE_RIGHT_HAND 5
#define FIGHTER_CLONE_EFFECT_SCALE 0.75f
#define FIGHTER_NEAR_RANGE 400.0f
#define FIGHTER_VOICE_FLAG 0xf79          // script flag: the long voice set

#define W(n) DAT_1020b430[index].words[n]
#define MODEL DAT_10db6ee8[index]                         // the fighter's model
#define TWIN DAT_10db6ee8[index + FIGHTER_TWIN]            // its twin model
#define SLOT DAT_10daaae8[index]                           // the fighter's slot record
#define TWIN_SLOT DAT_10daaae8[index + FIGHTER_TWIN]       // its twin's slot record
#define STATE_FRAME W(FIGHTER_WORD_FRAME)
#define STATE_STEP W(FIGHTER_WORD_STEP)

// Starts a swing effect and then its trail (the options follow the fighter).
static __forceinline void Fighter_Swing(int index, int swingEffect, int trailEffect, Node *swingNode, Node *trailNode, int sound)
{
    EffectOptions options;
    Matrix4 turn;
    turn.SetIdentity();
    turn = RotationZ(DAT_10db6ee8[index].heading);
    Vec4 origin;
    options.m_0000 = 0;
    options.m_0004 = DAT_10daaae8[index].stats[3];
    options.followOwner = 1;
    options.m_000e = 5;
    options.owner = index;
    if (swingEffect != 0)
        FUN_1001d8b0(index, swingEffect, origin, -1, -1, &swingNode->worldMatrix, 0, 0, &turn, &options);
    if (sound != 0)
        FUN_10065b10(index, sound, 1, 0);
    options.m_0004 = 0;
    if (trailEffect != 0)
        FUN_1001d8b0(index, trailEffect, origin, -1, -1, &trailNode->worldMatrix, 0, 0, 0, &options);
    DAT_10218fc0[index] = 1;
}

// Plays one of the fighter's grunts: the long voice set when script flag 0xf79 is set, else the short one.
#define Fighter_Grunt(index)     do     {         int voice;         if (DAT_10874848.TestScriptFlag(FIGHTER_VOICE_FLAG) == 0)             voice = FUN_1013f880() % 3 + 0x34;         else             voice = FUN_1013f880() % 6 + 0x31;         FUN_100b9bb0(index, voice, 0, 0);     } while (0)

// Picks one of the strike steps at random (never step 0xf more often than one time in 200).
static __forceinline int Fighter_RandomStrike(int index, int fatigueLimit)
{
    int roll;
    do
    {
        if (DAT_1020b498[index].value < fatigueLimit)
            roll = FUN_1013f880() % 5 + FIGHTER_STEP_STRIKE_FIRST;
        else
            roll = FUN_1013f880() % 6 + FIGHTER_STEP_STRIKE_FIRST;
        W(FIGHTER_WORD_STEP) = roll;
    } while (roll == 0xf && FUN_1013f880() % 1000 > 5);
    return roll;
}

// 0x100bdad0 (Enemy_UpdateFighter). One frame of the fighter in slot `index`: swaps its model with the twin
// while it fades, plays its defeat sequence (the first fighter also drives the camera), keeps the step it plays
// (WORD_STEP) and the frame in it (WORD_FRAME), starts the swing effects on the key frames, chooses the next
// step when one ends and finally moves and turns the model and starts its motion. Returns 0 (1 while the first
// fighter's defeat sequence runs).
int Enemy_UpdateFighter(int index)
{
    int finished = 0;
    int category;
    int roll;
    int phase;
    float distance;

    if (SLOT.stats[FIGHTER_STAGGER_STAT] == 100 && STATE_STEP != FIGHTER_STEP_STAGGER)
    {
        STATE_STEP = FIGHTER_STEP_STAGGER;
        STATE_FRAME = 0;
    }
    TWIN_SLOT.stats[STAT_BUSY] = 1;
    TWIN_SLOT.m_0141 = 1;
    TWIN.m_0294 = 1;
    TWIN.mode = 1;
    DAT_10210d30[index + FIGHTER_TWIN] = 0;
    DAT_1021f9c0[index + FIGHTER_TWIN] = 0;
    DAT_10255a88[index + FIGHTER_TWIN] = 0;
    TWIN.targetHeading = MODEL.heading;
    TWIN.heading = MODEL.heading;
    if (TWIN.root != 0)
        TWIN.root->M_10194730(0x20, 0x400);
    if (MODEL.motionFrame > -1)
        TWIN.motionFrame = MODEL.motionFrame;
    else
    {
        TWIN.m_0294 = 0;
        if (MODEL.motion->blending != 0)
            TWIN.PlayMotion(MODEL.motion->nextLayer, 0, 5, 1.0f);
    }

    phase = DAT_1020b4a8[index].value;
    if (phase != 0)
    {
        if (index == FIGHTER_FIRST_SLOT)
            DAT_10daaae8[0x3c].stats[STAT_BUSY] = 0;
        if (phase == 1)
            TWIN_SLOT.stats[STAT_HEALTH] = SLOT.stats[STAT_HEALTH];
        SLOT.stats[STAT_HEALTH] = TWIN_SLOT.stats[STAT_HEALTH];
        TWIN_SLOT.stats[STAT_VALUE_4] = SLOT.stats[STAT_VALUE_4];
        TWIN_SLOT.m_0141 = 0;
        DAT_102550e0[index] = DAT_102550e0[index + FIGHTER_TWIN];
        if (phase < FIGHTER_SWAP_STEPS)
        {
            MODEL.alpha = (float)(FIGHTER_SWAP_STEPS - phase) / 7.0f;
            DAT_1020b4a8[index].value = phase + 1;
            TWIN.alpha = (float)phase / 7.0f;
        }
        else if (phase == FIGHTER_SWAP_STEPS)
        {
            MODEL.alpha = 0.0f;
            TWIN.alpha = 1.0f;
            if (DAT_10210d30[index] != 0 && W(14) != 0)
                DAT_1020b4a8[index].value = FIGHTER_SWAP_DONE;
        }
        else if (phase < 0xf)
        {
            TWIN.alpha = (float)(0xf - phase) / 7.0f;
            DAT_1020b4a8[index].value = phase + 1;
            MODEL.alpha = (float)(phase - 8) / 7.0f;
        }
        else if (phase == FIGHTER_SWAP_DONE)
            MODEL.alpha = 0.0f;
        else
        {
            MODEL.alpha = 1.0f;
            DAT_1020b4a8[index].value = 0;
            TWIN.alpha = 0.0f;
        }
    }
    else
    {
        TWIN_SLOT.m_0141 = 1;
        TWIN_SLOT.stats[STAT_HEALTH] = TWIN_SLOT.stats[STAT_MAX_HEALTH];
        TWIN.alpha = 0.0f;
    }

    if (DAT_103153b0[index] < DAT_103153b0[index + FIGHTER_TWIN])
    {
        DAT_103153b0[index] = DAT_103153b0[index + FIGHTER_TWIN];
        DAT_103153b0[index + FIGHTER_TWIN] = 0;
    }
    if (DAT_103153b0[index] > FIGHTER_BLINK_MAX)
        DAT_103153b0[index] = FIGHTER_BLINK_MAX;
    if (DAT_103153b0[index + FIGHTER_TWIN] > FIGHTER_BLINK_MAX)
        DAT_103153b0[index + FIGHTER_TWIN] = FIGHTER_BLINK_MAX;
    roll = DAT_1020b4b0[index].value;
    if (roll == 0)
        DAT_1020b4b0[index].value = DAT_103153b0[index];
    else if (DAT_103153b0[index] > roll)
    {
        if (roll < 0)
        {
            DAT_103153b0[index + FIGHTER_TWIN] = 0;
            DAT_103153b0[index] = 0;
            DAT_1020b4b0[index].value = roll + 1;
        }
        else
        {
            DAT_103153b0[index + FIGHTER_TWIN] = (short)(DAT_1020b4b0[index].value - 1);
            DAT_103153b0[index] = (short)(DAT_1020b4b0[index].value - 1);
        }
    }
    else if (roll < 0)
        DAT_1020b4b0[index].value = roll + 1;
    else if (roll == 0 || DAT_103153b0[index] != 0)
        DAT_1020b4b0[index].value = DAT_103153b0[index];
    else
        DAT_1020b4b0[index].value = -120;

    Vec4 defeatPos;
    int motion = -1;
    int motionSnap = 0;
    int blink = 0;
    if (index != FIGHTER_FIRST_SLOT && DAT_10210d30[index] != 0)
    {
        if (DAT_1020b4a4[index].value != 0)
            return 0;
        FUN_1000ec70(0xf, 1.0f, 1.0f, 1.0f, 1.0f);
        phase = DAT_1020b4a8[index].value;
        DAT_1020b4a4[index].value = 1;
        if (phase != 0 && phase < 8)
            DAT_1020b4a8[index].value = 0xf - phase;
        if (SLOT.stats[STAT_VALUE_4] < 0)
            SLOT.stats[STAT_VALUE_4] = DAT_1020b48c[index].value;
        return 0;
    }

    DAT_1020b4a4[index].value = 0;
    if (index > FIGHTER_FIRST_SLOT)
        SLOT.stats[STAT_BUSY] = 1;
    if (index < FIGHTER_FIRST_SLOT)
    {
        SLOT.stats[2] = 0;
        DAT_10210d30[index] = 1;
        MODEL.SetPosition(-200000.0f, 0.0f, 0.0f);
    }
    DAT_1020b484[index].value = 0;
    if (MODEL.matrix._41 < -10000.0f)
        return 1;
    if ((DAT_104bfdf0 != 0 || DAT_104c0135 != 0 || DAT_104bfe1c != 0) && DAT_1020b480[index].value == 0)
    {
        if (MODEL.m_0294 == 0)
        {
            Playback *playback = MODEL.motion;
            MODEL.m_0294 = 1;
            DAT_1020e138[index] = playback->blending == 0 ? playback->layer : -2;
            DAT_10253a58[index] = playback->nextLayer;
            DAT_1042dba8[index] = MODEL.motionFrame;
        }
        return 1;
    }
    if (MODEL.m_0294 != 0)
        MODEL.m_0294 = 0;
    DAT_1020b480[index].value = 0;
    if (index == FIGHTER_FIRST_SLOT)
        DAT_104c0110 = index;

    if (DAT_10210d30[index] != 0)
    {
        // The defeat sequence: the models flash and shrink away, the camera circles the body, then the map goes on.
        int counter;
        int flagBase;
        FUN_10011c60();
        flagBase = DAT_10c63ffc * 2 + 1000;
        roll = DAT_10874848.TestScriptFlag(flagBase);
        if (flagBase < 4000 && flagBase != 0)
            DAT_10c62edc[flagBase] = roll | (1 << ((index - FIGHTER_FIRST_SLOT) & 0x1f));
        SLOT.m_0141 = 1;
        SLOT.stats[3] = 0;
        SLOT.stats[STAT_BUSY] = 1;
        DAT_10db6ee8[index + 1].m_17e8 = 1;
        DAT_10db6ee8[88].m_17e8 = 1;
        DAT_10db6ee8[89].m_17e8 = 1;
        DAT_104c0354 = 2;
        DAT_10868838[index][0]._41 *= 0.95f;
        DAT_10868838[index][0]._42 *= 0.95f;
        DAT_10868838[index][0]._43 -= 10.0f;
        DAT_10868838[index][1]._41 *= 0.95f;
        DAT_10868838[index][1]._42 *= 0.95f;
        DAT_10868838[index][1]._43 -= 10.0f;
        counter = W(FIGHTER_WORD_DEFEAT_FRAMES);
        if (counter == 1)
        {
            FUN_1000ec70(0xf, 1.0f, 1.0f, 1.0f, 1.0f);
            defeatPos.Copy(MODEL.matrix.GetTranslation());
            defeatPos.z += 50.0f;
            FUN_1001d8b0(index, 0x33, defeatPos, -1, -1, 0, 0, 0, 0, 0);
            MODEL.PlayMotion(0x19, 1, 5, 1.0f);
            MODEL.mode = 0;
            TWIN.mode = 0;
            DAT_10db6ee8[88].mode = 0;
            DAT_10db6ee8[89].mode = 0;
            DAT_10db6ee8[88].m_17e8 = 1;
            DAT_10db6ee8[89].m_17e8 = 1;
            DAT_10daaa80.RemoveOfType(0x435);
            DAT_10daaa80.RemoveOfType(0x443);
            DAT_10daaa80.RemoveOfType(0x427);
            DAT_10daaa80.RemoveOfType(0x429);
            DAT_10daaa80.RemoveOfType(0x42a);
            DAT_10daaa80.RemoveOfType(0x42c);
            DAT_10daaa80.RemoveOfType(0x420);
            DAT_10daaa80.RemoveOfType(0x41a);
            DAT_10daaa80.RemoveOfType(0x431);
        }
        else if (counter < 1 && counter % 10 == 0)
        {
            defeatPos.Copy(MODEL.matrix.GetTranslation());
            defeatPos.z += 50.0f;
            FUN_1001d8b0(index, 0x2580, defeatPos, -1, -1, 0, 0, 0, 0, 0);
            FUN_1001d8b0(index, 0x2580, defeatPos, -1, -1, 0, 0, 0, 0, 0);
            FUN_1001d8b0(index, 0x2580, defeatPos, -1, -1, 0, 0, 0, 0, 0);
            FUN_1001d8b0(index, 0x2580, defeatPos, -1, -1, 0, 0, 0, 0, 0);
            FUN_1001d8b0(index, 0x2580, defeatPos, -1, -1, 0, 0, 0, 0, 0);
        }
        roll = DAT_1020b4ac[index].value;
        if (roll != 0)
        {
            if (roll < 0xf)
                TWIN.alpha = (float)(0xf - roll) / 15.0f;
            else if (roll == 0xf)
            {
                TWIN.alpha = 0.0f;
                TWIN.SetPosition(0.0f, 2850.0f, 1100.0f);
            }
            else if (roll < 0x1e)
                TWIN.alpha = (float)(roll - 0xf) / 15.0f;
            else
            {
                TWIN.alpha = 1.0f;
                DAT_1020b4ac[index].value = -1;
            }
            DAT_1020b4ac[index].value++;
        }
        if (W(FIGHTER_WORD_DEFEAT_FRAMES) == 0x10e)
        {
            FUN_1000db30(0, 0, 0);
            FUN_1000db30(0, 100, 0x1e);
        }
        else if (W(FIGHTER_WORD_DEFEAT_FRAMES) == 300)
        {
            DAT_10874848.MarkKilled(SLOT.routeRecord);
            FUN_1000ec70(0xf, 1.0f, 1.0f, 1.0f, 1.0f);
            MODEL.SetPosition(-200000.0f, 0.0f, 0.0f);
            TWIN.SetPosition(-200000.0f, 0.0f, 0.0f);
            DAT_10db6ee8[88].SetPosition(-200000.0f, 0.0f, 0.0f);
            DAT_10db6ee8[89].SetPosition(-200000.0f, 0.0f, 0.0f);
            DAT_104c00c8 = 0;
            DAT_104c0110 = 0;
            FUN_1013fb10(3, 0, 10);
            DAT_104c0354 = 0;
        }
        if (W(14) == 0)
        {
            MODEL.PlayMotion(2, 0, 5, 1.0f);
            FUN_10065b10(index, 0x2d1, 1, 0);
            if (DAT_104bfe78 == 0)
                W(14) = 1;
            roll = DAT_1020b4a8[index].value;
            if (roll == 0)
                DAT_1020b4a8[index].value = 1;
            else if (roll > 7 && roll < FIGHTER_SWAP_DONE)
            {
                roll = 0xf - roll;
                if (roll < 7)
                    roll = 7;
                DAT_1020b4a8[index].value = roll;
            }
            DAT_10db6ee8[88].targetHeading = MODEL.targetHeading;
            DAT_10db6ee8[89].targetHeading = MODEL.targetHeading;
            DAT_10db6ee8[88].heading = MODEL.heading;
            DAT_10db6ee8[89].heading = MODEL.heading;
            W(FIGHTER_WORD_DEFEAT_FRAMES) = -30;
            DAT_10255160[index][0] = (DAT_118e69e8 - DAT_118e6cb8).Length();
            if (MODEL.root != 0)
                MODEL.root->M_101946b0(0, 0xffffffff);
            if (TWIN.root != 0)
                TWIN.root->M_101946b0(0, 0xffffffff);
            if (DAT_10db6ee8[88].root != 0)
                DAT_10db6ee8[88].root->M_101946b0(0, 0xffffffff);
            if (DAT_10db6ee8[89].root != 0)
                DAT_10db6ee8[89].root->M_101946b0(0, 0xffffffff);
            DAT_104c00c8 = 1;
            DAT_10453008[index] = -10001;
            SLOT.stats[3] = 0;
            DAT_10db6ee8[88].alpha = 0.0f;
            DAT_10db6ee8[89].alpha = 0.0f;
        }
        counter = W(FIGHTER_WORD_DEFEAT_FRAMES);
        if (counter > 0x3c)
        {
            if (counter < 0xb4)
            {
                float left = (float)(0xb4 - counter);
                TWIN.alpha = (left * left) / 14400.0f;
            }
            else
            {
                MODEL.m_17e8 = 1;
                MODEL.m_0294 = 1;
                MODEL.motionFrame = DAT_10db6ee8[88].motionFrame;
                MODEL.alpha = 0.0f;
                TWIN.m_17e8 = 1;
                TWIN.alpha = 0.0f;
            }
        }
        if (counter >= 0x3d && counter < 0x8c)
        {
            int bob = 100 - counter;
            if (bob < 0)
                bob = -bob;
            MODEL.SetPosition(MODEL.matrix._41, MODEL.matrix._42, ((float)(0x28 - bob) * 5.0f) / 40.0f + MODEL.matrix._43);
        }
        else if (counter >= 0x8c)
        {
            MODEL.SetPosition(MODEL.matrix._41, MODEL.matrix._42, MODEL.matrix._43);
            MODEL.m_17e8 = 1;
        }
        if (W(FIGHTER_WORD_DEFEAT_FRAMES) == 0x3c)
        {
            FUN_1013fb10(3, 200, 0x1e);
            FUN_100b9bb0(index, 0x3f, 1, 0);
        }
        counter = W(FIGHTER_WORD_DEFEAT_FRAMES);
        if (counter > 0x3c)
        {
            AnimModel &flash = DAT_10db6ee8[88];
            if (counter < 0x96)
            {
                float since = (float)(counter - 0x3c);
                flash.scale.x = 1.0f;
                flash.scale.y = 0.0f;
                flash.scale.z = 0.0f;
                flash.scale.w = (since * since) / 8100.0f;
            }
            else if (counter < 0xb4)
            {
                flash.scale.x = 1.0f;
                flash.scale.y = 0.0f;
                flash.scale.z = 0.0f;
                flash.scale.w = 1.0f;
            }
            else if (counter < 0x102)
            {
                if (counter == 0xb4)
                {
                    FUN_10065b10(index, 0x2d2, 1, 0);
                    if (DAT_10c64680 != 0)
                    {
                        Script_ValueCommand(0x7549, 0x4b);
                        roll = DAT_10874848.TestScriptFlag(0x1a4);
                        if (roll != 0)
                        {
                            DAT_10db6ee8[DAT_10874848.TestScriptFlag(0x1a4)].SetPosition(-100000.0f - MODEL.matrix._41, MODEL.matrix._42, MODEL.matrix._43);
                            DAT_10c6356c = 100000;
                            DAT_10c63588 = 7;
                            Script_ValueCommand(0x753f, 0x4b);
                        }
                    }
                }
                counter = W(FIGHTER_WORD_DEFEAT_FRAMES);
                flash.scale.x = 1.0f;
                flash.scale.y = (float)(0x102 - counter) * (float)(0x102 - counter) * 0.004213826f + flash.scale.y;
                flash.scale.z = flash.scale.z - (float)(counter - 200) * (float)(counter - 200) * 0.006584103f;
                flash.scale.w = flash.scale.w - (float)(counter - 0xb4) / 3081.0f;
                if (flash.scale.w < 0.0f)
                    flash.scale.w = 0.0f;
            }
            else
                flash.scale.Fill(0.0f);
        }
        counter = W(FIGHTER_WORD_DEFEAT_FRAMES);
        if (counter > 0x83)
        {
            AnimModel &glow = DAT_10db6ee8[89];
            if (counter < 0xb8)
            {
                float since = (float)(counter - 0x84);
                glow.scale.z = 0.0f;
                glow.scale.x = (float)(0xb8 - counter) * 0.0001451379f + glow.scale.x;
                glow.scale.y = since / 16.0f;
                glow.scale.w = since / 208.0f + 0.25f;
                if (glow.scale.x < 0.0f)
                    glow.scale.x = 0.0f;
            }
            else if (W(11) < 300)
            {
                float since, left;
                if (W(11) == 0xb8)
                    glow.scale.x = 0.3f;
                since = (float)(counter - 0xb8);
                left = (float)(300 - counter);
                glow.scale.x = glow.scale.x - since * 2.947244e-05f;
                glow.scale.y = left * left * 0.0008686249f + glow.scale.y;
                glow.scale.z = since * since * 0.001357226f + glow.scale.z;
                glow.scale.w = (left * left * 0.5f) / 13456.0f;
                if (glow.scale.x < 0.0f)
                    glow.scale.x = 0.0f;
                glow.heading = glow.heading - since * 5e-05f;
                glow.targetHeading = glow.heading;
            }
            else
                glow.scale.Fill(0.0f);
        }
        DAT_10db6ee8[88].m_0294 = 1;
        DAT_10db6ee8[89].m_0294 = 1;
        DAT_10db6ee8[88].motionFrame = MODEL.motionFrame;
        W(FIGHTER_WORD_DEFEAT_FRAMES)++;
        DAT_10db6ee8[89].motionFrame = MODEL.motionFrame;
        DAT_10db6ee8[88].SetPosition(MODEL.matrix._41, MODEL.matrix._42, MODEL.matrix._43);
        if (DAT_104bfe80 < 2)
            DAT_10db6ee8[89].SetPosition(-200000.0f, 0.0f, 0.0f);
        else
            DAT_10db6ee8[89].SetPosition(MODEL.matrix._41, MODEL.matrix._42, MODEL.matrix._43);
        DAT_10db6ee8[89].alpha = DAT_10db6ee8[89].scale.w;
        DAT_10db6ee8[88].alpha = DAT_10db6ee8[88].scale.w;

        defeatPos.Fill(0.0f);
        defeatPos.x += 950.0f;
        defeatPos.y += 200.0f;
        defeatPos.RotateZ(-(FUN_10001f80(DAT_10868818.x - DAT_10da9150.x, DAT_10868818.y - DAT_10da9150.y) + FIGHTER_PI) -
                    (float)(W(FIGHTER_WORD_DEFEAT_FRAMES) + 0x1e) * (float)(W(FIGHTER_WORD_DEFEAT_FRAMES) + 0x1e) * 3e-05f -
                    0.04363f);
        DAT_10edbd48.Copy(MODEL.matrix.GetTranslation());
        DAT_10866d98.Copy(DAT_10edbd48 + defeatPos * 1.3f);
        if (W(FIGHTER_WORD_DEFEAT_FRAMES) < 0xa1)
        {
            float left = (float)(0xa0 - W(FIGHTER_WORD_DEFEAT_FRAMES));
            float ease = (left + 1.0f) * left * 0.5f;
            float keep;
            ease = ease * ease;
            keep = 1.0f - ease / 3.292411e+08f;
            DAT_10866d98.Copy((DAT_10866d98 - DAT_10da9150) * keep + DAT_10da9150);
            DAT_10edbd48.Copy((DAT_10edbd48 - DAT_10868818) * keep + DAT_10868818);
        }
        distance = (DAT_10866d98 - DAT_10edbd48).Length();
        if (distance <= DAT_10255160[index][0])
            DAT_10255160[index][0] = distance;
        else
        {
            DAT_10255160[index][0] = (distance - DAT_10255160[index][0]) * 0.3f + DAT_10255160[index][0];
            defeatPos.Copy(DAT_10866d98 - DAT_10edbd48);
            defeatPos.Normalized3();
            DAT_10866d98.Copy(DAT_10edbd48 + defeatPos * DAT_10255160[index][0]);
        }
        DAT_1020344c = 1.0f;
        DAT_1101dddc = 0.0f;
        return 1;
    }

    if (index == FIGHTER_FIRST_SLOT)
        DAT_1021f9c0[FIGHTER_FIRST_SLOT] = 0;
    roll = DAT_1020b49c[index].value;
    DAT_1101dddc = 150.0f;
    DAT_1020344c = 0.8f;
    DAT_10255a88[index] = 0;
    finished = 0;
    DAT_10218fc0[index] = 0;
    if (roll > 0 && (DAT_10210d30[0x3d] != 0 || DAT_10db6ee8[61].matrix._41 < -50000.0f || DAT_10210d30[0x40] != 0 ||
                     DAT_10db6ee8[64].matrix._41 < -50000.0f))
        DAT_1020b49c[index].value = roll - 1;
    if (DAT_102550e0[index] == 0)
    {
        MODEL.locked = 0;
        DAT_1020b490[index].value = 0;
        roll = index;
    }
    else
    {
        if (index == FIGHTER_FIRST_SLOT && (DAT_1020c314 == 0 || DAT_1020c31c == 0) &&
            DAT_10daaae8[FIGHTER_FIRST_SLOT].stats[STAT_HEALTH] < DAT_10daaae8[FIGHTER_FIRST_SLOT].stats[STAT_MAX_HEALTH] / 2)
            DAT_1020c314 = 1;
        blink = 0;
        if (DAT_1020b494[index].value == 1 && SLOT.stats[STAT_HEALTH] < SLOT.stats[STAT_MAX_HEALTH] / 2 &&
            index == FIGHTER_FIRST_SLOT &&
            (DAT_10210d30[0x3d] > 0 || DAT_10db6ee8[61].matrix._41 < -50000.0f || DAT_10210d30[0x40] > 0 ||
             DAT_10db6ee8[64].matrix._41 < -50000.0f))
        {
            // The first fighter, hurt to half, calls a second fighter in beside the player.
            // Vec4 locals in the original's order; `arenaCentre` and `fighterPos` are set and never read.
            Vec4 player, ring, spot, probe, hit, arenaCentre, fighterPos;
            float radius = 300.0f;
            int tries = 0;
            int side = 0;
            int clone;
            DAT_1020c31c = 0xd2;
            fighterPos.Copy(DAT_10db6ee8[FIGHTER_FIRST_SLOT].matrix.GetTranslation());
            fighterPos.z = 0.0f;
            player.Copy(DAT_10db6ee8[0].matrix.GetTranslation());
            arenaCentre.y = 2850.0f;
            arenaCentre.z = 0.0f;
            for (;;)
            {
                ring.Fill(0.0f);
                switch (side)
                {
                case 0:
                    ring.y = radius;
                    break;
                case 2:
                    ring.x = radius;
                    break;
                case 1:
                case 3:
                    ring.y = -radius;
                }
                tries++;
                side = tries % 4;
                if (side == 0 && tries >= 4)
                    radius += 100.0f;
                ring.RotateZ(-DAT_10db6ee8[0].heading);
                spot.Copy(ring + player);
                probe.Copy(spot);
                spot.z = 1100.0f;
                probe.z = 0.0f;
                if (DAT_10255f08 != 0 && DAT_10255f08->Collide(0, 4, &probe, &spot, &hit) != 0)
                {
                    probe.Copy(spot);
                    spot.x = 0.0f;
                    spot.y = 2850.0f;
                    spot.z = 1100.0f;
                    if ((probe - spot).Length2D() <= 300.0f)
                    {
                        ring.Copy(probe);
                        break;
                    }
                    hit.Copy(probe - spot);
                    hit.Normalized3();
                    hit.Copy(hit * 45.0f);
                    probe.Copy(probe + hit);
                    spot.Copy(probe);
                    probe.z = 0.0f;
                    if (DAT_10255f08->Collide(0, 4, &probe, &spot, &hit) != 0)
                    {
                        ring.Copy(probe);
                        break;
                    }
                }
                if (tries > 10)
                {
                    ring.x = 0.0f;
                    ring.y = 2850.0f;
                    break;
                }
                ring.z = 0.0f;
            }
            ring.z = 1050.0f;
            clone = FUN_10050510(ring.x, ring.y, ring.z, 0, 10);
            if (clone > 0)
            {
                int word;
                for (word = 0; word < 16; word++)
                    DAT_1020b430[clone + 1].words[word] = 0;
                Matrix4 matrix;
                matrix.SetIdentity();
                matrix.SetTranslation(ring);
                ring.Fill(0.0f);
                EffectOptions options;
                options.followOwner = 1;
                FUN_1001d8b0(FIGHTER_FIRST_SLOT, 0x3f4, ring, -1, -1, 0, 0, 0, &matrix, &options);
                FUN_1001d8b0(FIGHTER_FIRST_SLOT, 0x6d1, ring, -1, -1, 0, 0, 0, &matrix, &options);
                FUN_1001d8b0(FIGHTER_FIRST_SLOT, 0x6d1, ring, -1, -1, 0, 0, 0, &matrix, &options);
                FUN_1001d8b0(FIGHTER_FIRST_SLOT, 0x6d1, ring, -1, -1, 0, 0, 0, &matrix, &options);
                FUN_1001d8b0(FIGHTER_FIRST_SLOT, 0x6d2, ring, -1, -1, 0, 0, 0x28, &matrix, &options);
                if (DAT_10db6ee8[clone].root->Find("scop-pt", 0) == 0)
                    FUN_10012450(clone, 1, "scop.it3");
                if (DAT_10db6ee8[clone + FIGHTER_TWIN].root->Find("scop-pt", 0) == 0)
                    FUN_10012450(clone + FIGHTER_TWIN, 1, "scop.it3");
                DAT_10daaae8[clone].stats[STAT_HEALTH] = DAT_10daaae8[clone].stats[STAT_MAX_HEALTH] / 4;
                DAT_10daaae8[clone].stats[3] = (int)((float)DAT_10db03c8 * FIGHTER_CLONE_EFFECT_SCALE + 0.5f);
                DAT_1021f940[clone] = 0x1e;
                DAT_1021f940[FIGHTER_FIRST_SLOT] = 0x1e;
                DAT_1020c314 = 2;
                DAT_10db6ee8[clone].heading =
                    FUN_10001f80(DAT_10db6ee8[0].matrix._41 - DAT_10db6ee8[clone].matrix._41,
                           DAT_10db6ee8[0].matrix._42 - DAT_10db6ee8[clone].matrix._42) - FIGHTER_PI;
                DAT_10db6ee8[clone].targetHeading = DAT_10db6ee8[clone].heading;
                DAT_1020b430[clone].words[FIGHTER_WORD_GUARD_FRAMES] = 0x4b;
                DAT_1020b4a0[clone].value = 0x28;
                DAT_10daaae8[clone].m_0141 = 1;
                DAT_10daaae8[clone].stats[FIGHTER_STAGGER_STAT] = 0;
                DAT_10db6ee8[clone].alpha = 0.0f;
                FUN_10065b10(clone, 0x294, 1, 0);
                blink = 1;
            }
        }
        if (DAT_1020b494[index].value != 0)
            DAT_1020b494[index].value = 2;
        if (DAT_1020b47c[index].value != 0)
        {
            if (DAT_1020b474[index].value != MODEL.motion->layer)
            {
                MODEL.motionFrame = DAT_1020b478[index].value;
                MODEL.PlayMotion(DAT_1020b474[index].value, 1, 0, 1.0f);
                MODEL.motion->layer = DAT_1020b474[index].value;
                MODEL.motion->frame = MODEL.motionFrame;
                MODEL.motion->m_0004 = (float)MODEL.motionFrame;
            }
        }
        else if (DAT_102550e0[index] == 6)
        {
            if (DAT_1020b470[index].value != 0 && DAT_1020b474[index].value != MODEL.motion->layer)
            {
                MODEL.motionFrame = DAT_1020b478[index].value;
                MODEL.PlayMotion(DAT_1020b474[index].value, 1, 0, 1.0f);
                MODEL.motion->layer = DAT_1020b474[index].value;
                MODEL.motion->frame = MODEL.motionFrame;
                MODEL.motion->m_0004 = (float)MODEL.motionFrame;
                MODEL.motion->Advance();
                MODEL.motion->Advance();
                MODEL.motion->Advance();
            }
        }
        else if (DAT_102550e0[index] > 6 && DAT_1020b470[index].value == 0)
        {
            int guard;
            if (STATE_STEP < 6 || STATE_STEP > 0xb)
                guard = FUN_1013f880() % 10 + 1;
            else
            {
                W(FIGHTER_WORD_FRAME) = -2;
                guard = FUN_1013f880() % 5 + 5;
            }
            roll = DAT_1020b490[index].value;
            W(FIGHTER_WORD_GUARD_FRAMES) = guard;
            if ((6 - roll) * 2 < (int)DAT_102550e0[index])
            {
                roll++;
                DAT_1020b490[index].value = roll;
                guard = (6 - roll) * 2;
                DAT_102550e0[index] = guard < 1 ? 0 : (unsigned char)guard;
                if (DAT_102550e0[index] < 7)
                    DAT_102550e0[index] = 7;
            }
            if (DAT_102550e0[index] < DAT_102550e2[index])
                DAT_102550e2[index] = DAT_102550e0[index];
            return 0;
        }
        if ((6 - DAT_1020b490[index].value) * 2 < (int)DAT_102550e0[index])
        {
            DAT_1020b490[index].value++;
            DAT_102550e0[index] = (unsigned char)((6 - DAT_1020b490[index].value) * 2);
            if (DAT_102550e0[index] < 7)
                DAT_102550e0[index] = 7;
        }
        roll = index;
        if (blink != 0)
            FUN_100b9bb0(index, FUN_1013f880() % 3 + 0x31, 0, 0);
        else if (SLOT.held == 0)
        {
            DAT_10874848.TestScriptFlag(FIGHTER_VOICE_FLAG);
            FUN_100b9bb0(index, (FUN_1013f880() & 1) + 0x3c, 0, 0);
        }
    }
    if (DAT_10210b30[index] != 0 || (W(FIGHTER_WORD_HEALTH_MARK) != 0 && W(FIGHTER_WORD_HEALTH_MARK) != SLOT.stats[STAT_HEALTH]))
    {
        int health = SLOT.stats[STAT_HEALTH];
        int mark = W(FIGHTER_WORD_HEALTH_MARK);
        DAT_10210b30[index] = 0;
        if (health < mark)
        {
            W(FIGHTER_WORD_PROTECT) = 200;
            W(FIGHTER_WORD_HEALTH_MARK) = health;
        }
    }
    roll = DAT_1020b488[index].value;
    if (roll > 0)
    {
        if (roll < 0x14)
        {
            SLOT.stats[STAT_HEALTH] += roll;
            DAT_1020b488[index].value = 0;
        }
        else
        {
            SLOT.stats[STAT_HEALTH] += 0x14;
            DAT_1020b488[index].value = roll - 0x14;
        }
        if (DAT_1020b430[index].state < SLOT.stats[STAT_HEALTH])
            SLOT.stats[STAT_HEALTH] = DAT_1020b430[index].state;
        W(FIGHTER_WORD_HEALTH_MARK) = SLOT.stats[STAT_HEALTH];
    }
    DAT_1020b470[index].value = 0;
    DAT_1020b47c[index].value = 0;
    DAT_1020b480[index].value = 0;
    SLOT.m_0141 = 0;
    blink = 0;
    if (index > FIGHTER_FIRST_SLOT)
    {
        roll = DAT_1020b4a0[index].value;
        if (roll < 1)
        {
            if (roll > -0xf)
            {
                MODEL.alpha = (float)(roll - 1) / -15.0f;
                blink = 1;
                DAT_1020b4a0[index].value = roll - 1;
                FUN_1000ec70(0xf, 0.3f, 0.3f, 0.3f, 1.0f);
            }
            else if (roll == -0xf)
            {
                DAT_1020b4a0[index].value = -0x10;
                MODEL.alpha = 1.0f;
                SLOT.m_0141 = 0;
                FUN_1000ec70(0xf, 1.0f, 1.0f, 1.0f, 1.0f);
            }
        }
        else
        {
            MODEL.alpha = 0.0f;
            blink = 1;
            DAT_1020b4a0[index].value = roll - 1;
            FUN_1000ec70(0xf, 0.3f, 0.3f, 0.3f, 1.0f);
        }
    }
    if (DAT_1020b430[index].state == 0)
    {
        // First frame: record the health, load the twin models of the first fighter and find the bones.
        TWIN_SLOT.kind = -CHAR_FIGHTER_KIND;
        DAT_1020b4b0[index].value = 0;
        MODEL.SetPosition(MODEL.matrix._41, MODEL.matrix._42, MODEL.matrix._43);
        TWIN.alpha = 0.0f;
        if (DAT_10c62ed0 == 0x25e)
            Scene_RunCommand(0x753d, 0xc351, "fall_atari_ue");
        DAT_1020b430[index].state = SLOT.stats[STAT_HEALTH];
        W(FIGHTER_WORD_HEALTH_MARK) = SLOT.stats[STAT_HEALTH];
        W(FIGHTER_WORD_STEP) = 0;
        W(2) = 0;
        W(FIGHTER_WORD_FRAME) = 0;
        if (index < FIGHTER_FIRST_SLOT + 1)
        {
            W(FIGHTER_WORD_GUARD_FRAMES) = 0;
            DAT_10db6ee8[88].LoadFile(FUN_1013f8a0("char02ce.it3", 0));
            DAT_10db6ee8[89].LoadFile(FUN_1013f8a0("char02cp.it3", 0));
            DAT_10db6ee8[88].LoadModel(FUN_1013f8a0("mt_nise.it3", 0));
            DAT_10db6ee8[89].LoadModel(FUN_1013f8a0("mt_nise.it3", 0));
            DAT_10db6ee8[88].mode = 0;
            DAT_10db6ee8[89].mode = 0;
            DAT_10db6ee8[88].scale.Fill(0.0f);
            DAT_10db6ee8[89].scale.Fill(0.0f);
            DAT_10db6ee8[88].SetPosition(-200000.0f, 0.0f, 0.0f);
            DAT_10db6ee8[89].SetPosition(-200000.0f, 0.0f, 0.0f);
            DAT_10836c70.ResetFrameTimer();
            DAT_1020b4a0[index].value = -0x10;
        }
        else
            W(FIGHTER_WORD_GUARD_FRAMES) = 0x4b;
        DAT_1020b48c[index].value = SLOT.stats[STAT_VALUE_4];
        DAT_1020b454[index].value = 0;
        DAT_1021bdf8[index][FIGHTER_NODE_BODY] = MODEL.root->Find("scop-pt", 0);
        while (DAT_1021bdf8[index][FIGHTER_NODE_BODY] == 0)
        {
            FUN_10012450(index, 1, "scop.it3");
            DAT_1021bdf8[index][FIGHTER_NODE_BODY] = MODEL.root->Find("scop-pt", 0);
        }
        if (MODEL.root->Find("scop-pt", 0) == 0)
            FUN_10012450(index + FIGHTER_TWIN, 1, "scop.it3");
        DAT_1021bdf8[index][FIGHTER_NODE_LEFT_TOE] = MODEL.root->Find("Bip01 L Toe0", 0);
        DAT_1021bdf8[index][FIGHTER_NODE_LEFT_HAND] = MODEL.root->Find("Bip01 L Hand", 0);
        DAT_1021bdf8[index][FIGHTER_NODE_RIGHT_TOE] = MODEL.root->Find("Bip01 R Toe0", 0);
        DAT_1021bdf8[index][FIGHTER_NODE_FOOTSTEPS] = MODEL.root->Find("Bip01 Footsteps", 0);
        DAT_1021bdf8[index][FIGHTER_NODE_RIGHT_HAND] = MODEL.root->Find("Bip01 R Hand", 0);
    }

    Vec4 player, place, aim, step;
    player.Copy(DAT_10db6ee8[0].matrix.GetTranslation());
    place.Copy(MODEL.matrix.GetTranslation());
    aim.Copy(Vec3_Normalized(&(player - place)));
    distance = (player - place).Length2D();
    step.Fill(0.0f);
    SLOT.m_0141 = 0;

#define FIGHTER_FACE_PLAYER() (MODEL.targetHeading = FUN_10001f80(aim.x, aim.y) + 3.14159f)
#define FIGHTER_STEP_FORWARD(speed) (step.y = (speed), step.RotateZ(FIGHTER_PI - MODEL.heading))
#define FIGHTER_HOLD_MOTION() \
    do \
    { \
        DAT_1020b480[index].value = 1; \
        DAT_1020b470[index].value = 1; \
        if (DAT_102550e0[index] == 0) \
        { \
            if (MODEL.motion->blending == 0) \
            { \
                DAT_1020b474[index].value = MODEL.motion->layer; \
                DAT_1020b478[index].value = MODEL.motionFrame; \
            } \
            else \
            { \
                DAT_1020b474[index].value = 1; \
                DAT_1020b478[index].value = 0x11; \
            } \
        } \
    } while (0)

    switch (STATE_STEP)
    {
    case 0:
        // Step 0: the back-step. Hops backwards with the first frames of the motion.
        if (MODEL.m_238c == 1)
        {
            if (W(FIGHTER_WORD_SIDE) == 0)
            {
                W(FIGHTER_WORD_SIDE) = 1;
                STATE_FRAME = 0;
                FIGHTER_FACE_PLAYER();
            }
        }
        else if (W(FIGHTER_WORD_SIDE) == 0)
        {
            motion = 1;
            motionSnap = 1;
            break;
        }
        if (MODEL.m_238c != 1)
        {
            FIGHTER_STEP_FORWARD(-24.0f);
            MODEL.targetHeading = MODEL.heading;
        }
        if (STATE_FRAME == 3)
        {
            MODEL.m_238c = 1;
            MODEL.AddMotionSpeed(48.0f);
            FUN_10065b10(index, 0x182, 1, 0);
        }
        if (STATE_FRAME < 0x25)
        {
            motion = 4;
            break;
        }
        motion = 1;
        if (MODEL.m_238c == 1)
            finished = 1;
        break;
    case 1:
        if (STATE_FRAME == 0)
            W(FIGHTER_WORD_SIDE) = FUN_1013f880() & 1;
        aim.Copy(Vec3_Normalized(&(place - player)));
        aim.RotateZ(W(FIGHTER_WORD_SIDE) == 0 ? -0.0873f : 0.0873f);
        if (distance > 320.0f)
            distance = 320.0f;
        else if (distance < 300.0f)
            distance = 300.0f;
        step.Copy(player + aim * distance - place);
        step.z = 0.0f;
        step.Copy(step.Normalized3() * 24.0f);
        MODEL.targetHeading = FUN_10001f80(step.x, step.y) + 3.14159f;
        motion = 0;
        if (((STATE_FRAME < 0x3d || index != FIGHTER_FIRST_SLOT) && STATE_FRAME < 0x5b) || FUN_1013f880() % 3 != 0)
        {
            if (W(FIGHTER_WORD_BLOCKED) == 0)
                break;
        }
        finished = 1;
        break;
    case 2:
        // Step 2: the slow lunge forward.
        if (MODEL.m_238c == 1)
        {
            if (W(FIGHTER_WORD_SIDE) == 0)
            {
                W(FIGHTER_WORD_SIDE) = 1;
                STATE_FRAME = 0;
                FIGHTER_FACE_PLAYER();
            }
        }
        else if (W(FIGHTER_WORD_SIDE) == 0)
        {
            motion = 1;
            break;
        }
        if (MODEL.m_238c != 1)
        {
            FIGHTER_STEP_FORWARD(16.0f);
            MODEL.targetHeading = MODEL.heading;
        }
        if (STATE_FRAME == 6)
        {
            MODEL.m_238c = 1;
            MODEL.AddMotionSpeed(48.0f);
            FUN_10065b10(index, 0x182, 1, 0);
        }
        if (STATE_FRAME < 0x28)
        {
            motion = 3;
            break;
        }
        motion = 1;
        if (MODEL.m_238c == 1)
            finished = 1;
        break;
    case 3:
        // Step 3: walks towards the player until it is far or blocked.
        MODEL.targetHeading = FUN_10001f80(aim.x, aim.y);
        FIGHTER_STEP_FORWARD(22.0f);
        motion = 0;
        if (distance > 700.0f || W(FIGHTER_WORD_BLOCKED) != 0)
            finished = 1;
        break;
    case 4:
        // Step 4: backs away from the player until it is near or blocked.
        MODEL.targetHeading = FUN_10001f80(aim.x, aim.y) + 3.14159f;
        FIGHTER_STEP_FORWARD(22.0f);
        motion = 0;
        if (W(FIGHTER_WORD_BLOCKED) != 0 || distance < 200.0f)
            finished = 1;
        break;
    case 5:
        // Step 5: the fast lunge forward.
        if (MODEL.m_238c == 1)
        {
            if (W(FIGHTER_WORD_SIDE) == 0)
            {
                W(FIGHTER_WORD_SIDE) = 1;
                STATE_FRAME = 0;
                FIGHTER_FACE_PLAYER();
            }
        }
        else if (W(FIGHTER_WORD_SIDE) == 0)
        {
            motion = 1;
            break;
        }
        if (MODEL.m_238c != 1)
        {
            FIGHTER_STEP_FORWARD(32.0f);
            MODEL.targetHeading = MODEL.heading;
        }
        if (STATE_FRAME == 6)
        {
            MODEL.m_238c = 1;
            MODEL.AddMotionSpeed(48.0f);
            FUN_10065b10(index, 0x182, 1, 0);
        }
        if (STATE_FRAME < 0x28)
        {
            motion = 3;
            break;
        }
        motion = 1;
        if (MODEL.m_238c == 1)
            finished = 1;
        break;
    case 6:
    {
        // Step 6: the first slash (the swing effect starts on frame 6).
        int c = STATE_FRAME;
        DAT_1020b484[index].value = 1;
        if (c == 3)
            Fighter_Grunt(index);
        motion = 5;
        if (c < 6)
            MODEL.targetHeading = FUN_10001f80(aim.x, aim.y) + 3.14159f;
        if (c >= 7 && c < 0x19)
            FIGHTER_STEP_FORWARD(12.0f);
        if (c >= 0x1d)
            motion = -1;
        if (c == 6)
        {
            Fighter_Swing(index, 0x434, FIGHTER_SWING_SIDE, DAT_1021bdf8[index][FIGHTER_NODE_BODY],
                          DAT_1021bdf8[index][FIGHTER_NODE_BODY], 0);
            break;
        }
        if (c == 5)
        {
            DAT_10218fc0[index] = 1;
            MODEL.heading = FUN_10001f80(aim.x, aim.y) + 3.14159f;
            break;
        }
        if (c >= 0x1d)
            finished = 1;
        DAT_10218fc0[index] = 1;
        break;
    }
    case 7:
    {
        // Step 7: the long slash (the swing effect starts on frame 0x13).
        int c = STATE_FRAME;
        DAT_1020b484[index].value = 1;
        if (c == 3)
            Fighter_Grunt(index);
        motion = 6;
        if (c < 6)
            MODEL.targetHeading = FUN_10001f80(aim.x, aim.y) + 3.14159f;
        if (c >= 7 && c < 0x2a)
            FIGHTER_STEP_FORWARD(12.0f);
        if (c >= 0x2c)
            motion = -1;
        if (c == 0x13)
        {
            Fighter_Swing(index, 0x434, FIGHTER_SWING_SIDE, DAT_1021bdf8[index][FIGHTER_NODE_BODY],
                          DAT_1021bdf8[index][FIGHTER_NODE_BODY], 0);
            break;
        }
        if (c == 5)
        {
            DAT_10218fc0[index] = 1;
            MODEL.heading = FUN_10001f80(aim.x, aim.y) + 3.14159f;
            break;
        }
        if (c >= 0x2c)
            finished = 1;
        DAT_10218fc0[index] = 1;
        break;
    }
    case 8:
    {
        // Step 8: the kick with the left foot.
        int c = STATE_FRAME;
        DAT_1020b484[index].value = 1;
        if (c == 3)
            Fighter_Grunt(index);
        motion = 7;
        if (c < 6)
            MODEL.targetHeading = FUN_10001f80(aim.x, aim.y) + 3.14159f;
        if (c < 0x10)
            FIGHTER_STEP_FORWARD(12.0f);
        if (c >= 0x1d)
            motion = -1;
        if (c == 0xc)
        {
            Fighter_Swing(index, FIGHTER_SWING_EFFECT, FIGHTER_SWING_OFF, DAT_1021bdf8[index][FIGHTER_NODE_LEFT_TOE],
                          DAT_1021bdf8[index][FIGHTER_NODE_LEFT_TOE], 0x181);
            break;
        }
        if (c == 5)
        {
            DAT_10218fc0[index] = 1;
            MODEL.heading = FUN_10001f80(aim.x, aim.y) + 3.14159f;
            break;
        }
        if (c >= 0x1d)
            finished = 1;
        DAT_10218fc0[index] = 1;
        break;
    }
    case 9:
    {
        // Step 9: the punch with the left hand.
        int c = STATE_FRAME;
        DAT_1020b484[index].value = 1;
        if (c == 3)
            Fighter_Grunt(index);
        motion = 8;
        if (c < 5)
            MODEL.targetHeading = FUN_10001f80(aim.x, aim.y) + 3.14159f;
        if (c < 0xf)
            FIGHTER_STEP_FORWARD(12.0f);
        if (c >= 0xe)
            motion = -1;
        if (c == 5)
        {
            Fighter_Swing(index, FIGHTER_SWING_EFFECT, FIGHTER_SWING_OFF, DAT_1021bdf8[index][FIGHTER_NODE_BODY],
                          DAT_1021bdf8[index][FIGHTER_NODE_LEFT_HAND], 0x180);
            break;
        }
        if (c == 4)
        {
            DAT_10218fc0[index] = 1;
            MODEL.heading = FUN_10001f80(aim.x, aim.y) + 3.14159f;
            break;
        }
        if (c >= 0xe)
            finished = 1;
        DAT_10218fc0[index] = 1;
        break;
    }
    case 0xa:
    {
        // Step 0xa: the three-hit combo (grunt and swing on frames 1, 8 and 0x10).
        int c = STATE_FRAME;
        DAT_1020b484[index].value = 1;
        motion = 9;
        if (c < 5)
            MODEL.targetHeading = FUN_10001f80(aim.x, aim.y) + 3.14159f;
        if (c < 0x16)
            FIGHTER_STEP_FORWARD(12.0f);
        if (c >= 0x16)
            motion = -1;
        if (c == 8 || c == 1 || c == 0x10)
        {
            Fighter_Grunt(index);
            Fighter_Swing(index, FIGHTER_SWING_EFFECT, c == 8 ? FIGHTER_SWING_SIDE : 0,
                          DAT_1021bdf8[index][FIGHTER_NODE_BODY], DAT_1021bdf8[index][FIGHTER_NODE_BODY],
                          c == 1 ? 0x17d : c == 8 ? 0x17e : 0x17f);
            break;
        }
        if (c == 5)
        {
            DAT_10218fc0[index] = 1;
            MODEL.heading = FUN_10001f80(aim.x, aim.y) + 3.14159f;
            break;
        }
        if (c >= 0x16)
            finished = 1;
        DAT_10218fc0[index] = 1;
        break;
    }
    case 0xb:
    {
        // Step 0xb: the double kick (left foot on frame 6, right foot on frame 0xe).
        int c = STATE_FRAME;
        DAT_1020b484[index].value = 1;
        if (c == 1 || c == 0xf)
            Fighter_Grunt(index);
        motion = 10;
        if (c < 6)
            MODEL.targetHeading = FUN_10001f80(aim.x, aim.y) + 3.14159f;
        if (c >= 7 && c < 0x1e)
            FIGHTER_STEP_FORWARD(12.0f);
        if (c >= 0x2c)
            motion = -1;
        if (c == 6)
        {
            Fighter_Swing(index, FIGHTER_SWING_EFFECT, FIGHTER_SWING_OFF, DAT_1021bdf8[index][FIGHTER_NODE_LEFT_TOE],
                          DAT_1021bdf8[index][FIGHTER_NODE_LEFT_TOE], 0x17b);
            break;
        }
        if (c == 0xe)
        {
            Fighter_Swing(index, FIGHTER_SWING_EFFECT, FIGHTER_SWING_OFF, DAT_1021bdf8[index][FIGHTER_NODE_RIGHT_TOE],
                          DAT_1021bdf8[index][FIGHTER_NODE_RIGHT_TOE], 0x17c);
            break;
        }
        if (c == 5)
        {
            DAT_10218fc0[index] = 1;
            MODEL.heading = FUN_10001f80(aim.x, aim.y) + 3.14159f;
            break;
        }
        if (c >= 0x2c)
            finished = 1;
        DAT_10218fc0[index] = 1;
        break;
    }
    case 0xc:
    {
        // Step 0xc: the first ranged shot (hand effect on frame 1, hold, finish on frame 0xaa).
        int c;
        Node *shotNode = DAT_1021bdf8[index][FIGHTER_NODE_RIGHT_HAND];
        MODEL.locked = 1;
        DAT_1020b484[index].value = 1;
        FIGHTER_HOLD_MOTION();
        MODEL.targetHeading = FUN_10001f80(aim.x, aim.y) + 3.14159f;
        c = STATE_FRAME;
        if (c == 1)
        {
            Vec4 shotPos;
            Vec4 shotHand;
            EffectOptions shotOptions;
            Matrix4 shotMatrix;
            shotMatrix.SetIdentity();
            motion = 0xe;
            motionSnap = 1;
            shotPos.Fill(0.0f);
            shotHand.Copy(shotNode->worldMatrix.GetTranslation());
            shotHand.z += 35.0f;
            shotMatrix = RotationZ(MODEL.heading);
            shotMatrix.SetTranslation(shotHand);
            shotOptions.m_0004 = SLOT.stats[3];
            shotOptions.followOwner = 1;
            shotOptions.m_0008 = 1.35f;
            shotOptions.m_0000 = 0;
            shotOptions.m_000e = 1;
            shotOptions.owner = index;
            FUN_1001d8b0(index, 0x427, shotPos, -1, -1, 0, 0, 0, &shotMatrix, &shotOptions);
            FUN_100b9bb0(index, 0x38, 0, 0);
        }
        else if (c == 0x37)
        {
            motion = 0xf;
            motionSnap = 1;
        }
        else if (c < 0x38 || c > 0x95)
        {
            if (c == 0x96)
                motion = 1;
            else if (c > 0xa9)
                finished = 1;
        }
        else
            motion = 0xf;
        if (STATE_FRAME > 0 && STATE_FRAME < 0x96)
            SLOT.m_0141 = 1;
        break;
    }
    case 0xd:
    {
        // Step 0xd: the rain of shots from above (an effect every 10 frames from 0x3d to 0x78, then the big shot).
        int c;
        Node *shotNode = DAT_1021bdf8[index][FIGHTER_NODE_RIGHT_HAND];
        DAT_1020b484[index].value = 1;
        FIGHTER_HOLD_MOTION();
        MODEL.targetHeading = FUN_10001f80(aim.x, aim.y) + 3.14159f;
        c = STATE_FRAME;
        if (c < 0x79 && c > 0x3c && c % 10 == 0)
        {
            Vec4 rainPos;
            Vec4 spot;
            EffectOptions rainOptions;
            Matrix4 rainMatrix;
            rainMatrix.SetIdentity();
            rainPos.Fill(0.0f);
            spot.Copy(place);
            spot.z += (float)(FUN_1013f880() % 100) + 50.0f;
            spot.x += (float)(FUN_1013f880() % 300) - 150.0f;
            spot.y += (float)(FUN_1013f880() % 300) - 150.0f;
            rainMatrix = RotationX(-0.0525f) * RotationZ(FUN_10001f80(player.x - spot.x, player.y - spot.y) + FIGHTER_PI);
            rainMatrix.SetTranslation(spot);
            rainOptions.m_0004 = SLOT.stats[3];
            rainOptions.followOwner = 1;
            rainOptions.m_0008 = 1.44f;
            rainOptions.m_0000 = 0;
            rainOptions.m_000e = 2;
            rainOptions.owner = index;
            FUN_1001d8b0(index, 0x429, rainPos, -1, -1, 0, 0, 0, &rainMatrix, &rainOptions);
        }
        c = STATE_FRAME;
        if (c == 1)
        {
            Vec4 shotPos;
            Vec4 shotHand;
            EffectOptions shotOptions;
            Matrix4 shotMatrix;
            shotMatrix.SetIdentity();
            motion = 0x13;
            motionSnap = 1;
            shotPos.Fill(0.0f);
            shotHand.Copy(shotNode->worldMatrix.GetTranslation());
            shotHand.z += 19.0f;
            shotMatrix = RotationZ(MODEL.heading);
            shotMatrix.SetTranslation(shotHand);
            shotOptions.m_0004 = 0;
            shotOptions.followOwner = 1;
            shotOptions.m_0000 = 0;
            shotOptions.m_000e = 2;
            shotOptions.owner = index;
            FUN_1001d8b0(index, 0x42c, shotPos, -1, -1, 0, 0, 0, &shotMatrix, &shotOptions);
            FUN_100b9bb0(index, 0x3a, 0, 0);
        }
        else if (c == 0x4f)
        {
            motion = 0x14;
            motionSnap = 1;
        }
        else if (c < 0x50 || c > 0xb3)
        {
            if (c == 0xb4)
                motion = 1;
            else if (c > 199)
                finished = 1;
        }
        else
            motion = 0x14;
        if (STATE_FRAME > 0 && STATE_FRAME < 0xb5)
            SLOT.m_0141 = 1;
        break;
    }
    case 0xe:
    {
        // Step 0xe: the charged shot (charge on frame 0, shot on frame 0xf, the second shot on frame 0x3c).
        int c;
        Node *shotNode = DAT_1021bdf8[index][FIGHTER_NODE_RIGHT_HAND];
        MODEL.locked = 1;
        DAT_1020b484[index].value = 1;
        FIGHTER_HOLD_MOTION();
        MODEL.targetHeading = FUN_10001f80(aim.x, aim.y) + 3.14159f;
        c = STATE_FRAME;
        if (c == 0)
        {
            motion = 0x10;
            motionSnap = 1;
        }
        else if (c == 0xf)
        {
            Vec4 shotPos;
            Vec4 shotHand;
            EffectOptions shotOptions;
            Matrix4 shotMatrix;
            shotMatrix.SetIdentity();
            shotPos.Fill(0.0f);
            shotHand.Copy(shotNode->worldMatrix.GetTranslation());
            shotHand.z += 19.0f;
            shotMatrix = RotationZ(MODEL.heading);
            shotMatrix.SetTranslation(shotHand);
            shotOptions.m_0004 = 0;
            shotOptions.followOwner = 1;
            shotOptions.m_0000 = 0;
            shotOptions.m_000e = 3;
            shotOptions.owner = index;
            FUN_1001d8b0(index, 0x420, shotPos, -1, -1, 0, 0, 0, &shotMatrix, &shotOptions);
            FUN_100b9bb0(index, 0x39, 0, 0);
        }
        else if (c == 0x3c)
        {
            Vec4 shotPos;
            Vec4 shotHand;
            EffectOptions shotOptions;
            Matrix4 shotMatrix;
            shotMatrix.SetIdentity();
            motion = 0x12;
            motionSnap = 1;
            shotPos.Fill(0.0f);
            shotHand.Copy(shotNode->worldMatrix.GetTranslation());
            shotHand.z += 19.0f;
            shotMatrix = RotationZ(MODEL.heading);
            shotMatrix.SetTranslation(shotHand);
            shotOptions.m_0004 = SLOT.stats[3];
            shotOptions.followOwner = 1;
            shotOptions.m_0008 = 1.87f;
            shotOptions.m_0000 = 0;
            shotOptions.m_000e = 3;
            shotOptions.owner = index;
            FUN_1001d8b0(index, 0x41a, shotPos, -1, -1, 0, 0, 0, &shotMatrix, &shotOptions);
        }
        else if (c == 0x2d)
        {
            motion = 0x11;
            motionSnap = 1;
        }
        else if (c < 0x3d || c > 0x9a)
        {
            if (c == 0x9b)
                motion = 1;
            else if (c > 0xae)
                finished = 1;
        }
        else
            motion = 0x12;
        c = STATE_FRAME;
        if (c >= 0xe && c <= 0x9a)
            SLOT.m_0141 = 1;
        else if (c < 0xf)
            DAT_10218fc0[index] = 1;
        break;
    }
    case 0xf:
        DAT_10868838[index][0] = DAT_1021bdf8[index][FIGHTER_NODE_FOOTSTEPS]->worldMatrix;
        finished = 1;
        break;
    case 0x10:
    {
        // Step 0x10: the heavy shot that knocks the player back (effect on frame 1, knock-back on frame 0x15).
        int c;
        Node *shotNode = DAT_1021bdf8[index][FIGHTER_NODE_RIGHT_HAND];
        MODEL.locked = 1;
        DAT_1020b484[index].value = 1;
        FIGHTER_HOLD_MOTION();
        MODEL.targetHeading = FUN_10001f80(aim.x, aim.y) + 3.14159f;
        c = STATE_FRAME;
        if (c == 1)
        {
            Vec4 shotPos;
            Vec4 shotHand;
            EffectOptions shotOptions;
            Matrix4 shotMatrix;
            shotMatrix.SetIdentity();
            motion = 0x17;
            motionSnap = 1;
            shotPos.Fill(0.0f);
            shotHand.Copy(shotNode->worldMatrix.GetTranslation());
            shotHand.z += 20.0f;
            shotMatrix = RotationZ(MODEL.heading);
            shotMatrix.SetTranslation(shotHand);
            shotOptions.m_0004 = SLOT.stats[3];
            shotOptions.followOwner = 1;
            shotOptions.m_0008 = 2.3f;
            shotOptions.m_0000 = 0;
            shotOptions.m_000e = 1;
            shotOptions.owner = index;
            FUN_1001d8b0(index, 0x431, shotPos, -1, -1, 0, 0, 0, &shotMatrix, &shotOptions);
            FUN_100b9bb0(index, 0x3b, 0, 0);
        }
        else if (c == 0x15)
        {
            FUN_1016b230("82919191918264");
            FUN_10055ee0();
            motion = 0x18;
            motionSnap = 1;
        }
        else if (c < 0x20 || c > 0xa0)
        {
            if (c == 0xa1)
                motion = 1;
            else if (c > 0xb4)
                finished = 1;
        }
        else
            motion = 0x18;
        if (STATE_FRAME > 0 && STATE_FRAME < 0xa0)
            SLOT.m_0141 = 1;
        break;
    }
    case 0x11:
    {
        // Step 0x11: the dash. It slides with a speed that grows towards the player and ends in a burst.
        int c;
        float ease;
        DAT_1020b498[index].value = 0;
        DAT_1020b484[index].value = 1;
        FIGHTER_HOLD_MOTION();
        DAT_1020b47c[index].value = 1;
        c = STATE_FRAME;
        if (c == 0)
        {
            motionSnap = 1;
            motion = 0xb;
            DAT_10255160[index][0] = 0.0f;
            DAT_10255160[index][1] = 0.0f;
            FUN_100b9bb0(index, 0x37, 0, 0);
        }
        else if (c == 0xf)
        {
            Vec4 dashPos;
            Vec4 dashHand;
            EffectOptions dashOptions;
            Matrix4 dashMatrix;
            dashMatrix.SetIdentity();
            motion = 0xc;
            motionSnap = 1;
            dashPos.Fill(0.0f);
            dashHand.Copy(DAT_1021bdf8[index][FIGHTER_NODE_FOOTSTEPS]->worldMatrix.GetTranslation());
            dashHand.z += 20.0f;
            dashMatrix = RotationZ(MODEL.heading);
            dashMatrix.SetTranslation(dashHand);
            dashOptions.m_0004 = SLOT.stats[3];
            dashOptions.followOwner = 1;
            dashOptions.m_0008 = 1.98f;
            dashOptions.m_0000 = 0;
            dashOptions.m_000e = 5;
            dashOptions.owner = index;
            FUN_1001d8b0(index, 0x435, dashPos, -1, -1, &DAT_10868838[index][0], 0, 0, &dashMatrix, &dashOptions);
            dashMatrix = DAT_1021bdf8[index][FIGHTER_NODE_BODY]->worldMatrix;
            FUN_1001d8b0(index, 0x443, dashPos, -1, -1, &DAT_10868838[index][1], 0, 0, &dashMatrix, &dashOptions);
            DAT_102028d4 = 0;
        }
        else if (c < 0xf0)
        {
            motion = 0xc;
            motionSnap = 1;
            if (DAT_102028d4 > 0xed)
                STATE_FRAME = 0xef;
        }
        else if (c == 0xf0)
        {
            motion = 0xd;
            motionSnap = 1;
            DAT_102028d4 = -1;
        }
        else if (c == 0xff)
            motion = 1;
        else if (c > 0x11c)
            finished = 1;
        ease = DAT_10255160[index][0];
        if (player.x > place.x && STATE_FRAME <= 0xe0 && STATE_FRAME >= 0x1f)
            ease = ease >= 0.0f ? ease + 0.3f : ease + 0.9f;
        else if (place.x > player.x && STATE_FRAME <= 0xe0 && STATE_FRAME >= 0x1f)
            ease = ease <= 0.0f ? ease - 0.3f : ease - 0.9f;
        else
            ease = ease * 0.9f;
        DAT_10255160[index][0] = ease;
        ease = DAT_10255160[index][1];
        if (player.y > place.y && STATE_FRAME <= 0xe0 && STATE_FRAME >= 0x1f)
            ease = ease >= 0.0f ? ease + 0.4f : ease + 1.2f;
        else if (place.y > player.y && STATE_FRAME <= 0xe0 && STATE_FRAME >= 0x1f)
            ease = ease <= 0.0f ? ease - 0.4f : ease - 1.2f;
        else
            ease = ease * 0.9f;
        DAT_10255160[index][1] = ease;
        if (DAT_10255160[index][0] > 13.0f)
            DAT_10255160[index][0] = 13.0f;
        else if (DAT_10255160[index][0] < -13.0f)
            DAT_10255160[index][0] = -13.0f;
        if (DAT_10255160[index][1] > 13.0f)
            DAT_10255160[index][1] = 13.0f;
        else if (DAT_10255160[index][1] < -13.0f)
            DAT_10255160[index][1] = -13.0f;
        step.x = DAT_10255160[index][0];
        step.y = DAT_10255160[index][1];
        DAT_10868838[index][0] = DAT_1021bdf8[index][FIGHTER_NODE_FOOTSTEPS]->worldMatrix;
        DAT_10868838[index][0]._43 += 30.0f;
        DAT_10868838[index][1] = DAT_1021bdf8[index][FIGHTER_NODE_BODY]->worldMatrix;
        if (STATE_FRAME < 0xd)
            DAT_10218fc0[index] = 1;
        break;
    }
    case 0x12:
        STATE_FRAME++;
        motion = 2;
        if (STATE_FRAME < 0x97 && DAT_10874848.TestScriptFlag(0x1c7) != 2)
            break;
        finished = 1;
        break;
    case 0x15:
        SLOT.m_0141 = 1;
        roll = STATE_FRAME;
        if (roll < 0xf)
            MODEL.alpha = (float)(0xf - roll) / 15.0f;
        else if (roll == 0xf)
        {
            motion = 2;
            aim.Set(0.0f, 2850.0f, 1100.0f, 0.0f);
            MODEL.SetPosition((aim - player).Length2D() <= 100.0f ? -300.0f : 0.0f, 2850.0f, 1100.0f);
            SLOT.stats[FIGHTER_STAGGER_STAT] = 0;
            DAT_10daaa80.RemoveOwnedBy(index);
            MODEL.alpha = 0.0f;
            FUN_100b9bb0(index, 0x3e, 0, 1);
        }
        else
        {
            motion = 2;
            if (roll > 0x1d)
            {
                MODEL.alpha = 1.0f;
                SLOT.stats[FIGHTER_STAGGER_STAT] = 0;
                SLOT.m_0141 = 0;
                STATE_STEP = 0x13;
                finished = 1;
            }
            else
                MODEL.alpha = (float)(roll - 0xf) / 15.0f;
        }
        break;
    default:
        if (index == FIGHTER_FIRST_SLOT || DAT_10874848.TestScriptFlag(0x1c7) == 2)
            finished = 1;
        break;
    }

    if (W(FIGHTER_WORD_GUARD_FRAMES) < 1)
    {
        int healthPercent;
        int mood;
        if (finished == 2)
            goto pick_random_step;
        if (finished == 0)
            goto move_and_animate;
        healthPercent = (SLOT.stats[STAT_HEALTH] * 100) / DAT_1020b430[index].state;
        DAT_1020b498[index].value++;
        roll = STATE_STEP;
        DAT_1020b480[index].value = 0;
        STATE_FRAME = -1;
        W(FIGHTER_WORD_SIDE) = 0;
        DAT_1020b454[index].value = 0;
        if (roll < 6)
            category = 0;
        else if (roll < 0xc)
            category = 1;
        else
            category = (roll == 0x12) + 2;
        roll = FUN_1013f880() % 1000;
        if (healthPercent < 100)
        {
            if (healthPercent < 0x32)
            {
                mood = roll < 200 ? 1 : roll < 0x1c2 ? 2 : 0;
                if (category == 0)
                {
                    if (distance >= FIGHTER_NEAR_RANGE)
                    {
                        if (FUN_1013f880() % 3 != 0)
                        {
                            Fighter_RandomStrike(index, 0xb);
                            goto after_choice;
                        }
                        goto strike_or_chase;
                    }
                    roll = FUN_1013f880();
                    if (roll % 10 < 8 && (mood != 2 || FUN_1013f880() % 3 == 1))
                        goto circle_step;
                    if (roll % 10 < 7)
                        Fighter_RandomStrike(index, 0xb);
                    else
                        STATE_STEP = FUN_1013f880() % 6;
                }
                else if (category == 1 || category == 2)
                {
                    if (mood == 0)
                    {
                        if (distance < FIGHTER_NEAR_RANGE)
                            goto approach_step;
                    }
                    else
                    {
                        if (mood != 1)
                        {
                            if (mood == 2)
                                Fighter_RandomStrike(index, 0xb);
                            goto after_choice;
                        }
                        if (distance < 500.0f || FUN_1013f880() % 5 == 0)
                            goto circle_step;
                    }
                    STATE_STEP = (FUN_1013f880() & 1) + 4;
                    if (STATE_STEP == 5 && (FUN_1013f880() & 1) != 0)
                        STATE_STEP = 2;
                }
            }
            else
            {
                if (roll < 0x15e)
                    mood = 2;
                else
                {
                    mood = 0;
                    if (roll > 0x351)
                        mood = (roll > 0x3dd) * 2 + 1;
                }
                if (distance < 200.0f)
                {
                    // Too close: look behind it for room to back off.
                    Vec4 spot, from, to, hit;
                    int gapBehind = 0;
                    DAT_11b002c0 = MODEL.collisionFilter;
                    spot.Copy(place);
                    to.Set(0.0f, 200.0f, 0.0f, 0.0f);
                    to.RotateZ(FIGHTER_PI - FUN_10001f80(aim.x, aim.y));
                    from.Copy(place + to);
                    from.z = 1500.0f;
                    spot.z = 0.0f;
                    if (DAT_10255f08 == 0 || DAT_10255f08->Collide(0, 0x4000000, &from, &spot, &hit) == 0)
                        gapBehind = 1;
                    from.Copy(place + to);
                    from.z = 10.0f;
                    if (gapBehind != 0 || DAT_10255f08 == 0 ||
                        DAT_10255f08->RayCast(0, 4, 0x4000000, &spot, &from, &hit, 0) != 0)
                    {
                        to.Set(0.0f, -400.0f, 0.0f, 0.0f);
                        to.RotateZ(FIGHTER_PI - FUN_10001f80(aim.x, aim.y));
                        from.Copy(place + to);
                        if (DAT_10255f08 == 0 || DAT_10255f08->RayCast(0, 4, 0x4000000, &spot, &from, &hit, 0) == 0)
                        {
                            STATE_STEP = FIGHTER_STEP_JUMP_BACK;
                            mood = 4;
                        }
                        else if (FUN_1013f880() % 1000 < 5)
                        {
                            STATE_STEP = 0x11;
                            mood = 4;
                        }
                        else
                            mood = 1;
                    }
                    else
                    {
                        STATE_STEP = (FUN_1013f880() & 1) << 2;
                        mood = 4;
                    }
                    DAT_11b002c0 = 0;
                }
                if (mood == 3 || mood == 2)
                {
                    do
                    {
                        if (DAT_1020b498[index].value < 0x15)
                            roll = FUN_1013f880() % 5 + FIGHTER_STEP_STRIKE_FIRST;
                        else
                            roll = FUN_1013f880() % 6 + FIGHTER_STEP_STRIKE_FIRST;
                        STATE_STEP = roll;
                    } while (roll == 0xf);
                }
                else if (mood == 1)
                {
                    if (distance < 300.0f)
                        goto circle_step;
                    if (distance >= 500.0f)
                        STATE_STEP = (FUN_1013f880() & 1) + 4;
                    else
                    {
                        STATE_STEP = (FUN_1013f880() & 7) + 4;
                        if (STATE_STEP > 5)
                            goto after_choice;
                    }
                    if (STATE_STEP == 5 && (FUN_1013f880() & 1) != 0)
                        STATE_STEP = 3;
                }
                else if (mood == 0)
                {
                strike_or_chase:
                    STATE_STEP = FUN_1013f880() % 6;
                }
            }
        }
        else
        {
            int style = roll < 300 ? 1 : roll < 800 ? 0 : 2;
            if (category == 0 || category == 2)
            {
                if (distance < 300.0f)
                    goto circle_step;
                if (distance >= 500.0f)
                {
                    roll = FUN_1013f880() % 100;
                    STATE_STEP = roll;
                    if (roll < 0x51)
                    {
                        if ((FUN_1013f880() & 1) == 0)
                            STATE_STEP = 4;
                        else
                            STATE_STEP = (FUN_1013f880() & 1) != 0 ? 2 : 5;
                    }
                    else
                        Fighter_RandomStrike(index, 0x1f);
                }
                else if (style == 0)
                    STATE_STEP = FUN_1013f880() % 6;
                else
                {
                    if (style == 1)
                        goto circle_step;
                    Fighter_RandomStrike(index, 0x1f);
                }
            }
            else if (style == 0)
            {
            approach_step:
                STATE_STEP = FUN_1013f880() % 3;
                if (STATE_STEP == 2)
                    STATE_STEP = 3;
            }
            else
            {
            circle_step:
                STATE_STEP = FUN_1013f880() % 6 + 6;
            }
        }
    after_choice:
        if (index != FIGHTER_FIRST_SLOT && category != 3 && (category == 2 || FUN_1013f880() % 0x32 < 0x14))
        {
            STATE_STEP = FIGHTER_STEP_GUARD;
            W(FIGHTER_WORD_GUARD_FRAMES) = 0x1e;
        }
        if (W(FIGHTER_WORD_PREVIOUS_STEP) == STATE_STEP)
            motion = 1;
        W(FIGHTER_WORD_PREVIOUS_STEP) = STATE_STEP;
    }
    else
    {
        W(FIGHTER_WORD_GUARD_FRAMES)--;
        STATE_STEP = 0x13;
        STATE_FRAME = 0;
        motion = 1;
        if (W(FIGHTER_WORD_GUARD_FRAMES) == 0 && MODEL.m_0294 == 0)
        {
        pick_random_step:
            DAT_1020b480[index].value = 0;
            STATE_FRAME = -1;
            DAT_1020b454[index].value = 0;
            W(7) = 0;
            do
                STATE_STEP = FUN_1013f880() % 0xc;
            while (STATE_STEP == 4);
            DAT_1020b498[index].value++;
        }
    }
move_and_animate:
    step.z += 30.0f;
    STATE_FRAME++;
    if (MODEL.IsClearOfMapLimited(&step) == 0 || (STATE_STEP < 5 && MODEL.IsClearAny(&step) == 0))
    {
        W(FIGHTER_WORD_BLOCKED) = 1;
        step.z -= 30.0f;
    }
    else
    {
        Vec4 ahead, ground, hit;
        step.z -= 30.0f;
        ahead.Copy(place + step * 15.0f);
        ahead.z = 1500.0f;
        ground.Copy(ahead);
        ground.z = 0.0f;
        if (DAT_10255f08 == 0 || DAT_10255f08->Collide(0, 0x4000000, &ahead, &ground, &hit) == 0)
        {
            W(FIGHTER_WORD_BLOCKED) = 1;
            step.Copy(step * 0.4f);
        }
        else
            W(FIGHTER_WORD_BLOCKED) = 0;
    }
    if (DAT_104bfdf0 == 0 || DAT_104c0135 == 0 || DAT_104bfe1c == 0 || DAT_1020b480[index].value != 1)
        FUN_100241e0(index, step.x, step.y, step.z);
    if (DAT_104bfdf0 != 0 || DAT_104c0135 != 0 || DAT_104bfe1c != 0)
        DAT_102024ac = index;
    roll = DAT_1020b48c[index].value;
    SLOT.stats[STAT_VALUE_4] = roll;
    SLOT.held = 1;
    if ((SLOT.m_0141 == 0 || STATE_STEP == FIGHTER_STEP_STAGGER) && DAT_103153b0[index] == 0 &&
        DAT_1020b4a8[index].value == FIGHTER_SWAP_STEPS)
        DAT_1020b4a8[index].value = 8;
    if (DAT_10874848.TestScriptFlag(0x1c7) == 1 && DAT_103153b0[index] == 0)
    {
        if (SLOT.m_0141 == 0 || STATE_STEP == FIGHTER_STEP_STAGGER || blink != 0)
        {
            SLOT.held = 0;
            SLOT.stats[STAT_VALUE_4] = roll / 3;
        }
        goto count_flash;
    }
    if (DAT_103153b0[index] != 0 || (SLOT.m_0141 != 0 && STATE_STEP != FIGHTER_STEP_STAGGER && blink == 0))
    {
        if (DAT_1020b4a8[index].value == 0)
            DAT_1020b4a8[index].value = 1;
        SLOT.stats[STAT_VALUE_4] = DAT_10874848.TestScriptFlag(0x1c7) == 1 ? -8 : 0;
        SLOT.held = 0;
        if (DAT_1020b46c[index].value == 0)
        {
            Vec4 hand;
            EffectOptions flash;
            flash.followOwner = 1;
            flash.owner = index;
            FUN_1001d8b0(index, 0x5f2, hand, -1, -1, &DAT_1021bdf8[index][FIGHTER_NODE_FOOTSTEPS]->worldMatrix, 0, 0, 0,
                         &flash);
        }
        DAT_1020b46c[index].value++;
        if (STATE_STEP != 0x11 || index == FIGHTER_FIRST_SLOT)
            SLOT.m_0141 = 0;
    }
    else if (DAT_1020b46c[index].value > 0)
        DAT_1020b46c[index].value++;
count_flash:
    if (DAT_1020b46c[index].value > 0x1c)
        DAT_1020b46c[index].value = 0;
    if (motion >= 0)
    {
        MODEL.PlayMotion(motion, 0, motionSnap != 0 ? 0 : 5, 1.0f);
        if (DAT_1020b470[index].value != 0 && DAT_1020b474[index].value != MODEL.motion->layer)
        {
            DAT_1020b470[index].value = 0;
            DAT_1020b47c[index].value = 0;
            DAT_1020b480[index].value = 0;
        }
        if (DAT_102550e0[index] != 0)
            DAT_102550e0[index] = 0;
    }
    if (DAT_1020b484[index].value != 0)
    {
        Vec4 effectPos;
        EffectOptions trail;
        trail.followOwner = 1;
        FUN_1001d8b0(index, 0x6a8, effectPos, -1, -1, 0, 0, 0, &DAT_1021bdf8[index][FIGHTER_NODE_RIGHT_HAND]->worldMatrix, &trail);
    }
    SLOT.m_0141 = blink != 0;
    {
        float turn = MODEL.heading - MODEL.targetHeading;
        while (turn > FIGHTER_PI)
            turn -= FIGHTER_TWO_PI;
        while (turn < -FIGHTER_PI)
            turn += FIGHTER_TWO_PI;
        if (fabsf(turn) < 0.5f || step.Length2D() < 4.0f)
        {
            if (MODEL.spinAngle < 0.0f)
            {
                MODEL.spinAngle += 0.05f;
                if (MODEL.spinAngle > 0.0f)
                    MODEL.spinAngle = 0.0f;
            }
            else if (MODEL.spinAngle > 0.0f)
            {
                MODEL.spinAngle -= 0.05f;
                if (MODEL.spinAngle < 0.0f)
                    MODEL.spinAngle = 0.0f;
            }
        }
        else if (turn >= 0.0f)
        {
            if (MODEL.spinAngle > -0.3f)
                MODEL.spinAngle -= 0.05f;
        }
        else if (MODEL.spinAngle < 0.3f)
            MODEL.spinAngle += 0.05f;
    }
    if (DAT_1020b4a8[index].value != 0)
        SLOT.m_0141 = 1;
    return 0;
}
