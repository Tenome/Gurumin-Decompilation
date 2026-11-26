// The player taking damage (0x1005dbf0, called by enemy attacks, traps and map hazards): health and the anti-tamper
// copies, the hurt voice, rumble, the game over movie and the hit effect.

#include "mmv_player.h"
#include "vec4.h"

#include "matrix4.h"
#include "char_tables.h"

struct Matrix4;
#include "effect_manager.h"

#include "anim_model.h"

#include "char_slot.h"

#include "texture_object.h"

#include "dinput_min.h"

#include "game.h"

#include "doppler_loops.h"

#include "char_vars.h"
#include "game_vars.h"
#include "map_vars.h"
#include "ui_vars.h"

extern "C" int FUN_1013f880(void);                                          // the game's random number
extern "C" int FUN_100119d0(int index, int damage, int a);                  // EventSlots_Add: shows the damage
extern "C" void FUN_1000e750(void);                                         // AntiTamper_Store
extern "C" const char *FUN_1013f8a0(const char *format, ...);               // builds a data path

// 0x1005dbf0 (Player_TakeDamage). Takes `damage` from the player's health unless a scene, menu or other pause
// state blocks it (none while invulnerable; at most 1 in costume 12, two thirds with DAT_104c0093). A hit plays a
// hurt voice (0x76..0x82, or 0x8d.. per face variant) and rumbles, shows the damage and lowers DAT_10c64ad8 by the
// damage's share of health (at least 2). At 0 health an extra life (DAT_104c00cc) refills it, else the game over
// movie and sound 0x335 start. Always starts the hit effect 0xc 50 units above the player. Returns 0.
extern "C" int FUN_1005dbf0(int damage)
{
    if (DAT_104bfe78 != 0)
        return 0;
    if (DAT_104bfe1c != 0)
        return 0;
    if (DAT_104bfd78 != 0)
        return 0;
    if (DAT_1032d69c == 0)
        return 0;
    if (DAT_104bfe6c != 0)
        return 0;
    if (DAT_104c00c8 != 0)
        return 0;
    if (DAT_104c00cc == 2)
        return 0;
    if (DAT_104c00d0 != 0)
        return 0;
    if (DAT_104c0135 != 0)
        return 0;
    if (DAT_104c00b8 != 0)
        damage = 0;
    else if (damage > 0)
    {
        if (DAT_10c655f0 == 12)
            damage = 1;
        if (DAT_104c033a == 0)
        {
            const Vec4 *position = &(const Vec4 &)DAT_10db6ee8[0].matrix.GetTranslation();
            int voice = FUN_1013f880() % 13 + 0x76;
            if (DAT_104c0332 == 0)
                FUN_1003ad20(voice, position, 1.0f);
        }
        else
        {
            const Vec4 *position = &(const Vec4 &)DAT_10db6ee8[0].matrix.GetTranslation();
            int voice = FUN_1013f880() % 5 + 0x8d + DAT_104c033a * 15;
            if (DAT_104c0332 == 0)
                FUN_1003ad20(voice, position, 1.0f);
        }
    }
    if (DAT_104c0093 != 0)
    {
        damage = damage * 2 / 3;
        if (damage == 0)
            damage = 1;
    }
    if (damage != 0)
        DAT_10f08008.Rumble(1.0f, 100);
    FUN_100119d0(0, damage, 2);
    int gauge = DAT_10c64ad8;
    DAT_10daaae8[0].stats[STAT_HEALTH] -= damage;
    int maxHealth = DAT_10daaae8[0].stats[STAT_MAX_HEALTH];
    if (DAT_10c66d00 == 0 && damage != 0)
    {
        int loss = damage * 100 / (maxHealth / (DAT_10c64ad4 + 1));
        if (loss <= 2)
            loss = 2;
        gauge -= loss;
    }
    DAT_10c64ad8 = gauge;
    FUN_1000e750();
    if (DAT_10daaae8[0].stats[STAT_HEALTH] <= 0)
    {
        if (DAT_104c00cc != 0)
        {
            DAT_104c00cc = 2;
            DAT_10daaae8[0].stats[STAT_HEALTH] = maxHealth;
            DAT_10874848.RaiseGameOverEvents();
            FUN_1000e750();
        }
        else
        {
            DAT_10f08ab0.Open(FUN_1013f8a0("gameover.mmv", 0));
            DAT_10f08ab0.DrawToTexture(&DAT_118d0ba8[DAT_10202478]);
            FUN_1003ad20(0x335, &(const Vec4 &)DAT_10db6ee8[0].matrix.GetTranslation(), 1.0f);
            DAT_10f08ab0.state = MMV_PLAYING;
            DAT_10f08ab0.loop = 0;
            DAT_104bfe78 = 1;
            DAT_10db6ee8[0].PlayMotion(0x84, 1, 5, 1.0f);
        }
    }
    DAT_104686b0[PLAYER_SLOT] = 60;
    DAT_1020d4c0[PLAYER_SLOT] = 4;
    Vec4 position = DAT_10db6ee8[0].matrix.GetTranslation();
    position.z += 50.0f;
    DAT_10daaa80.Start(0xc, position, -1, 0, 0, 0, 0, 0, 0, 0);
    return 0;
}
