// Choosing a character's behaviour kind from its script code (called when a character is set up or its script
// changes it): the code's digit pairs select a kind, and a changed kind resets the character's AI state.

#include "last_position.h"
#include <string.h>

#include "vec4.h"
#include "char_state.h"

#include "char_slot.h"

#include "char_tables.h"


// 0x1001b0a0 (Char_SelectKind). Sets the kind of character `index` from its script code: the millions pair picks a
// special kind (some with an alternative when `alternative` is set), the ten-thousands pair 0x44 or 0x59, else the
// hundreds or units pair a kind for characters without a base kind; characters with base kind 0x31 become 0x55
// (unless `quiet`), or with hundreds 3 are reset at once as kind 0x62. When the kind (with its variant) changed,
// the variant is split off and the AI state, the step record and the last position are cleared (state slots 2 and
// 3 survive for kind 0x55 and base kind 0x31). Returns 0.
extern "C" int FUN_1001b0a0(int index, int alternative, int quiet)
{
    int code = DAT_102548c0[index];
    int *kind = &DAT_10daaae8[index].kind;
    int old = *kind;
    int special = code / 1000000 % 100;
    int apply = 1;
    int group = code / 10000 % 100;
    int units = code % 100;
    int hundreds = code / 100 % 100;
    if (special != 0)
    {
        switch (special)
        {
        case 0x32:
            *kind = 0x56;
            break;
        case 0x33:
            *kind = 0x2766;
            break;
        case 0x34:
            *kind = 0x45;
            break;
        case 0x35:
            *kind = 0x45;
            break;
        case 0x36:
            *kind = 0x4e81;
            if (alternative != 0)
                *kind = 0x2771;
            break;
        case 0x37:
            *kind = 0x4e81;
            if (alternative != 0)
                *kind = 0x2771;
            break;
        case 0x38:
            *kind = 0x111d1;
            break;
        case 0x39:
            *kind = 0x111d1;
            break;
        case 0x3a:
            *kind = 0x138e1;
            break;
        case 0x3b:
            *kind = 0x15ff1;
            break;
        case 0x3c:
            *kind = 0x7591;
            if (alternative != 0)
                *kind = 0x9ca1;
            break;
        case 0x3d:
            *kind = 0xc3b1;
            if (alternative != 0)
                *kind = 0xeac1;
            break;
        }
    }
    else if (group != 0)
    {
        switch (group)
        {
        case 1:
            *kind = 0x44;
            break;
        case 2:
            *kind = 0x44;
            break;
        case 3:
            *kind = 0x59;
            break;
        }
    }
    else
    {
        int base = DAT_1030b668[index];
        if (hundreds != 0)
        {
            if (base == -1 || base == 0)
            {
                switch (hundreds)
                {
                case 1:
                    *kind = 0;
                    break;
                case 2:
                    *kind = 0;
                    break;
                case 4:
                    *kind = 0;
                    break;
                case 5:
                    *kind = 0;
                    break;
                case 6:
                    *kind = 0;
                    break;
                case 7:
                    *kind = 0;
                    break;
                case 8:
                    *kind = 0;
                    break;
                case 9:
                    *kind = 0;
                    break;
                case 10:
                    *kind = 0;
                    break;
                case 11:
                    *kind = 0;
                    break;
                case 12:
                    *kind = 0;
                    break;
                case 14:
                    *kind = 0;
                    break;
                case 16:
                    *kind = 0;
                    break;
                case 3:
                    *kind = 0x1ae11;
                    break;
                case 13:
                    *kind = 0x1ae11;
                    break;
                case 15:
                    *kind = 0x18701;
                    break;
                }
            }
            else if (base == 0x31)
            {
                if (hundreds != 3)
                    *kind = base;
                else
                {
                    memset(DAT_10255160[index], 0, sizeof(DAT_10255160[index]));
                    if (*kind == 0x55)
                    {
                        DAT_1020b430[index].state = 0;
                        DAT_1020b430[index].wait = 0;
                        // words 4..15: every variable after `range` (0x0c); talk and range are kept
                        memset(&DAT_1020b430[index].words[4], 0, 12 * sizeof(int));
                        DAT_1021bdf8[index][0] = 0;
                        DAT_1021bdf8[index][1] = 0;
                        memset(&DAT_1021bdf8[index][4], 0, 4 * sizeof(Node *));
                    }
                    else
                    {
                        memset(DAT_1020b430[index].words, 0, sizeof(DAT_1020b430[index].words));
                        memset(DAT_1021bdf8[index], 0, 8 * sizeof(Node *));
                    }
                    DAT_10802bf8[index].position.Fill(0.0f);
                    DAT_10802bf8[index].step.Fill(0.0f);
                    DAT_10255a88[index] = 1;
                    DAT_1020b430[index].talk = 0x3cd;
                    DAT_1020b430[index].behaviour = 0x26;
                    *kind = 0x62;
                    apply = 0;
                }
            }
            else
                *kind = base;
        }
        else if (units != 0)
        {
            if (base == -1 || base == 0)
            {
                switch (units)
                {
                case 1:
                    *kind = 0;
                    break;
                case 2:
                    *kind = 0;
                    break;
                case 3:
                    *kind = 0;
                    break;
                case 4:
                    *kind = 0;
                    break;
                case 8:
                    *kind = 0;
                    break;
                case 10:
                    *kind = 0;
                    break;
                case 11:
                    *kind = 0;
                    break;
                case 12:
                    *kind = 0;
                    break;
                case 13:
                    *kind = 0;
                    break;
                case 17:
                    *kind = 0;
                    break;
                case 5:
                    *kind = 0x1fc31;
                    break;
                case 6:
                    *kind = 0x24a51;
                    break;
                case 16:
                    *kind = 0x24a51;
                    break;
                case 7:
                    *kind = 0x22341;
                    break;
                case 9:
                    *kind = 0x27161;
                    break;
                case 14:
                    *kind = 0x1d521;
                    break;
                case 15:
                    *kind = 0x29871;
                    break;
                }
            }
            else
                *kind = base;
        }
        else if (base != 0)
        {
            if (base == 0x31 && quiet == 0)
                *kind = 0x55;
            else
                *kind = base == -1 ? 0 : base;
        }
    }
    int value = *kind;
    if (DAT_1020b230[index] * 10000 + old != value && apply != 0)
    {
        DAT_1020b230[index] = value / 10000;
        memset(DAT_10255160[index], 0, sizeof(DAT_10255160[index]));
        *kind = value % 10000;
        if (value % 10000 == 0x55 || DAT_1030b668[index] == 0x31)
        {
            DAT_1020b430[index].state = 0;
            DAT_1020b430[index].wait = 0;
            // words 4..15: every variable after `range` (0x0c); talk and range are kept
            memset(&DAT_1020b430[index].words[4], 0, 12 * sizeof(int));
            DAT_1021bdf8[index][0] = 0;
            DAT_1021bdf8[index][1] = 0;
            memset(&DAT_1021bdf8[index][4], 0, 5 * sizeof(Node *));
        }
        else
        {
            for (int i = 0; i < 16; i++)
                DAT_1020b430[index].words[i] = 0;
            memset(DAT_1021bdf8[index], 0, sizeof(DAT_1021bdf8[index]));
        }
        DAT_10802bf8[index].position.Fill(0.0f);
        DAT_10802bf8[index].step.Fill(0.0f);
        DAT_10255a88[index] = 1;
        return 0;
    }
    if (value > 9999)
    {
        DAT_1020b230[index] = value / 10000;
        *kind = value % 10000;
    }
    return 0;
}
