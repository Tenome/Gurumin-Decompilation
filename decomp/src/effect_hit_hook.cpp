// The host's hit callback for effects (host callback slot 0 of DAT_1101de40, 0x10093cc0). EffectManager::Step calls it
// for every effect whose definition has a hit test.

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vec4.h"
#include "matrix4.h"
#include "node.h"
#include "anim_model.h"
#include "effect_manager.h"
#include "char_slot.h"
#include "char_tables.h"
#include "char_vars.h"
#include "game_vars.h"
#include "game.h"
#include "map_setup.h"
#include "map_vars.h"
#include "debris.h"
#include "dinput_min.h"
#include "texture_manager.h"
#include "world.h"
#include "value_command.h"
#include "effect_model.h"
#include "game_load_data.h"
#include "game_frame.h"
#include "script_system.h"
#include "effect_hit_hook.h"
#include "doppler_loops.h"
#include "dodge_game.h"
#include "scene_command_ids.h"
#include "target_race.h"
#include "char_hikari.h"
#include "volume_fade.h"
#include "wisp_caster_enemy.h"


#define PROP_REACH_SQUARED 1000000.0f
#define PROP_FLAG_LIMIT 0xf9f   // script flags below 4000 can be set
#define COIN_1 0x51
#define COIN_5 0x52
#define COIN_10 0x53
#define COIN_50 0x54
#define COIN_100 0x55
#define COIN_500 0x56
#define PI_APPROX 3.1415f
#define TWO_PI_APPROX 6.283f

// Throws the coins of `amount` (500, 100, 50, 10, 5, 1 by denomination) from 50 units above `origin`.
static __forceinline void ThrowCoins(const Vec4 &origin, int amount)
{
    while (amount != 0)
    {
        if (amount >= 500)
        {
            FUN_1000dd90(origin.x, origin.y, origin.z + 50.0f, COIN_500);
            amount -= 500;
        }
        else if (amount >= 100)
        {
            FUN_1000dd90(origin.x, origin.y, origin.z + 50.0f, COIN_100);
            amount -= 100;
        }
        else if (amount >= 50)
        {
            FUN_1000dd90(origin.x, origin.y, origin.z + 50.0f, COIN_50);
            amount -= 50;
        }
        else if (amount >= 10)
        {
            FUN_1000dd90(origin.x, origin.y, origin.z + 50.0f, COIN_10);
            amount -= 10;
        }
        else if (amount >= 5)
        {
            FUN_1000dd90(origin.x, origin.y, origin.z + 50.0f, COIN_5);
            amount -= 5;
        }
        else if (amount > 0)
        {
            FUN_1000dd90(origin.x, origin.y, origin.z + 50.0f, COIN_1);
            amount--;
        }
    }
}

// The flash of a parried strike: effect 0x8f0c is started on the player's "scop-pt" node (turned to face the hit) and
// sound 0x338 plays at `object`.
static __forceinline void StartParryFlash(const Vec4 &center, Matrix4 &object)
{
    Matrix4 flashMatrix;
    Vec4 playerPosition;
    Vec4 position = object.GetTranslation();
    playerPosition.Copy(DAT_10db9210.GetTranslation());
    float angle = (float)atan2((double)(playerPosition.x - center.x), (double)(playerPosition.y - center.y));
    Node *scop = DAT_10db6ee8[0].root->Find("scop-pt", 0);
    flashMatrix = RotationZ(angle + TWO_PI_APPROX);
    if (scop != 0)
        flashMatrix.SetTranslation(scop->worldMatrix.GetTranslation());
    position.Fill(0.0f);
    DAT_10daaa80.Start(0x8f0c, position, -1, 0, 0, 0, 0, &flashMatrix, 0, 0);
    FUN_1003ad20(0x338, &object.GetTranslation(), 1.0f);
}

// Pushes the player back from the parry and turns the character in `slot` away from it.
static __forceinline void ParryRecoil(const Vec4 &center, int slot)
{
    FUN_10011b70(0xc, 4, 0, &center);
    DAT_104c01a0 = 10;
    Vec4 playerPosition;
    Vec4 slotPosition;
    playerPosition.Copy(DAT_10db9210.GetTranslation());
    slotPosition.Copy(DAT_10db6ee8[slot].matrix.GetTranslation());
    float turn = (float)atan2((double)(slotPosition.x - playerPosition.x), (double)(slotPosition.y - playerPosition.y));
    turn = turn - DAT_10db6ee8[slot].targetHeading;
    DAT_10db6ee8[slot].blastAngle = (int)(turn * 1000.0f);
    DAT_10db6ee8[slot].blastPush = 30;
    DAT_10f08008.Rumble(1.0f, 100);
}

// A hit that did not hurt: the character in `slot` shows its reaction.
#define SHOW_REACTION(slot) FUN_10011aa0((slot), 1)

int FUN_10093cc0(float x, float y, float z, float size, EffectOptions *options)
{
    if (DAT_1032d69c == 0 && DAT_10c64fbc == 0)
        return 0;
    if (DAT_10daaaec < 1)
        return 0;
    Vec4 center;
    center.Set(x, y, z, 0.0f);
    Node *hitNodes[HIT_NODE_MAX];
    int hitCount;
    int negativeHit = 0;
    if (options->m_001c != 0)
    {
        if (options->m_0000 != 0)
        {
            int alreadyHit = 0;
            for (int slot = HIT_FIRST_ENEMY_SLOT; slot < HIT_END_ENEMY_SLOT; slot++)
            {
                if (DAT_10db6ee8[slot].root == 0 || DAT_10db6ee8[slot].m_2384 == 0)
                    continue;
                char kind = DAT_1021f3c0[slot];
                if (!((kind == HIT_KIND_ENEMY && DAT_10210d30[slot] == 0) || kind == HIT_KIND_CRATE ||
                      kind == HIT_KIND_SPARE_63 || kind == HIT_KIND_GUARD_Q || kind == HIT_KIND_SPARE_60 ||
                      kind == HIT_KIND_SPARE_4D || kind == HIT_KIND_GUARD_COMMA || kind == HIT_KIND_TREASURE ||
                      kind == HIT_KIND_POT))
                    continue;
                int damage = options->m_0004;
                DAT_10daaae8[slot].stats[STAT_NEGATIVE_HIT] = 0;
                if (damage < 0)
                {
                    damage = -damage;
                    negativeHit = 1;
                    DAT_10daaae8[slot].stats[STAT_NEGATIVE_HIT] = 1;
                }
                hitCount = 0;
                if (options->settings[3] == 0 || options->m_0034 == 0)
                    DAT_10db6ee8[slot].root->FindTouchingSphere((int)&DAT_10db6ee8[slot].matrix, &center, size, hitNodes,
                                                                &hitCount);
                else
                    DAT_10db6ee8[slot].root->CollectNearSegment((int)&DAT_10db6ee8[slot].matrix, options->m_0034,
                                                                options->m_0038, options->m_0034->w, hitNodes, &hitCount,
                                                                &center);
                kind = DAT_1021f3c0[slot];
                if ((kind == 2 || kind == 3) && DAT_104bfe1c != 0)
                    continue;

                // 'Q': the armoured enemy parries the sword and loses one hit point per counted strike.
                if (hitCount != 0 && DAT_1021f3c0[slot] == HIT_KIND_GUARD_Q)
                {
                    unsigned char strike = options->settings[4];
                    if ((strike & STRIKE_HEAVY) != 0)
                    {
                        if ((strike & STRIKE_SWORD) != 0 && DAT_10db6ee8[slot].m_0288 == 0)
                        {
                            DAT_10db6ee8[slot].m_0288 = 6;
                            if (DAT_10db6ee8[slot].m_028c > 0)
                                DAT_10db6ee8[slot].m_028c--;
                            DAT_10f08008.Rumble(1.0f, 200);
                            if ((options->settings[4] & STRIKE_DOUBLE) == 0)
                                FUN_100119d0(slot, 1, 1);
                            else
                            {
                                FUN_100119d0(slot, 2, 1);
                                if (DAT_10db6ee8[slot].m_028c > 0)
                                    DAT_10db6ee8[slot].m_028c--;
                            }
                            if (negativeHit != 0)
                            {
                                if (DAT_10db6ee8[slot].m_028c > 0)
                                    DAT_10db6ee8[slot].m_028c--;
                                FUN_1003ab40(slot, 0);
                            }
                            FUN_1003ad20(0x336, &DAT_10db6ee8[slot].matrix.GetTranslation(), 1.0f);
                            Vec4 position = DAT_10db6ee8[slot].matrix.GetTranslation();
                            position.z += 50.0f;
                            DAT_10daaa80.Start(0x28a, position, -1, 0, 0, 0, 0, 0, 0, 0);
                            Vec4 sprayOrigin;
                            Vec4 spare;
                            position.Set(0.0f, 0.0f, 15.0f, 0.0f);
                            Node *scop = DAT_10db6ee8[0].root->Find("scop-pt", 0);
                            sprayOrigin.Copy(DAT_10db9210.GetTranslation());
                            if (scop != 0)
                                sprayOrigin.Copy(scop->worldMatrix.GetTranslation());
                            for (int piece = 0; piece < 4; piece++)
                                FUN_1000e130(sprayOrigin.x, sprayOrigin.y, sprayOrigin.z, &position, FUN_1013f880() % 2 + 0x6c,
                                             -1);
                            if (DAT_10db6ee8[slot].m_028c == 4 || DAT_10db6ee8[slot].m_028c == 3)
                            {
                                Node *crack = DAT_10db6ee8[slot].root->Find("hibi2", 0);
                                if (crack != 0)
                                    crack->ClearFlags(0x20);
                            }
                            if (DAT_10db6ee8[slot].m_028c == 0)
                            {
                                FUN_1016b230("656565");
                                Vec4 where = DAT_10db6ee8[slot].matrix.GetTranslation();
                                int dropType = DAT_10db6ee8[slot].m_0284 / 10000;
                                int dropCount = DAT_10db6ee8[slot].m_0284 % 10000;
                                if (dropType > 0 && dropCount > 0 && DAT_10c64000 == 0 && dropCount > 0)
                                {
                                    do
                                    {
                                        FUN_1000dd90(where.x, where.y, where.z + 80.0f, dropType);
                                        dropCount--;
                                    } while (dropCount != 0);
                                }
                                int coins = FUN_1013f880() % 2 + 2;
                                while (coins > 0)
                                {
                                    FUN_1000dd90(where.x, where.y, where.z + 80.0f, COIN_5);
                                    coins--;
                                }
                                DAT_1021f3c0[slot] = HIT_KIND_GUARD_DONE;
                                FUN_1003ad20(0x337, &DAT_10db6ee8[slot].matrix.GetTranslation(), 1.0f);
                                int leftModel = DAT_10db6ee8[slot].leftModel;
                                Vec4 tint;
                                tint.Copy(DAT_10db6ee8[slot].scale);
                                DAT_10c64ad8 += 2;
                                FUN_1000e750();
                                FUN_1003a880(DAT_10894698[leftModel], where.x, where.y, where.z, 0, 0, 0,
                                             DAT_10db6ee8[slot].targetHeading * 180.0f / PI_APPROX, tint);
                                DAT_10836c70.ResetFrameTimer();
                                DAT_10874848.SetScriptFlag(DAT_10db6ee8[slot].scriptFlag, 0x29a);
                                DAT_10db6ee8[slot].SetPosition(-250000.0f, 0, 0);
                            }
                        }
                    }
                    else
                    {
                        if ((strike & STRIKE_SWORD) != 0 && DAT_104c01a0 == 0)
                        {
                            StartParryFlash(center, DAT_10db6ee8[slot].matrix);
                            ParryRecoil(center, slot);
                        }
                    }
                }

                // ',': the same parry; its model is left behind when it falls.
                if (hitCount != 0 && DAT_1021f3c0[slot] == HIT_KIND_GUARD_COMMA)
                {
                    unsigned char strike = options->settings[4];
                    if ((strike & STRIKE_HEAVY) != 0)
                    {
                        if ((strike & STRIKE_SWORD) != 0 && DAT_10db6ee8[slot].m_0288 == 0)
                        {
                            DAT_10db6ee8[slot].m_0288 = 6;
                            if (DAT_10db6ee8[slot].m_028c > 0)
                                DAT_10db6ee8[slot].m_028c--;
                            DAT_10f08008.Rumble(1.0f, 200);
                            if ((options->settings[4] & STRIKE_DOUBLE) == 0)
                                FUN_100119d0(slot, 1, 1);
                            else
                            {
                                FUN_100119d0(slot, 2, 1);
                                if (DAT_10db6ee8[slot].m_028c > 0)
                                    DAT_10db6ee8[slot].m_028c--;
                            }
                            if (negativeHit != 0)
                            {
                                if (DAT_10db6ee8[slot].m_028c > 0)
                                    DAT_10db6ee8[slot].m_028c--;
                                FUN_1003ab40(slot, 0);
                            }
                            FUN_1003ad20(0x336, &DAT_10db6ee8[slot].matrix.GetTranslation(), 1.0f);
                            Vec4 position = DAT_10db6ee8[slot].matrix.GetTranslation();
                            position.z += 50.0f;
                            DAT_10daaa80.Start(0x28a, position, -1, 0, 0, 0, 0, 0, 0, 0);
                            Vec4 sprayOrigin;
                            Vec4 spare;
                            position.Set(0.0f, 0.0f, 15.0f, 0.0f);
                            Node *scop = DAT_10db6ee8[0].root->Find("scop-pt", 0);
                            sprayOrigin.Copy(DAT_10db9210.GetTranslation());
                            if (scop != 0)
                                sprayOrigin.Copy(scop->worldMatrix.GetTranslation());
                            for (int piece = 0; piece < 4; piece++)
                                FUN_1000e130(sprayOrigin.x, sprayOrigin.y, sprayOrigin.z, &position, FUN_1013f880() % 2 + 0x6c,
                                             -1);
                            if (DAT_10db6ee8[slot].m_028c == 0)
                            {
                                FUN_1016b230("656565");
                                Vec4 where = DAT_10db6ee8[slot].matrix.GetTranslation();
                                int coins = FUN_1013f880() % 3 + 1;
                                while (coins > 0)
                                {
                                    FUN_1000dd90(where.x, where.y, where.z + 30.0f, COIN_5);
                                    coins--;
                                }
                                DAT_1021f3c0[slot] = HIT_KIND_CRATE_BROKEN;
                                FUN_1003ad20(0x337, &DAT_10db6ee8[slot].matrix.GetTranslation(), 1.0f);
                                int leftModel = DAT_10db6ee8[slot].leftModel;
                                Vec4 tint;
                                tint.Set(1.0f, 1.0f, 1.0f, 1.0f);
                                DAT_10c64ad8 += 2;
                                FUN_1000e750();
                                FUN_1003a880(DAT_10894698[leftModel], where.x, where.y, where.z, 0, 0, 0,
                                             DAT_10db6ee8[slot].targetHeading * 180.0f / PI_APPROX, tint);
                                DAT_10836c70.ResetFrameTimer();
                                DAT_10874848.SetScriptFlag(DAT_10db6ee8[slot].scriptFlag, 0x29a);
                                DAT_10db6ee8[slot].SetPosition(-250000.0f, 0, 0);
                            }
                        }
                    }
                    else
                    {
                        if ((strike & STRIKE_SWORD) != 0 && DAT_104c01a0 == 0)
                        {
                            StartParryFlash(center, DAT_10db6ee8[slot].matrix);
                            ParryRecoil(center, slot);
                        }
                    }
                }

                // 5: the crate.
                if (hitCount != 0 && DAT_1021f3c0[slot] == HIT_KIND_CRATE)
                {
                    unsigned char strike = options->settings[4];
                    if ((strike & STRIKE_HEAVY) != 0)
                    {
                        if ((strike & STRIKE_SWORD) != 0 && DAT_10db6ee8[slot].m_0288 == 0)
                        {
                            DAT_10db6ee8[slot].m_0288 = 6;
                            if (DAT_10db6ee8[slot].m_028c > 0)
                                DAT_10db6ee8[slot].m_028c--;
                            if ((options->settings[4] & STRIKE_DOUBLE) != 0 && DAT_10db6ee8[slot].m_028c > 0)
                                DAT_10db6ee8[slot].m_028c--;
                            FUN_100119d0(slot, 1, 1);
                            DAT_10f08008.Rumble(1.0f, 200);
                            if (negativeHit != 0)
                            {
                                FUN_1003ab40(slot, 0);
                                if (DAT_10db6ee8[slot].m_028c > 0)
                                    DAT_10db6ee8[slot].m_028c--;
                            }
                            FUN_1003ad20(0x336, &DAT_10db6ee8[slot].matrix.GetTranslation(), 1.0f);
                            Vec4 position = DAT_10db6ee8[slot].matrix.GetTranslation();
                            position.z += 50.0f;
                            DAT_10daaa80.Start(0x28a, position, -1, 0, 0, 0, 0, 0, 0, 0);
                            if (DAT_10db6ee8[slot].m_028c == 0)
                            {
                                FUN_1016b230("656565");
                                DAT_1021f3c0[slot] = HIT_KIND_SPARE_FB;
                                FUN_1003ad20(0x337, &DAT_10db6ee8[slot].matrix.GetTranslation(), 1.0f);
                                Vec4 where = DAT_10db6ee8[slot].matrix.GetTranslation();
                                DAT_10c64ad8 += 2;
                                FUN_1000e750();
                                FUN_1003a7d0("O_001br.it3", where.x, where.y, where.z, 0, 0, 0, 0);
                                int pieces = FUN_1013f880() % 3 + 3;
                                while (pieces > 0)
                                {
                                    FUN_1000dd90(where.x, where.y, where.z + 30.0f, COIN_5);
                                    pieces--;
                                }
                                DAT_10db6ee8[slot].SetPosition(-250000.0f, 0, 0);
                            }
                        }
                    }
                    else
                    {
                        if ((strike & STRIKE_SWORD) != 0 && DAT_104c01a0 == 0)
                        {
                            StartParryFlash(center, DAT_10db6ee8[slot].matrix);
                            FUN_10011b70(0xc, 4, 0, &center);
                            DAT_104c01a0 = 10;
                            DAT_10db6ee8[slot].m_0288 = 6;
                            DAT_10f08008.Rumble(1.0f, 100);
                        }
                    }
                }

                // 'z' (a pot) and 'P' (a treasure) break at once.
                if (hitCount != 0 && ((DAT_104c01e0 == 0 && DAT_1021f3c0[slot] == HIT_KIND_POT) ||
                                      DAT_1021f3c0[slot] == HIT_KIND_TREASURE))
                {
                    if (DAT_104c01e0 == 0 && DAT_1021f3c0[slot] == HIT_KIND_POT)
                    {
                        Vec4 where = DAT_10db6ee8[slot].matrix.GetTranslation();
                        if (where.x > -100000.0f)
                        {
                            DAT_1021f3c0[slot] = HIT_KIND_POT_DONE;
                            if (negativeHit != 0)
                                FUN_1003ab40(slot, 0);
                            DAT_10874848.RegisterEvent(0);
                            DAT_10f08008.Rumble(1.8f, 100);
                            Vec4 scale;
                            scale.Set(DAT_10db6ee8[slot].scale.x, DAT_10db6ee8[slot].scale.y, DAT_10db6ee8[slot].scale.z,
                                      1.0f);
                            DAT_10c64ad8 += 1;
                            FUN_1000e750();
                            FUN_1003ad20(0x338, &DAT_10db6ee8[slot].matrix.GetTranslation(), 1.0f);
                            FUN_1003a880("O_061br.it3", where.x, where.y, where.z, 0, 0, 0,
                                         DAT_10db6ee8[slot].targetHeading * 180.0f / PI_APPROX, scale);
                            DAT_10db6ee8[slot].SetPosition(-250000.0f, 0, 0);
                        }
                    }
                    else if (DAT_1021f3c0[slot] == HIT_KIND_TREASURE)
                    {
                        DAT_1021f3c0[slot] = HIT_KIND_TREASURE_DONE;
                        Vec4 position = DAT_10db6ee8[slot].matrix.GetTranslation();
                        if (negativeHit != 0)
                            FUN_1003ab40(slot, 0);
                        DAT_10f08008.Rumble(1.8f, 100);
                        int item;
                        int count;
                        if (DAT_10db6ee8[slot].m_0280 == 5 || DAT_10db6ee8[slot].m_0280 == 4)
                        {
                            FUN_10017b10(DAT_10db6ee8[slot].scriptFlag, &item, &count);
                            if (item > 0 && count > 0)
                            {
                                Vec4 chestPosition;
                                chestPosition.Copy(position);
                                chestPosition.z += 50.0f;
                                for (int piece = 5; piece != 0; piece--)
                                {
                                    int roll = FUN_1013f880() % 10;
                                    FUN_10062750(&chestPosition, "", roll, 0.3f, 0.0f);
                                }
                            }
                            item = 0;
                            count = 0;
                            DAT_10c62edc[DAT_10db6ee8[slot].scriptFlag] = 1;
                            FUN_1013faf0();
                        }
                        else
                        {
                            FUN_10017b10(DAT_10db6ee8[slot].scriptFlag, &item, &count);
                            if (item > 0 && count > 0)
                            {
                                do
                                {
                                    FUN_1000dd90(position.x, position.y, position.z + 130.0f, item);
                                    count--;
                                } while (count != 0);
                            }
                            int coins = FUN_1013f880() % 3 + 2;
                            while (coins > 0)
                            {
                                FUN_1000dd90(position.x, position.y, position.z + 130.0f, COIN_1);
                                coins--;
                            }
                            item = 0;
                            count = 0;
                            DAT_10c62edc[DAT_10db6ee8[slot].scriptFlag] = 1;
                        }
                        DAT_10c64ad8 += 1;
                        FUN_1000e750();
                        switch (DAT_10db6ee8[slot].m_0280)
                        {
                        case 5:
                            FUN_1003ad20(0x11b, &DAT_10db6ee8[slot].matrix.GetTranslation(), 1.0f);
                            FUN_1003a880("O_075mbr.it3", position.x, position.y, position.z, 0, 0, 0, 0, DAT_10db6ee8[slot].scale);
                            break;
                        case 4:
                            FUN_1003ad20(0x11b, &DAT_10db6ee8[slot].matrix.GetTranslation(), 1.0f);
                            FUN_1003a880("O_075br.it3", position.x, position.y, position.z, 0, 0, 0, 0, DAT_10db6ee8[slot].scale);
                            break;
                        case 3:
                            FUN_1003ad20(0x33b, &DAT_10db6ee8[slot].matrix.GetTranslation(), 1.0f);
                            FUN_1003a880("O_071br.it3", position.x, position.y, position.z, 0, 0, 0, 0, DAT_10db6ee8[slot].scale);
                            break;
                        case 2:
                            FUN_1003ad20(0x33b, &DAT_10db6ee8[slot].matrix.GetTranslation(), 1.0f);
                            FUN_1003a880("O_070br.it3", position.x, position.y, position.z, 0, 0, 0, 0, DAT_10db6ee8[slot].scale);
                            break;
                        case 1:
                            FUN_1003ad20(0x33b, &DAT_10db6ee8[slot].matrix.GetTranslation(), 1.0f);
                            FUN_1003a880("O_014mb.it3", position.x, position.y, position.z, 0, 0, 0, 0, DAT_10db6ee8[slot].scale);
                            break;
                        default:
                            FUN_1003ad20(0x33b, &DAT_10db6ee8[slot].matrix.GetTranslation(), 1.0f);
                            FUN_1003a880("O_014b.it3", position.x, position.y, position.z, 0, 0, 0, 0, DAT_10db6ee8[slot].scale);
                            break;
                        }
                        DAT_10db6ee8[slot].SetPosition(-250000.0f, 0, 0);
                    }
                    continue;
                }

                // The strike lands on every node it touched.
                for (int hit = 0; hit < hitCount; hit++)
                {
                    char element = options->m_000e;
                    int reacted = 0;
                    if (element == ELEMENT_THUNDER && (hitNodes[hit]->element & 3) != 0)
                    {
                        FUN_10057b90(slot);
                        return 1;
                    }
                    if (element == ELEMENT_ICE && (hitNodes[hit]->element & 2) != 0)
                    {
                        FUN_10057c40(slot, hitNodes[hit], 2);
                        return 1;
                    }
                    if (element == ELEMENT_FIRE && (hitNodes[hit]->element & 1) != 0)
                    {
                        FUN_10057c40(slot, hitNodes[hit], 0);
                        return 1;
                    }
                    int immune = 0;
                    if (hitCount != 0 && alreadyHit == 0 && DAT_10daaae8[slot].m_0141 == 0 &&
                        DAT_1021f3c0[slot] == HIT_KIND_ENEMY)
                    {
                        // Fire, ice and thunder set the enemy burning, freezing or shocked.
                        if (DAT_104686b0[slot] == 0 && element == ELEMENT_FIRE && options->settings[5] == 0 &&
                            (DAT_10daaae8[slot].m_0144 & 1) == 0 && (DAT_10daaae8[slot].m_0098 & 2) == 0)
                        {
                            DAT_103153b0[slot] = 300;
                            if (DAT_10c635f8 == 2)
                            {
                                DAT_103153b0[slot] = 0x96;
                                if (DAT_10c66cc0 == 2)
                                    DAT_103153b0[slot] = 0x14;
                            }
                            if (DAT_10315350[slot] < 3)
                            {
                                Vec4 position = DAT_10db6ee8[slot].matrix.GetTranslation();
                                position.z += 50.0f;
                                DAT_10daaa80.Start(0x2265, position, -1, 0, &DAT_10db6ee8[slot].matrix, 0, 0, 0, 0, 0);
                            }
                            int partner = DAT_10daaae8[slot].m_00a8;
                            DAT_10315350[slot] = 5;
                            if (partner != 0)
                                DAT_10315350[DAT_10daaae8[slot].m_00a4] = 5;
                        }
                        if (DAT_104686b0[slot] == 0)
                        {
                            if (options->m_000e == ELEMENT_ICE && options->settings[5] == 0 &&
                                (DAT_10daaae8[slot].m_0144 & 4) == 0 && (DAT_10daaae8[slot].m_0098 & 8) == 0)
                            {
                                DAT_10219770[slot] = 300;
                                if (DAT_10c635f8 == 2)
                                {
                                    DAT_10219770[slot] = 0x78;
                                    if (DAT_10c66cc0 == 2)
                                        DAT_10219770[slot] = 0xf;
                                }
                                if (DAT_10315350[slot] < 0xc)
                                {
                                    Vec4 position = DAT_10db6ee8[slot].matrix.GetTranslation();
                                    position.z += 50.0f;
                                    DAT_10daaa80.Start(0x834, position, -1, 0, &DAT_10db6ee8[slot].matrix, 0, 0, 0, 0, 0);
                                }
                                int partner = DAT_10daaae8[slot].m_00a8;
                                DAT_10315350[slot] = 0xf;
                                if (partner != 0)
                                    DAT_10315350[DAT_10daaae8[slot].m_00a4] = 0xf;
                            }
                            if (DAT_104686b0[slot] == 0)
                            {
                                if (options->m_000e == ELEMENT_THUNDER && options->settings[5] == 0 &&
                                    (DAT_10daaae8[slot].m_0144 & 2) == 0 && (DAT_10daaae8[slot].m_0098 & 4) == 0)
                                {
                                    if (DAT_10315350[slot] < 0x32)
                                    {
                                        Vec4 position = DAT_10db6ee8[slot].matrix.GetTranslation();
                                        position.z += 50.0f;
                                        DAT_10daaa80.Start(0x13ed, position, -1, 0, &DAT_10db6ee8[slot].matrix, 0, 0, 0, 0, 0);
                                    }
                                    DAT_10315350[slot] = 0x56;
                                    if (DAT_10c635f8 == 2)
                                    {
                                        DAT_10315350[slot] = 0x2b;
                                        if (DAT_10c66cc0 == 2)
                                            DAT_10315350[slot] = 0xf;
                                    }
                                    if (DAT_10daaae8[slot].m_00a8 != 0)
                                        DAT_10315350[DAT_10daaae8[slot].m_00a4] = 0x56;
                                }
                                if (DAT_104686b0[slot] == 0)
                                {
                                    // Elements the enemy is weak or resistant against.
                                    char e = options->m_000e;
                                    if (e == ELEMENT_FIRE && (DAT_10daaae8[slot].m_0143 & 1) != 0)
                                    {
                                        damage *= 2;
                                        SHOW_REACTION(slot);
                                        reacted = 1;
                                    }
                                    else if (e == ELEMENT_ICE && (DAT_10daaae8[slot].m_0143 & 4) != 0)
                                    {
                                        damage *= 2;
                                        SHOW_REACTION(slot);
                                        reacted = 1;
                                    }
                                    else if (e == ELEMENT_THUNDER && (DAT_10daaae8[slot].m_0143 & 2) != 0)
                                    {
                                        damage *= 2;
                                        SHOW_REACTION(slot);
                                        reacted = 1;
                                    }
                                    else if (e == ELEMENT_BLAST && (DAT_10daaae8[slot].m_0143 & 8) != 0)
                                    {
                                        damage *= 2;
                                        SHOW_REACTION(slot);
                                        reacted = 1;
                                    }
                                    else if ((e == ELEMENT_FIRE && (DAT_10daaae8[slot].m_0144 & 1) != 0) ||
                                             (e == ELEMENT_ICE && (DAT_10daaae8[slot].m_0144 & 4) != 0) ||
                                             (e == ELEMENT_THUNDER && (DAT_10daaae8[slot].m_0144 & 2) != 0))
                                    {
                                        SHOW_REACTION(slot);
                                        reacted = 1;
                                    }
                                    else if (e == ELEMENT_BLAST && (DAT_10daaae8[slot].m_0144 & 8) != 0)
                                    {
                                        damage /= 2;
                                        SHOW_REACTION(slot);
                                        reacted = 1;
                                    }
                                    if (DAT_104686b0[slot] == 0 && options->m_000e != ELEMENT_BLAST &&
                                        options->settings[5] == 0 && (options->owner == 999 || options->owner == 9999) &&
                                        (DAT_10c655f0 == 0xc || DAT_10c655f0 == 0xb || DAT_10c655f0 == 8 ||
                                         (DAT_104c02c8 & 0x3000000) == 0x3000000 || (DAT_104c02c8 & 0x2000000) != 0 ||
                                         (DAT_104c02c8 & 0x1000000) != 0))
                                    {
                                        if (DAT_10c655f0 == 0xc)
                                            damage = (int)((double)damage * 5.5);
                                        else if (DAT_10c655f0 == 0xb)
                                            damage = (int)((double)damage * 4.5);
                                        else if (DAT_10c655f0 == 8)
                                            damage = (int)((double)damage * 3.5);
                                        else if ((DAT_104c02c8 & 0x3000000) == 0x3000000)
                                            damage = (int)((double)damage * 3.0);
                                        else if ((DAT_104c02c8 & 0x2000000) != 0)
                                            damage = (int)((double)damage * 2.0);
                                        else
                                            damage = (int)((double)damage * 1.5);
                                    }
                                }
                            }
                        }
                        immune = 0;
                        if (DAT_104686b0[slot] == 0)
                        {
                            if (DAT_10daaae8[slot].held != 0 ||
                                (DAT_10c66cc0 == 2 && DAT_10c635f8 == 2 &&
                                 (((options->settings[4] & STRIKE_SLASH) != 0 && negativeHit == 0) ||
                                  (FUN_1013f880() % 100 < 30 && negativeHit == 0) ||
                                  (DAT_10c66cc0 == 2 && DAT_10c635f8 == 2 && options->owner == 0x4d55))))
                            {
                                SHOW_REACTION(slot);
                                damage = 0;
                                reacted = 1;
                                immune = 1;
                            }
                            if (DAT_104686b0[slot] == 0)
                            {
                                // A guarding enemy blocks strikes that come from the side it faces.
                                if (DAT_102239d0[slot] == 0 && options->settings[5] < 2 && DAT_10daaae8[slot].twoHanded != 0)
                                {
                                    Vec4 fromPoint;
                                    if (options->settings[5] == 0)
                                        fromPoint.Copy(center);
                                    if (options->settings[5] == 1)
                                        fromPoint.Copy(DAT_10db9210.GetTranslation());
                                    Vec4 slotPosition = DAT_10db6ee8[slot].matrix.GetTranslation();
                                    float facing = (float)atan2((double)(slotPosition.x - fromPoint.x),
                                                                (double)(slotPosition.y - fromPoint.y));
                                    float diff = facing - DAT_10db6ee8[slot].heading;
                                    while (diff > PI_APPROX)
                                        diff -= TWO_PI_APPROX;
                                    while (diff < -PI_APPROX)
                                        diff += TWO_PI_APPROX;
                                    if (options->settings[7] == 0 && DAT_10daaae8[slot].twoHanded == 1 &&
                                        diff < 0.78537f && diff >= -0.78537f)
                                    {
                                        SHOW_REACTION(slot);
                                        damage = 0;
                                        reacted = 1;
                                        immune = 1;
                                        if ((options->settings[4] & STRIKE_SWORD) != 0 && options->settings[5] == 1 &&
                                            DAT_104c01a0 == 0)
                                        {
                                            StartParryFlash(center, DAT_10db6ee8[slot].matrix);
                                            if (DAT_1042d9e8[slot] != 0 || DAT_10c66cc0 != 2 || DAT_10c635f8 != 2)
                                                FUN_10011b70(0xc, 4, 0, &center);
                                            DAT_104c01a0 = 10;
                                            DAT_10f08008.Rumble(1.0f, 100);
                                        }
                                    }
                                    if (options->settings[7] == 0 && DAT_10daaae8[slot].twoHanded == 2)
                                    {
                                        SHOW_REACTION(slot);
                                        damage = 0;
                                        reacted = 1;
                                        immune = 1;
                                        if ((options->settings[4] & STRIKE_SWORD) != 0 && options->settings[5] == 1 &&
                                            DAT_104c01a0 == 0)
                                        {
                                            StartParryFlash(center, DAT_10db6ee8[slot].matrix);
                                            if (DAT_1042d9e8[slot] != 0 || DAT_10c66cc0 != 2 || DAT_10c635f8 != 2)
                                                FUN_10011b70(0xc, 4, 0, &center);
                                            DAT_104c01a0 = 10;
                                            DAT_10f08008.Rumble(1.0f, 100);
                                        }
                                    }
                                }
                                if (DAT_104686b0[slot] == 0)
                                {
                                    // Immunities of the enemy to the strike's element and kind.
                                    if (options->m_000e == ELEMENT_BLAST && (DAT_10daaae8[slot].m_0098 & 1) != 0)
                                    {
                                        SHOW_REACTION(slot);
                                        reacted = 1;
                                        damage = 0;
                                        immune = 1;
                                    }
                                    if (options->m_000e == ELEMENT_FIRE && (DAT_10daaae8[slot].m_0098 & 2) != 0)
                                    {
                                        SHOW_REACTION(slot);
                                        reacted = 1;
                                        damage = 0;
                                        immune = 1;
                                    }
                                    if (options->m_000e == ELEMENT_THUNDER && (DAT_10daaae8[slot].m_0098 & 4) != 0)
                                    {
                                        SHOW_REACTION(slot);
                                        reacted = 1;
                                        damage = 0;
                                        immune = 1;
                                    }
                                    if (options->m_000e == ELEMENT_ICE && (DAT_10daaae8[slot].m_0098 & 8) != 0)
                                    {
                                        SHOW_REACTION(slot);
                                        reacted = 1;
                                        damage = 0;
                                        immune = 1;
                                    }
                                    if (options->settings[5] == 1 && (DAT_10daaae8[slot].m_0098 & 0x10) != 0)
                                    {
                                        SHOW_REACTION(slot);
                                        reacted = 1;
                                        damage = 0;
                                        immune = 1;
                                    }
                                    if (options->settings[5] == 0 && (DAT_10daaae8[slot].m_0098 & 0x20) != 0)
                                    {
                                        SHOW_REACTION(slot);
                                        reacted = 1;
                                        damage = 0;
                                        immune = 1;
                                    }
                                    DAT_10daaae8[slot].m_0084 = options->m_000e;
                                    if (damage > 99999)
                                        damage = 99999;
                                    damage = damage - DAT_10daaae8[slot].stats[STAT_DEFENCE];
                                    if (damage < 1)
                                        damage = 1;
                                    damage = (int)((float)damage * options->m_0008);
                                    if (DAT_102239d0[slot] != 0)
                                        damage = damage * 2;
                                    if (damage < 1)
                                        damage = 1;
                                    if (DAT_104686b0[slot] == 0)
                                    {
                                        if (immune != 0)
                                        {
                                            if (options->settings[5] == 1 && DAT_104c01a0 == 0)
                                            {
                                                StartParryFlash(center, DAT_10db6ee8[slot].matrix);
                                                if (DAT_1042d9e8[slot] != 0 || DAT_10c66cc0 != 2 || DAT_10c635f8 != 2)
                                                    FUN_10011b70(0xc, 4, 0, &center);
                                                DAT_104c01a0 = 10;
                                                DAT_10f08008.Rumble(1.0f, 100);
                                            }
                                            damage = 0;
                                        }
                                        if (DAT_104686b0[slot] == 0)
                                        {
                                            if (DAT_102239d0[slot] != 0 || DAT_10daaae8[slot].stats[STAT_HEALTH] > 0)
                                            {
                                                FUN_100119d0(slot, damage, 0);
                                                if (options->owner != 0x1226)
                                                {
                                                    if (options->settings[5] == 0)
                                                        DAT_10f08008.Rumble(1.8f, 100);
                                                    else if (negativeHit == 0 && (options->settings[4] & STRIKE_HEAVY) == 0)
                                                        DAT_10f08008.Rumble(1.8f, 100);
                                                    else
                                                        DAT_10f08008.Rumble(1.0f, 100);
                                                }
                                                int health = DAT_10daaae8[slot].stats[STAT_HEALTH];
                                                DAT_10c64020 += damage;
                                                DAT_10daaae8[slot].stats[STAT_HEALTH] = health - damage;
                                                if ((options->settings[4] & STRIKE_HEAVY) != 0 &&
                                                    (options->settings[4] & STRIKE_SWORD) != 0 && options->settings[5] == 1)
                                                    DAT_1042d9e8[slot] = 1;
                                                DAT_1021f6c0[slot] = 0;
                                                DAT_10daaae8[slot].stats[STAT_COIN_BONUS] = (unsigned)(health * 2 <= damage);
                                                if (DAT_10daaae8[slot].stats[STAT_HEALTH] > 0 && DAT_102239d0[slot] == 0)
                                                    DAT_10467ba0[slot] = 0;
                                                if (DAT_10daaae8[slot].stats[STAT_HEALTH] < 1)
                                                {
                                                    if (options->settings[5] == 0 && DAT_10467ba0[slot] != -100 &&
                                                        DAT_10467ba0[slot] != -0x65)
                                                        DAT_10467ba0[slot] = (char)0xff;
                                                    if (options->settings[7] != 0 && DAT_10467ba0[slot] != -100 &&
                                                        DAT_10467ba0[slot] != -0x65)
                                                        DAT_10467ba0[slot] = (char)0xff;
                                                }
                                            }
                                            if (DAT_104686b0[slot] == 0)
                                            {
                                                DAT_104686b0[slot] = 8;
                                                if (negativeHit == 0)
                                                {
                                                    if (DAT_104c00f8 != 0)
                                                        DAT_10f08080[slot].Copy(DAT_10836c74 -
                                                                                DAT_10db6ee8[slot].matrix.GetTranslation());
                                                    DAT_1020d4c0[slot] = 4;
                                                }
                                                else
                                                {
                                                    if (DAT_104c00f8 != 0)
                                                        DAT_10f08080[slot].Copy(DAT_10836c74 -
                                                                                DAT_10db6ee8[slot].matrix.GetTranslation());
                                                    DAT_1020d4c0[slot] = 6;
                                                }
                                                Vec4 hitPosition = DAT_10db6ee8[slot].matrix.GetTranslation();
                                                Vec4 raw = hitPosition;
                                                hitPosition.z += 50.0f;
                                                float half = 0.5f;
                                                hitPosition.Copy((raw + hitPosition) * half);
                                                if (options->settings[5] == 1 && negativeHit != 0)
                                                    DAT_104c0132 = 1;
                                                if (options->settings[5] == 0 || negativeHit == 0)
                                                    DAT_10daaa80.Start(0x28a, hitPosition, -1, 0, 0, 0, 0, 0, 0, 0);
                                                else
                                                {
                                                    Matrix4 hitMatrix;
                                                    Vec4 playerPosition;
                                                    playerPosition.Copy(DAT_10db9210.GetTranslation());
                                                    float angle = (float)atan2((double)(playerPosition.x - center.x),
                                                                               (double)(playerPosition.y - center.y));
                                                    hitMatrix = RotationZ(angle + PI_APPROX);
                                                    hitMatrix.SetTranslation(hitPosition);
                                                    hitPosition.Fill(0.0f);
                                                    DAT_10daaa80.Start(0x665, hitPosition, -1, 0, 0, 0, 0, &hitMatrix, 0, 0);
                                                }
                                                if (options->settings[5] == 0)
                                                {
                                                    if (immune == 0 || DAT_1042d9e8[slot] != 0 || DAT_10c66cc0 != 2 ||
                                                        DAT_10c635f8 != 2)
                                                    {
                                                        if (slot != 0)
                                                        {
                                                            Vec4 away = DAT_10db6ee8[slot].matrix.GetTranslation() -
                                                                        DAT_10db6ee8[0].matrix.GetTranslation();
                                                            if (away.x != 0.0f || away.y != 0.0f || away.z != 0.0f)
                                                            {
                                                                away.Copy(away.Normalized3() * 15.0f);
                                                                DAT_10db6ee8[slot].SetPendingPush(away.x, away.y, 0, 4);
                                                            }
                                                        }
                                                        DAT_102550e0[slot] = 0x1e;
                                                        if (DAT_10c635f8 == 2 && DAT_10c66cc0 == 2)
                                                            DAT_102550e0[slot] = 4;
                                                    }
                                                    if (DAT_10255a88[slot] != 0)
                                                        DAT_10db6ee8[slot].mode = 1;
                                                }
                                                else
                                                {
                                                    Vec4 playerPosition;
                                                    playerPosition.Copy(DAT_10db9210.GetTranslation());
                                                    float facing = (float)atan2((double)(playerPosition.x - center.x),
                                                                                (double)(playerPosition.y - center.y));
                                                    float diff = facing - DAT_10db6ee8[slot].heading;
                                                    while (diff > PI_APPROX)
                                                        diff -= TWO_PI_APPROX;
                                                    while (diff < -PI_APPROX)
                                                        diff += TWO_PI_APPROX;
                                                    if ((options->settings[4] & STRIKE_PIERCE) != 0 &&
                                                        (DAT_1032d6a4 == 0 || DAT_1032d6a4 > 8))
                                                        DAT_104c0155 = 1;
                                                    if ((options->settings[4] & STRIKE_SLASH) != 0 && DAT_10db6ee8[0].m_238c == 0)
                                                        DAT_104c0155 = 1;
                                                    if (DAT_102239d0[slot] == 0 &&
                                                        (immune == 0 || DAT_1042d9e8[slot] != 0 || DAT_10c66cc0 != 2 ||
                                                         DAT_10c635f8 != 2))
                                                    {
                                                        FUN_10011b70(options->settings[0], options->settings[1], slot, &center);
                                                        DAT_102550e0[slot] = 0x1e;
                                                        if (DAT_10c635f8 == 2 && DAT_10c66cc0 == 2)
                                                            DAT_102550e0[slot] = 4;
                                                        if (DAT_10255a88[slot] != 0)
                                                            DAT_10db6ee8[slot].mode = 1;
                                                    }
                                                    else if (DAT_102239d0[slot] != 0)
                                                    {
                                                        // A fading enemy keeps its resting place and spins out.
                                                        if (DAT_10255a88[slot] == 0)
                                                        {
                                                            FUN_10011b70(options->settings[0], options->settings[1], slot,
                                                                         &center);
                                                            DAT_102550e0[slot] = 0x1e;
                                                            if (DAT_10c635f8 == 2 && DAT_10c66cc0 == 2)
                                                                DAT_102550e0[slot] = 4;
                                                        }
                                                        else if (immune == 0)
                                                        {
                                                            DAT_102550e0[slot] = 0x1e;
                                                            if (DAT_10c635f8 == 2 && DAT_10c66cc0 == 2)
                                                                DAT_102550e0[slot] = 4;
                                                            DAT_108377c8[slot].x = DAT_10db6ee8[slot].matrix._41;
                                                            DAT_108377c8[slot].y = DAT_10db6ee8[slot].matrix._42;
                                                            DAT_108377c8[slot].z = DAT_10db6ee8[slot].matrix._43;
                                                            DAT_10467f28[slot] = 0;
                                                            DAT_10467ba0[slot] = (char)0x9b;
                                                        }
                                                    }
                                                    FUN_1003ad20(0x33a, &DAT_10db6ee8[slot].matrix.GetTranslation(), 1.0f);
                                                    FUN_1003ad20(negativeHit == 0 ? 0x33c : 0x33d,
                                                                 &DAT_10db6ee8[slot].matrix.GetTranslation(), 1.0f);
                                                    if (diff >= 2.3561249f || diff < -2.3561249f)
                                                        DAT_10db6ee8[slot].PlayMotion(0x5c, 2, 1, 1.0f);
                                                    else if (diff < 2.3561249f && diff >= 0.785375f)
                                                        DAT_10db6ee8[slot].PlayMotion(0x5a, 2, 1, 1.0f);
                                                    else if (diff < 0.785375f && diff >= -0.785375f)
                                                        DAT_10db6ee8[slot].PlayMotion(0x5d, 2, 1, 1.0f);
                                                    else if (diff < -0.785375f && diff >= -2.3561249f)
                                                        DAT_10db6ee8[slot].PlayMotion(0x5b, 2, 1, 1.0f);
                                                    if (DAT_10255a88[slot] != 0)
                                                    {
                                                        Vec4 puff;
                                                        puff.Copy(DAT_10db6ee8[slot].matrix.GetTranslation());
                                                        puff.z += 50.0f;
                                                        Vec4 puffDirection = puff - DAT_10836fc8[slot];
                                                        FUN_1000df10(puffDirection.x + 5.0f, puffDirection.y,
                                                                     puffDirection.z + 50.0f, &puff, 100, slot);
                                                        FUN_1000df10(puffDirection.x + 5.0f, puffDirection.y, 0.0f, &puff, 100,
                                                                     slot);
                                                    }
                                                }
                                                if ((negativeHit == 0 && damage != 0 && (DAT_104c02c8 & 0x800) != 0) ||
                                                    (negativeHit != 0 && damage != 0 && (char)DAT_104c02c8 < 0))
                                                {
                                                    int life = DAT_10daaaec + 1;
                                                    if (DAT_10daaaf0 < life)
                                                        life = DAT_10daaaf0;
                                                    if (DAT_10daaaec != life)
                                                        DAT_104c011d = 8;
                                                    DAT_10daaaec = life;
                                                    FUN_1000e750();
                                                }
                                                if (negativeHit != 0 && damage != 0)
                                                    FUN_1003ab40(slot, 0);
                                                alreadyHit = 1;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // The effect also hits the water, the kskon props and the fire and candle lights.
            if ((options->settings[4] & STRIKE_SWORD) == 0 && DAT_104c007c != 0 &&
                center.z < (DAT_104bfe24 - 50.0f) + size &&
                DAT_104b7200.PointInRect((int)center.x, (int)center.y, (int)((float)(int)DAT_104d29e8.x - size),
                                         (int)((float)(int)DAT_104d29e8.y - size),
                                         (int)((float)(int)DAT_104d29f8.x + size),
                                         (int)((float)(int)DAT_104d29f8.y + size)))
            {
                char element = options->m_000e;
                if (element == ELEMENT_ICE)
                {
                    if (DAT_104bf8e4 == 0 && DAT_104c02dc == 0)
                    {
                        FUN_1000dab0(0, 100, 0, 0xffff00);
                        FUN_1000dab0(0, 0, 0xf, 0xffff00);
                        Vec4 position;
                        for (int i = HIT_FIRST_ENEMY_SLOT; i < HIT_END_OBJECT_SLOT; i++)
                        {
                            if (DAT_1021f3c0[i] == HIT_KIND_ENEMY && DAT_10daaae8[i].stats[STAT_HEALTH] > 0)
                            {
                                position.Copy(DAT_10db6ee8[i].matrix.GetTranslation());
                                if (position.z < DAT_104bfe24 &&
                                    DAT_104b7200.PointInRect((int)center.x, (int)center.y,
                                                             (int)((float)(int)DAT_104d29e8.x - size),
                                                             (int)((float)(int)DAT_104d29e8.y - size),
                                                             (int)((float)(int)DAT_104d29f8.x + size),
                                                             (int)((float)(int)DAT_104d29f8.y + size)))
                                {
                                    int hurt;
                                    if ((DAT_10daaae8[i].m_0144 & 4) == 0)
                                    {
                                        hurt = options->m_0004;
                                        if (hurt < 0)
                                            hurt = -hurt;
                                    }
                                    else
                                    {
                                        FUN_10011aa0(i, 1);
                                        hurt = 1;
                                    }
                                    if (DAT_104686b0[i] == 0)
                                    {
                                        DAT_104686b0[i] = 8;
                                        FUN_100119d0(i, hurt, 0);
                                        DAT_102550e0[i] = 0x1e;
                                        if (DAT_10c635f8 == 2 && DAT_10c66cc0 == 2)
                                            DAT_102550e0[i] = 4;
                                        int health = DAT_10daaae8[i].stats[STAT_HEALTH];
                                        DAT_10c64020 += hurt;
                                        DAT_10daaae8[i].stats[STAT_HEALTH] = health - hurt;
                                        DAT_1021f6c0[i] = 0;
                                        DAT_10219770[i] = 300;
                                        DAT_10daaae8[i].stats[STAT_COIN_BONUS] = (unsigned)(health * 2 <= hurt);
                                        if (DAT_10315350[i] < 0xc)
                                        {
                                            Vec4 startPosition = DAT_10db6ee8[i].matrix.GetTranslation();
                                            startPosition.z += 50.0f;
                                            DAT_10daaa80.Start(0x834, startPosition, -1, 0, &DAT_10db6ee8[i].matrix, 0, 0, 0,
                                                               0, 0);
                                        }
                                        int partner = DAT_10daaae8[i].m_00a8;
                                        DAT_10315350[i] = 0xf;
                                        if (partner != 0)
                                            DAT_10315350[DAT_10daaae8[i].m_00a4] = 0xf;
                                    }
                                }
                            }
                        }
                    }
                    return 1;
                }
                if (element == ELEMENT_THUNDER)
                {
                    if (DAT_104c0080 != 0 && DAT_104c02dc == 0)
                    {
                        FUN_1000db30(0, 100, 0);
                        FUN_10011fb0(1);
                        FUN_1000db30(0, 0, 0xf);
                    }
                    return 1;
                }
                if (element == ELEMENT_FIRE)
                {
                    if (DAT_104c0080 != 0 && DAT_104c02dc != 0)
                    {
                        FUN_1000dab0(0, 100, 0, 0xff5050);
                        FUN_10011fb0(0);
                        FUN_1000dab0(0, 0, 0xf, 0xff5050);
                    }
                    return 1;
                }
            }
            if (options->m_000e == ELEMENT_THUNDER || options->m_000e == ELEMENT_FIRE)
            {
                for (int i = 0; i < DAT_1021b3c0; i++)
                {
                    if (DAT_1049f0f8[i]->worldMatrix.GetTranslation().Distance(center) < size + 20.0f)
                    {
                        if (options->m_000e == ELEMENT_FIRE && DAT_10254838[i] < 1000)
                        {
                            FUN_10057c40(DAT_10254838[i] % 1000, DAT_1049f0f8[i], DAT_10254838[i] % 1000 >= 0x100);
                            const char *name = (const char *)DAT_1049f0f8[i];
                            int offset = 0;
                            if (name[1] == 'D' || name[1] == 'd')
                                offset = 6;
                            if (name[1] == 'V' || name[1] == 'v')
                                offset = 7;
                            if (name[offset] == 'C' || name[offset] == 'c')
                                offset += 10;
                            else if (name[offset] == 'F' || name[offset] == 'f')
                                offset += 9;
                            else
                                continue;
                            DAT_10874848.RaiseFireEvents(atol(name + offset), 1);
                        }
                        if (options->m_000e == ELEMENT_THUNDER && DAT_10254838[i] > 999)
                        {
                            FUN_10057b90(DAT_10254838[i] % 1000);
                            const char *name = (const char *)DAT_1049f0f8[i];
                            int offset = 0;
                            if (name[1] == 'D' || name[1] == 'd')
                                offset = 6;
                            if (name[1] == 'V' || name[1] == 'v')
                                offset = 7;
                            if (name[offset] == 'C' || name[offset] == 'c')
                                offset += 10;
                            else if (name[offset] == 'F' || name[offset] == 'f')
                                offset += 9;
                            else
                                continue;
                            DAT_10874848.RaiseFireEvents(atol(name + offset), 0);
                        }
                    }
                }
            }

            // The sword also breaks the kskon props: each takes counted hits and, when spent, is replaced by its broken model.
            if ((options->settings[4] & STRIKE_SWORD) != 0)
            {
                Matrix4 propMatrix;
                propMatrix.SetIdentity();
                Vec4 normal;
                for (int prop = 0; prop < HIT_PROP_COUNT; prop++)
                {
                    if (DAT_1021fa40[prop] == 0 || (options->settings[4] & STRIKE_SWORD) == 0)
                        continue;
                    Vec4 propPosition = DAT_1020af20[prop]->matrix.GetTranslation();
                    if (propPosition.DistanceSquared(center) >= PROP_REACH_SQUARED)
                        continue;
                    float depth;
                    if (DAT_1021fa40[prop]->SphereHitsMesh(&propMatrix, &center, size, &normal, &depth) == 0)
                        continue;
                    if ((options->settings[4] & STRIKE_HEAVY) == 0 && DAT_10253e58[prop] != 1)
                    {
                        if (DAT_104c01a0 == 0)
                        {
                            StartParryFlash(center, DAT_1020af20[prop]->matrix);
                            FUN_10011b70(0xc, 4, 0, &center);
                            DAT_104c01a0 = 10;
                            DAT_10f08008.Rumble(1.0f, 100);
                            DAT_10219970[prop] = 6;
                        }
                    }
                    else if (DAT_10219970[prop] == 0)
                    {
                        DAT_10219970[prop] = 4;
                        DAT_10255a48[prop]--;
                        if (DAT_10253e58[prop] == 1)
                            DAT_10255a48[prop] = 0;
                        if ((options->settings[4] & STRIKE_HEAVY) == 0)
                            DAT_10f08008.Rumble(1.8f, 100);
                        else
                            DAT_10f08008.Rumble(1.0f, 200);
                        if ((options->settings[4] & STRIKE_DOUBLE) != 0)
                        {
                            DAT_10255a48[prop]--;
                            if (DAT_10253e58[prop] == 1)
                                DAT_10255a48[prop] = 0;
                        }
                        if ((options->settings[4] & STRIKE_HEAVY) == 0)
                            DAT_10255a48[prop] = 0;
                        FUN_1003ad20(0x339, &DAT_1020af20[prop]->matrix.GetTranslation(), 1.0f);
                        Vec4 sparkPosition = DAT_1020af20[prop]->matrix.GetTranslation();
                        sparkPosition.z += 100.0f;
                        DAT_10daaa80.Start(0x28a, sparkPosition, -1, 0, 0, 0, 0, 0, 0, 0);
                        if (negativeHit != 0)
                        {
                            DAT_10255a48[prop]--;
                            DAT_10db6ee8[HIT_BONUS_PROP_SLOT + prop % 3].matrix = DAT_1020af20[prop]->matrix;
                            FUN_1003ab40(HIT_BONUS_PROP_SLOT + prop % 3, 0);
                        }
                        if (DAT_10255a48[prop] <= 0)
                        {
                            if (DAT_10253e58[prop] == 0)
                                DAT_10c64ad8 += 4;
                            else
                                DAT_10c64ad8 += 2;
                            FUN_1000e750();
                            if (DAT_10253e58[prop] == 0)
                                FUN_1016b230("656565");
                            FUN_1003ad20(0x337, &DAT_1020af20[prop]->matrix.GetTranslation(), 1.0f);
                            Vec4 where = DAT_1020af20[prop]->matrix.GetTranslation();
                            // The original calls Matrix4 member 0x101b0b10 (another heading of the matrix, atan2 of a transformed axis); matrix4.h does
                            // not declare it yet, so the sibling M_101b0a80 stands in for it.
                            float yaw = DAT_1020af20[prop]->matrix.M_101b0a80() + 1.57075f;
                            char name[0x100];
                            sprintf(name, "ksk%02d.it3", DAT_1021d0b8[prop]);
                            Vec4 tint;
                            tint.w = 1.0f;
                            // The map's lighting tints the broken model.
                            if (DAT_10c655e0 == 0)
                            {
                                if (DAT_10c62ed0 == 0x66 || DAT_10c62ed0 == 0x67 || DAT_10c62ed0 == 0x68)
                                    tint.Set(0.9914519786834717f, 1.0f, 0.8376070261001587f, 1.0f);
                            }
                            else if (DAT_10c62ed0 == 0x6f || DAT_10c62ed0 == 0x70 || DAT_10c62ed0 == 0x71)
                                tint.Set(0.8062679767608643f, 1.0f, 1.0f, 1.0f);
                            if (DAT_10c655e0 == 0)
                            {
                                if (DAT_10c62ed0 == 0xcb || DAT_10c62ed0 == 0xcc || DAT_10c62ed0 == 0xcd)
                                    tint.Set(0.8005689978599548f, 1.0f, 0.9629619717597961f, 1.0f);
                            }
                            else if (DAT_10c62ed0 == 0xc9 || DAT_10c62ed0 == 0xca)
                                tint.Set(0.5527070164680481f, 1.0f, 1.0f, 1.0f);
                            if (DAT_10c655e0 == 0)
                            {
                                if (DAT_10c62ed0 == 0x1f7 || DAT_10c62ed0 == 0x1f8)
                                    tint.Set(0.8547009825706482f, 1.0f, 0.8547009825706482f, 1.0f);
                            }
                            else if (DAT_10c62ed0 == 0x1f5 || DAT_10c62ed0 == 0x1f6)
                                tint.Set(1.0f, 0.7521370053291321f, 1.0f, 1.0f);
                            int placed = FUN_1003a880(name, where.x, where.y, where.z, 0, 0, 0,
                                                      ((yaw - 1.57075f) * 180.0f) / PI_APPROX, tint);
                            FUN_1013faf0();
                            if (placed != -1)
                            {
                                int frames = DAT_10edbe08[placed].end - 1;
                                if (frames < 2)
                                    frames = 1;
                                DAT_10467718[prop] = (short)frames;
                            }
                            sparkPosition = DAT_1020af20[prop]->matrix.GetTranslation();
                            sparkPosition.z += 10.0f;
                            DAT_10daaa80.Start(DAT_10253e58[prop] == 1 ? 0x2d4 : 0x2e4, sparkPosition, -1, 0, 0, 0, 0, 0, 0,
                                               0);
                            // The pieces of the prop ("@kskNN") are hidden, the "@kskNN+" pieces of its broken look are kept.
                            sprintf(name, "@ksk%02d", prop);
                            for (int piece = 0; piece < 12; piece++)
                            {
                                Node *hidden = (Node *)DAT_10255f08->M_1019ed10(name, piece);
                                if (hidden == 0)
                                    break;
                                hidden->SetFlags(0x20);
                                hidden->ClearFlags(4);
                            }
                            sprintf(name, "@ksk%02d+", prop);
                            for (int piece = 0; piece < 6; piece++)
                            {
                                Node *kept = (Node *)DAT_10255f08->M_1019ed10(name, piece);
                                if (kept == 0)
                                    break;
                                int length = (int)strlen((const char *)kept);
                                if (((const char *)kept)[length - 1] == '0')
                                    kept->M_10191d90();
                            }
                            int propFlag = DAT_104678a0[prop];
                            if (propFlag != 0 && propFlag <= PROP_FLAG_LIMIT)
                                DAT_10c62edc[propFlag] = 1;
                            Vec4 coinOrigin = DAT_1020af20[prop]->matrix.GetTranslation();
                            ThrowCoins(coinOrigin, DAT_1042da88[prop]);
                            DAT_10874848.RaiseKskonEvents(prop);
                            DAT_1020af20[prop] = 0;
                            DAT_1021fa40[prop] = 0;
                        }
                    }
                }
            }
        }
        else
        {
            if (DAT_104686b0[0] != 0)
                return 0;
            if (DAT_104bfe6c != 0)
                return 0;
            if (DAT_104bfdf0 != 0)
                return 0;
            int hit;
            if (options->settings[3] == 0 || options->m_0034 == 0)
                hit = DAT_10db6ee8[0].root->FindTouchingSphere((int)&DAT_10db9210, &center, size, 0, 0);
            else
                hit = DAT_10db6ee8[0].root->CollectNearSegment((int)&DAT_10db9210, options->m_0034, options->m_0038,
                                                               options->m_0034->w, 0, 0, &center);
            if (hit != 0)
            {
                if (DAT_104c0171 != 0)
                    return hit;
                if (DAT_104c01a4 != 0 && DAT_104c00e0 != 0)
                    return hit;
                float damage;
                if (options->m_000e == ELEMENT_PLAYER_HIT)
                {
                    if ((DAT_104c02c8 & 0x3000) == 0x3000)
                    {
                        if (DAT_10253e98[0] != 0)
                            return hit;
                        if (DAT_104bfe1c != 0)
                            return hit;
                        DAT_10253e98[0] = 0x3c;
                        FUN_10011aa0(0, 1);
                        int life = DAT_10daaaec + 1;
                        if (DAT_10daaaf0 < life)
                            life = DAT_10daaaf0;
                        if (DAT_10daaaec != life)
                            DAT_104c011d = 8;
                        DAT_10daaaec = life;
                        FUN_1000e750();
                        return hit;
                    }
                    if ((DAT_104c02c8 & 0x2000) != 0)
                    {
                        if (DAT_10253e98[0] != 0)
                            return hit;
                        if (DAT_104bfe1c == 0)
                        {
                            DAT_10253e98[0] = 0x3c;
                            FUN_10011aa0(0, 1);
                            return hit;
                        }
                        return hit;
                    }
                    if ((DAT_104c02c8 & 0x1000) != 0)
                    {
                        float scaled = (float)(options->m_0004 - DAT_10daaae8[0].stats[STAT_DEFENCE]) * options->m_0008;
                        if (scaled <= 1.0f)
                            damage = 1.0f;
                        else
                            damage = scaled;
                        FUN_1005dbf0((int)damage);
                        if (DAT_104bfe4c != 0)
                            return hit;
                        if (DAT_104bfe30 != 0)
                            return hit;
                        if (DAT_104bfdc8 != 0)
                            return hit;
                        FUN_10011b70(0xc, 4, 0, &center);
                        if (DAT_104c00e0 != 0)
                        {
                            DAT_104c00e0 = 0;
                            DAT_10db6ee8[0].mode = 1;
                        }
                        return hit;
                    }
                }
                float scaledDamage;
                if (DAT_10c655f0 == 8)
                    scaledDamage = ((float)(options->m_0004 - DAT_10daaae8[0].stats[STAT_DEFENCE]) * options->m_0008) / 2.0f;
                else if (DAT_10c655f0 == 0xb)
                    scaledDamage = ((float)(options->m_0004 - DAT_10daaae8[0].stats[STAT_DEFENCE]) * options->m_0008) / 4.0f;
                else if ((DAT_104c02c8 & 0x40) == 0)
                {
                    if (((unsigned char)DAT_104c02c8 & 0x30) == 0x30)
                        scaledDamage = ((float)(options->m_0004 - DAT_10daaae8[0].stats[STAT_DEFENCE]) * options->m_0008) / 3.0f;
                    else if ((DAT_104c02c8 & 0x20) != 0)
                        scaledDamage = ((float)(options->m_0004 - DAT_10daaae8[0].stats[STAT_DEFENCE]) * options->m_0008) / 2.0f;
                    else if ((DAT_104c02c8 & 0x10) != 0)
                        scaledDamage = (((float)(options->m_0004 - DAT_10daaae8[0].stats[STAT_DEFENCE]) * options->m_0008) * 3.0f) / 4.0f;
                    else
                        scaledDamage = (float)(options->m_0004 - DAT_10daaae8[0].stats[STAT_DEFENCE]) * options->m_0008;
                }
                else
                    scaledDamage = ((float)(options->m_0004 - DAT_10daaae8[0].stats[STAT_DEFENCE]) * options->m_0008) / 4.0f;
                if (scaledDamage <= 1.0f)
                    scaledDamage = 1.0f;
                FUN_1005dbf0((int)scaledDamage);
                if (DAT_104bfe4c != 0)
                    return hit;
                if (DAT_104bfe30 != 0)
                    return hit;
                if (DAT_104bfdc8 != 0)
                    return hit;
                FUN_10011b70(0x18, 5, 0, &center);
                if (DAT_104c00e0 != 0)
                {
                    DAT_104c00e0 = 0;
                    DAT_10db6ee8[0].mode = 1;
                }
                return hit;
            }
        }
    }
    if (options->m_0020 != 0)
    {
        Matrix4 identity;
        identity.SetIdentity();
        Vec4 normal;
        float depth;
        if (DAT_10db6ee8[0].dropCollider->Query(&identity, &center, size, &normal, &depth, "CA", 0) != 0)
        {
            options->m_0030->Copy(normal);
            return -1;
        }
    }
    return 0;
}
