// The host's aim callback for effects (host callback slot 3, DAT_1101de40[EFFECT_HOST_AIM]): picks the enemy a homing
// effect steers at (0x10010040). Called by EffectManager::Step for effects whose target type is
// EFFECT_TARGET_STEER_HOST.

#include "vec4.h"
#include "matrix4.h"

#include "anim_model.h"
#include "anim_step_vars.h"
#include "char_slot.h"
#include "char_tables.h"
#include "depth_sort.h"
#include "effect_manager.h"
#include "render_vars.h"

#define FIRST_ENEMY_SLOT 0x3a            // enemies occupy the character slots 0x3a..0x7b
#define END_ENEMY_SLOT 0x7c
#define ENEMY_SLOT_KIND 3                // DAT_1021f3c0 value of a live enemy
#define BOSS_SLOT_KIND 0x60              // kind a list-1 effect (options->m_000e == 1) may also aim at
#define AIM_LIST_BOSS 1                  // options->m_000e value that allows the BOSS_SLOT_KIND targets
#define REMOVED_X (-50000.0f)            // a slot whose x is below this has been moved out of the world
#define AIM_HEIGHT_OFFSET 60.0f          // the aim point lies this far above the enemy's origin
#define AIM_RANGE_SQUARED 4000000.0f     // an enemy farther than 2000 units is not a candidate
#define AIM_OFF_SCREEN_PENALTY 1000000.0f   // off-screen candidates sort behind the visible ones
#define AIM_VISIBLE_MARGIN 24.0f         // margin of the visible-area test
#define AIM_CANDIDATE_MAX 24             // candidates kept (slots 0x3a..0x7b are tested; the original arrays hold 24)
#define AIM_NONE (-1)

// 0x10010040. Writes into *options->m_0030 the point 60 units above the enemy the effect should steer at, and returns
// the options' target slot (options->m_0014, > 0), -1 when no enemy qualifies and 1 when a new target was picked.
// A target kept from an earlier frame (options->m_0014 != 0) stays while its record's health is not negative and it
// is still in the world. Otherwise every enemy slot is a candidate when it has a model, is a live enemy (or a
// BOSS_SLOT_KIND slot for list-1 effects), shows no display state, is not held and has health (list-1 boss targets
// need none) and lies within 2000 units of (x, y, z); candidates outside the visible area sort after the visible
// ones. The candidate is picked from the distance-sorted list by (int)variant modulo the candidate count.
extern "C" int FUN_10010040(float x, float y, float z, float variant, EffectOptions *options)
{
    Vec4 position;
    position.Set(x, y, z, 0.0f);
    int count = 0;
    int pick = (int)variant;
    if (options->m_0014 != 0)
    {
        int slot = options->m_0014;
        if (DAT_10daaae8[slot].stats[STAT_HEALTH] >= 0 && !(DAT_10db6ee8[slot].matrix._41 < REMOVED_X))
        {
            Vec4 aimPoint = DAT_10db6ee8[slot].matrix.GetTranslation();
            aimPoint.z += AIM_HEIGHT_OFFSET;
            options->m_0030->Copy(aimPoint);
            return options->m_0014;
        }
        options->m_0014 = 0;
    }
    float distances[AIM_CANDIDATE_MAX];
    int slots[AIM_CANDIDATE_MAX];
    for (int slot = FIRST_ENEMY_SLOT; slot < END_ENEMY_SLOT; slot++)
    {
        if (DAT_10db6ee8[slot].root == 0)
            continue;
        if (DAT_1021f3c0[slot] != ENEMY_SLOT_KIND &&
            (options->m_000e != AIM_LIST_BOSS || DAT_1021f3c0[slot] != BOSS_SLOT_KIND))
            continue;
        if (DAT_104686b0[slot] != 0 || DAT_10daaae8[slot].held != 0)
            continue;
        Vec4 target = DAT_10db6ee8[slot].matrix.GetTranslation();
        target.z += AIM_HEIGHT_OFFSET;
        if (options->m_000e != AIM_LIST_BOSS || DAT_1021f3c0[slot] != BOSS_SLOT_KIND)
        {
            if (!(target.x > REMOVED_X) || DAT_10daaae8[slot].stats[STAT_HEALTH] <= 0)
                continue;
        }
        float distance = position.DistanceSquared(target);
        distances[count] = distance;
        if (!(distance < AIM_RANGE_SQUARED))
            continue;
        if (FUN_101ae4d0(&DAT_10db6ee8[slot].matrix.GetTranslation(), &DAT_108392d8, AIM_VISIBLE_MARGIN) == 0)
            distances[count] += AIM_OFF_SCREEN_PENALTY;
        slots[count] = slot;
        count++;
    }
    if (count == 0)
        return AIM_NONE;
    FUN_10165020(distances, 0, 0, slots, count);
    options->m_0014 = slots[pick % count];
    Vec4 aimPoint = DAT_10db6ee8[options->m_0014].matrix.GetTranslation();
    aimPoint.z += AIM_HEIGHT_OFFSET;
    options->m_0030->Copy(aimPoint);
    return 1;
}
