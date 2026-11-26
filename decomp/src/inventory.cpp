// Item table at 0x1088e6b0 (0xc0 bytes per item). type 4 = stackable consumable.
#include "item_info.h"
#include "game_vars.h"

extern "C" int FUN_1000e810(void);

// Inventory_Add: adds *count of item *item. Most items stack to 3; item 0x10 and type-4 items stack to 99.
// When the stack is full the excess stays in *count and 0 is returned; otherwise *count and *item are cleared
// and 1 is returned. Marks the item as obtained (DAT_1042de28) and refreshes the anti-tamper copies
// (AntiTamper_StoreArrays 0x1000e810).
extern "C" int FUN_10012130(int *item, int *count)
{
    int id = *item;
    if (id != 0x10 && DAT_1088e6b0[id].type != 4)
    {
        if (DAT_103150c0[id] + *count > 3)
        {
            *count = DAT_103150c0[id] + *count - 3;
            DAT_103150c0[*item] = 3;
            DAT_1042de28[*item] = 1;
            FUN_1000e810();
            return 0;
        }
    }
    if (DAT_103150c0[id] + *count > 99)
    {
        *count = DAT_103150c0[id] + *count - 99;
        DAT_103150c0[*item] = 99;
        DAT_1042de28[*item] = 1;
        FUN_1000e810();
        return 0;
    }
    DAT_1042de28[id] = 1;
    DAT_103150c0[*item] += (char)*count;
    *count = 0;
    *item = 0;
    FUN_1000e810();
    return 1;
}