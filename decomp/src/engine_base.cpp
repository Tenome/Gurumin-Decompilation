// Engine base class and small engine entry points.

#include <stdlib.h>

#include "object.h"

#include "anim_model.h"

extern "C" void FUN_101b21c0(void);
extern "C" void FUN_1003ad20(int, int, float);
#include "char_vars.h"
#include "render_vars.h"
#include "sound_vars.h"
#include "ui_vars.h"

extern "C" int FUN_10001b70(int);

// Empty base class constructor.
Object::Object()
{
}

// 0x1013eda0: empty destructor. The linker folded every identical empty destructor into this one
// (Matrix4, Object, ...), which is why many unrelated classes point here.
Object::~Object()
{
}

// Stub that always returns 0 (an engine hook the game does not use).
extern "C" int FUN_1013edb0(void)
{
    return 0;
}

// Stub that always returns 1.
extern "C" int FUN_1013eed0(void)
{
    return 1;
}

// Engine shutdown step: calls the engine library cleanup (0x101b21c0).
extern "C" int FUN_1013e6d0(void)
{
    FUN_101b21c0();
    return 1;
}

// 0x1013e6e0. The mouse object's wheel callback (Obj::m_003c, set up with the cursor callback 0x1000fd50): the wheel
// movement the menu windows saved (DAT_10f657b8) while there is one, else the wheel movement collected by the
// input update (DAT_10f6555c). Obj::Update turns the sign of the result into its wheel direction.
extern "C" int FUN_1013e6e0(void)
{
    if (DAT_10f657b8 != 0)
        return DAT_10f657b8;
    return DAT_10f6555c;
}

// Starts the game-over sequence: plays sound effect 0x355, lowers the music unless muted (DAT_104c01b0),
// stores `value`, sets the scene timer and plays the player's defeat motion (0x59).
extern "C" int FUN_1003b670(int value)
{
    FUN_1003ad20(0x355, 0, 1.0f);
    if (DAT_104c01b0 == 0)
        FUN_10001b70((100 - (DAT_102027d8 * 5 + 50) / 2) * 100);
    DAT_104c0198 = value;
    DAT_104c00d0 = 0x816;
    DAT_10db6ee8[0].PlayMotion(0x59, 1, 5, 1.0f);
    rand();
    return 0;
}
