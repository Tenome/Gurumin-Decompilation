// Anti-cheat guards (docs/MODDING_MAP.md "Anti-cheat").
// The game keeps disguised copies of important values (player stats, money, costume id, inventory counts) in
// DAT_104684b0: copy[i] = ~value + i * 0x41. Every frame the checker compares the copies with the real values;
// a mismatch (a memory editor changed a value) drops random noise into the table DAT_104c0270, which later code
// reads to corrupt the game quietly. Mods that change these values must call AntiTamper_StoreArrays afterwards,
// or patch the checker.

#include <stdlib.h>

#include "char_slot.h"

#include "char_vars.h"
#include "game_vars.h"

// Disguised copy of `value` stored in guard slot `index`.
#define GUARD_VALUE(index, value) (~(value) + (index) * 0x41)

// AntiTamper_StoreValues: refreshes the guard copies of the ten single values (slots 0..9).
extern "C" int FUN_1000e750(void)
{
    DAT_104684b0[0] = GUARD_VALUE(0, DAT_10daaae8[0].stats[STAT_PICKUPS]);
    DAT_104684b0[1] = GUARD_VALUE(1, DAT_10daaae8[0].stats[STAT_GUARDED_B]);
    DAT_104684b0[2] = GUARD_VALUE(2, DAT_10daaae8[0].stats[STAT_HEALTH]);
    DAT_104684b0[3] = GUARD_VALUE(3, DAT_10daaae8[0].stats[STAT_MAX_HEALTH]);
    DAT_104684b0[4] = GUARD_VALUE(4, DAT_10daaae8[0].money);
    DAT_104684b0[5] = GUARD_VALUE(5, DAT_10c6670c);
    DAT_104684b0[6] = GUARD_VALUE(6, DAT_10c64ad4);
    DAT_104684b0[7] = GUARD_VALUE(7, DAT_10c64ad8);
    DAT_104684b0[8] = GUARD_VALUE(8, DAT_10c635f8);
    DAT_104684b0[9] = GUARD_VALUE(9, DAT_10c655f0);
    return 0;
}


// AntiTamper_StoreArrays: refreshes the copies of the 0x50 inventory counts (slots 10..89) and of 15 more
// values (slots 100..114). Called after every inventory change.
extern "C" int FUN_1000e810(void)
{
    int i;
    for (i = 0; i < 0x50; i++)
        DAT_104684b0[10 + i] = ~DAT_103150c0[i] + 0x14 + i * 0x41;
    for (i = 0; i < 15; i++)
        DAT_104684b0[100 + i] = ~DAT_10c66144[i] + 6500 + i * 2;
    return 0;
}


// Penalty: writes a random value into a random entry of the 16-entry noise table.
inline void TAMPER()
{
    DAT_104c0270[rand() % 16] = rand();
}

// AntiTamper_Check: counts frames, penalises an active debugger flag, compares every guard copy with its value
// (hp and max hp mismatches are also counted in DAT_104c0344) and every 30 frames checks the array copies.
extern "C" int FUN_1000e870(void)
{
    DAT_104c034c++;
    if (DAT_11b044f0 != 0)
        TAMPER();
    if (DAT_104684b0[0] != GUARD_VALUE(0, DAT_10daaae8[0].stats[STAT_PICKUPS]))
        TAMPER();
    if (DAT_104684b0[1] != GUARD_VALUE(1, DAT_10daaae8[0].stats[STAT_GUARDED_B]))
        TAMPER();
    if (DAT_104684b0[2] != GUARD_VALUE(2, DAT_10daaae8[0].stats[STAT_HEALTH]))
    {
        TAMPER();
        DAT_104c0344++;
    }
    if (DAT_104684b0[3] != GUARD_VALUE(3, DAT_10daaae8[0].stats[STAT_MAX_HEALTH]))
    {
        TAMPER();
        DAT_104c0344++;
    }
    if (DAT_104684b0[4] != GUARD_VALUE(4, DAT_10daaae8[0].money))
        TAMPER();
    if (DAT_104684b0[5] != GUARD_VALUE(5, DAT_10c6670c))
        TAMPER();
    if (DAT_104684b0[6] != GUARD_VALUE(6, DAT_10c64ad4))
        TAMPER();
    if (DAT_104684b0[7] != GUARD_VALUE(7, DAT_10c64ad8))
        TAMPER();
    if (DAT_104684b0[8] != GUARD_VALUE(8, DAT_10c635f8))
        TAMPER();
    if (DAT_104684b0[9] != GUARD_VALUE(9, DAT_10c655f0))
        TAMPER();
    if (DAT_104bf890 % 30 == 0)
    {
        for (int i = 0; i < 0x50; i++)
        {
            if (DAT_104684b0[10 + i] != ~DAT_103150c0[i] + 0x14 + i * 0x41)
                TAMPER();
        }
    }
    if (DAT_104bf890 % 30 == 14)
    {
        for (int i = 0; i < 15; i++)
        {
            if (DAT_104684b0[100 + i] != ~DAT_10c66144[i] + 6500 + i * 2)
                TAMPER();
        }
    }
    DAT_104c034c--;
    return 0;
}
