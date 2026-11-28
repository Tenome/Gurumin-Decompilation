// Enemy voices with a cooldown (called by the enemy behaviours): each kind of talking enemy keeps a cooldown counter
// in one of its AI state slots, and a voice only plays while that counter is zero.

// A character slot record (world.h CharSlot): only the kind is used here.
#include "char_state.h"

#include "char_slot.h"

// The cooldown counters of the different enemy kinds live in the behaviour state words of the enemy's own slot
// or of the slot before or after it (char_state.h).

#include "char_vars.h"

extern "C" int FUN_10065b10(int index, int sound, int always, int optional);  // Character_PlaySound

// 0x100b9bb0 (Enemy_PlayVoice). Plays voice `sound` of enemy `index` unless its cooldown is running (`limited`
// voices are skipped entirely unless DAT_10c66cc0 is set). The kinds 100, 0x46, 0x69, 0x2d, 0x3f and 0x43 keep their
// cooldown in slot 10, 11, 8, 1, 9 and 15 of one of the state views; other kinds never speak. `reset` clears the
// cooldown first. After playing, the cooldown is set by the sound (15 to 285 frames). Returns 0.
extern "C" int FUN_100b9bb0(int index, int sound, int reset, int limited)
{
    if (DAT_10c66cc0 == 0 && limited != 0)
        return 0;
    int kind = DAT_10daaae8[index].kind;
    int slot;
    if (kind == 100)
        slot = 10;
    else if (kind == 0x46)
        slot = 11;
    else if (kind == 0x69)
        slot = 8;
    else if (kind == 0x2d)
        slot = 1;
    else if (kind == 0x3f)
        slot = 9;
    else if (kind == 0x43)
        slot = 15;
    else
        return 0;
    if (reset != 0)
    {
        if (kind == 0x69)
            DAT_1020b430[index - 1].words[slot] = 0;
        else if (kind == 0x2d)
            DAT_1020b430[index + 2].words[slot] = 0;
        else if (kind == 0x3f || kind == 0x43)
            DAT_1020b430[index].words[slot] = 0;
        else
            DAT_1020b430[index + 1].words[slot] = 0;
    }
    if ((kind == 0x3f || kind == 0x43) && DAT_1020b430[index].words[slot] == 0)
    {
    }
    else if (kind == 0x2d && DAT_1020b430[index + 2].words[slot] == 0)
    {
    }
    else if (kind == 0x69 && DAT_1020b430[index - 1].words[slot] == 0)
    {
    }
    else
    {
        if (kind == 0x69 || kind == 0x2d || kind == 0x3f || kind == 0x43)
            return 0;
        if (DAT_1020b430[index + 1].words[slot] != 0)
            return 0;
    }
    FUN_10065b10(index, sound, 1, 1);
    switch (sound)
    {
    case 0x20:
        DAT_1020b430[index - 1].words[slot] = 0x1e;
        return 0;
    case 0x21:
        DAT_1020b430[index - 1].words[slot] = 0x1e;
        return 0;
    case 0x22:
        DAT_1020b430[index - 1].words[slot] = 0x1e;
        return 0;
    case 0x23:
        DAT_1020b430[index - 1].words[slot] = 0x1e;
        return 0;
    case 0x24:
        DAT_1020b430[index - 1].words[slot] = 15;
        return 0;
    case 0x25:
        DAT_1020b430[index - 1].words[slot] = 15;
        return 0;
    case 0x26:
        DAT_1020b430[index - 1].words[slot] = 0x1e;
        return 0;
    case 0x27:
        DAT_1020b430[index - 1].words[slot] = 0x69;
        return 0;
    case 0x28:
        DAT_1020b430[index - 1].words[slot] = 15;
        return 0;
    case 0x29:
        DAT_1020b430[index - 1].words[slot] = 15;
        return 0;
    case 0x2a:
        DAT_1020b430[index - 1].words[slot] = 0x1e;
        return 0;
    case 0x2b:
        DAT_1020b430[index - 1].words[slot] = 0x1e;
        return 0;
    case 0x2c:
        DAT_1020b430[index - 1].words[slot] = 0x14;
        return 0;
    case 0x2d:
        DAT_1020b430[index - 1].words[slot] = 0x1e;
        return 0;
    case 0x2e:
        DAT_1020b430[index - 1].words[slot] = 0x37;
        return 0;
    case 0x2f:
        DAT_1020b430[index - 1].words[slot] = 0x3c;
        return 0;
    case 0x30:
        DAT_1020b430[index - 1].words[slot] = 0x78;
        return 0;
    case 0x31:
        DAT_1020b430[index + 2].words[slot] = 15;
        return 0;
    case 0x32:
        DAT_1020b430[index + 2].words[slot] = 15;
        return 0;
    case 0x33:
        DAT_1020b430[index + 2].words[slot] = 15;
        return 0;
    case 0x34:
        DAT_1020b430[index + 2].words[slot] = 9;
        return 0;
    case 0x35:
        DAT_1020b430[index + 2].words[slot] = 9;
        return 0;
    case 0x36:
        DAT_1020b430[index + 2].words[slot] = 9;
        return 0;
    case 0x37:
        DAT_1020b430[index + 2].words[slot] = 0x2d;
        return 0;
    case 0x38:
        DAT_1020b430[index + 2].words[slot] = 0x3c;
        return 0;
    case 0x39:
        DAT_1020b430[index + 2].words[slot] = 0x3c;
        return 0;
    case 0x3a:
        DAT_1020b430[index + 2].words[slot] = 0x2d;
        return 0;
    case 0x3b:
        DAT_1020b430[index + 2].words[slot] = 0x3c;
        return 0;
    case 0x3c:
        DAT_1020b430[index + 2].words[slot] = 0x14;
        return 0;
    case 0x3d:
        DAT_1020b430[index + 2].words[slot] = 0x14;
        return 0;
    case 0x3e:
        DAT_1020b430[index + 2].words[slot] = 0x14;
        return 0;
    case 0x3f:
        DAT_1020b430[index + 2].words[slot] = 0x78;
        return 0;
    case 0x40:
        DAT_1020b430[index].words[slot] = 0x2d;
        return 0;
    case 0x41:
        DAT_1020b430[index].words[slot] = 0x2d;
        return 0;
    case 0x42:
        DAT_1020b430[index].words[slot] = 0x2d;
        return 0;
    case 0x43:
        DAT_1020b430[index].words[slot] = 0x1e;
        return 0;
    case 0x44:
        DAT_1020b430[index].words[slot] = 0x1e;
        return 0;
    case 0x45:
        DAT_1020b430[index].words[slot] = 0x1e;
        return 0;
    case 0x46:
        DAT_1020b430[index].words[slot] = 0x78;
        return 0;
    case 0x47:
        DAT_1020b430[index].words[slot] = 10;
        return 0;
    case 0x48:
        DAT_1020b430[index].words[slot] = 10;
        return 0;
    case 0x49:
        DAT_1020b430[index].words[slot] = 10;
        return 0;
    case 0x4a:
        DAT_1020b430[index].words[slot] = 15;
        return 0;
    case 0x4b:
        DAT_1020b430[index].words[slot] = 0x19;
        return 0;
    case 0x4c:
        DAT_1020b430[index].words[slot] = 15;
        return 0;
    case 0x4d:
        DAT_1020b430[index].words[slot] = 15;
        return 0;
    case 0x4e:
        DAT_1020b430[index].words[slot] = 0x1e;
        return 0;
    case 0x4f:
        DAT_1020b430[index].words[slot] = 0x78;
        return 0;
    case 0x1e4:
        DAT_1020b430[index + 1].words[slot] = 0x2d;
        return 0;
    case 0x1e5:
        DAT_1020b430[index + 1].words[slot] = 0x87;
        return 0;
    case 0x1e6:
        DAT_1020b430[index + 1].words[slot] = 15;
        return 0;
    case 0x1e7:
        DAT_1020b430[index + 1].words[slot] = 15;
        return 0;
    case 0x1e8:
        DAT_1020b430[index + 1].words[slot] = 15;
        return 0;
    case 0x1e9:
        DAT_1020b430[index + 1].words[slot] = 15;
        return 0;
    case 0x1ea:
        DAT_1020b430[index + 1].words[slot] = 15;
        return 0;
    case 0x1eb:
        DAT_1020b430[index + 1].words[slot] = 0x11d;
        return 0;
    case 0x1ec:
        DAT_1020b430[index + 1].words[slot] = 0x2d;
        return 0;
    case 0x1ed:
        DAT_1020b430[index + 1].words[slot] = 0x2d;
        return 0;
    case 0x1ee:
        DAT_1020b430[index + 1].words[slot] = 0x1e;
        return 0;
    case 0x1ef:
        DAT_1020b430[index + 1].words[slot] = 0x1e;
        return 0;
    case 0x1f0:
        DAT_1020b430[index + 1].words[slot] = 0x1e;
        return 0;
    case 0x1f1:
        DAT_1020b430[index + 1].words[slot] = 15;
        return 0;
    case 0x1f2:
        DAT_1020b430[index + 1].words[slot] = 15;
        return 0;
    case 0x1f3:
        DAT_1020b430[index + 1].words[slot] = 0x2d;
        return 0;
    case 0x1f4:
        DAT_1020b430[index + 1].words[slot] = 0x1e;
        return 0;
    case 0x1f5:
        DAT_1020b430[index + 1].words[slot] = 0x1e;
        return 0;
    case 0x1f6:
        DAT_1020b430[index + 1].words[slot] = 15;
        return 0;
    case 0x1f7:
        DAT_1020b430[index + 1].words[slot] = 15;
        return 0;
    case 0x1f8:
        DAT_1020b430[index + 1].words[slot] = 15;
        return 0;
    case 0x1f9:
        DAT_1020b430[index + 1].words[slot] = 0x3c;
        return 0;
    case 0x1fa:
        DAT_1020b430[index + 1].words[slot] = 10;
        return 0;
    case 0x1fb:
        DAT_1020b430[index + 1].words[slot] = 10;
        return 0;
    case 0x1fc:
        DAT_1020b430[index + 1].words[slot] = 10;
        return 0;
    case 0x1fd:
        DAT_1020b430[index + 1].words[slot] = 15;
        return 0;
    case 0x1fe:
        DAT_1020b430[index + 1].words[slot] = 0x1e;
        return 0;
    case 0x1ff:
        DAT_1020b430[index + 1].words[slot] = 0x4b;
        return 0;
    }
    return 0;
}
