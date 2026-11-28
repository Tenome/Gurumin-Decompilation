// Voice arbitration shared by the boss kinds (kinds 100, 0x46, 0x69, 0x2d, 0x3f, 0x43). A boss says a line only when
// the voice counter of its neighbouring boss is clear, then blocks that counter for a number of frames. The original
// expands this at every site that says a line (bs001_update.cpp: fade, stand and shout lines).
#ifndef GURUMIN_BOSS_VOICE_H
#define GURUMIN_BOSS_VOICE_H

#include "char_slot.h"
#include "char_state.h"
#include "char_vars.h"
#include "hop_spit_enemy.h"

// Says voice line `sound` of boss `index` (character sound, limited to one at a time) unless the counter that guards
// its kind is running, then starts the counter at `cooldown` frames. Kinds without a counter say nothing.
static __forceinline void BossSayLine(int index, int sound, int cooldown)
{
    int kind = DAT_10daaae8[index].kind;
    int word;
    if (kind == 100)
        word = 10;
    else if (kind == 0x46)
        word = 0xb;
    else if (kind == 0x69)
        word = 8;
    else if (kind == 0x2d)
        word = 1;
    else if (kind == 0x3f)
        word = 9;
    else if (kind == 0x43)
        word = 0xf;
    else
        return;
    // The counter that is tested: the golem and the prince keep it in their own slot, the others in a neighbour's.
    int busy;
    if (kind == 0x3f || kind == 0x43)
        busy = DAT_1020b430[index].words[word];
    else if (kind == 0x2d)
        busy = DAT_1020b430[index + 2].words[word];
    else if (kind == 0x69)
        busy = DAT_1020b430[index - 1].words[word];
    else
        busy = DAT_1020b430[index + 1].words[word];
    if (busy != 0)
        return;
    FUN_10065b10(index, sound, 1, 1);
    DAT_1020b430[index + 1].words[word] = cooldown;
}

#endif
