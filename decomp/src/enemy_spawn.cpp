// Enemy waves (0x10049650, called from the scene frame 0x100f5a80): four groups of up to ten enemy slots; while
// fewer than two of a group are in play the next waiting one is brought in with a flash.

#include <math.h>

#include "vec4.h"

#include "matrix4.h"

#include "effect_manager.h"

#include "anim_model.h"

typedef char check_charrecord[sizeof(AnimModel) == 0x249c ? 1 : -1];

#include "char_state.h"

#include "char_tables.h"
#include "char_vars.h"
#include "world.h"
extern "C" int FUN_10043cc0(int index);       // Char_ResetState

// atan2 on floats (an inline helper: both arguments are rounded to float first, the second one first).
inline float Atan2(float y, float x)
{
    return (float)atan2(y, x);
}

// 0x10049650 (Wave_Update). Goes through the first four wave groups: slots of defeated enemies are cleared, the
// others counted as alive. While fewer than two of a group are in the world (x above -20000 or placed), the waiting
// enemy in the earliest slot (unless its talk id has hundreds) is brought in: flagged, put in play, turned to the
// player with idle motion 2 and a flash (effect 0x3f4) where it stands, and its state is reset. The count of living
// wave enemies goes to DAT_10c635a8. Returns 0.
int FUN_10049650(void)
{
    int alive = 0;
    Vec4 position;
    for (int group = 0; group < 4; group++)
    {
        int chosen = -1;
        int earliest = 99;
        int present = 0;
        for (int slot = 0; slot < 10; slot++)
        {
            if (DAT_1049efb0[group][slot] == 0)
                continue;
            int enemy = DAT_1049efb0[group][slot];
            if (DAT_10210d30[enemy] != 0)
            {
                DAT_1049efb0[group][slot] = 0;
                continue;
            }
            alive++;
            if (DAT_10db6ee8[enemy].matrix.m[12] > -20000.0f || DAT_10867e18[enemy].x > -20000.0f)
                present++;
            else if (earliest > slot)
            {
                earliest = slot;
                if (DAT_1020b430[enemy].talk / 100 != 0)
                    chosen = -1;
                else
                    chosen = enemy;
            }
            if (present < 2 && chosen > 0)
            {
                position.Copy(DAT_10db6ee8[chosen].matrix.GetTranslation());
                DAT_10453008[chosen] = -1;
                position.x *= -1.0f;
                position.x -= 100000.0f;
                DAT_10db6ee8[chosen].SetPosition(position.x, position.y, position.z);
                DAT_10db6ee8[chosen].PlayMotion(2, 0, 5, 1.0f);
                DAT_104c4438[chosen].Copy(DAT_10db6ee8[chosen].matrix.GetTranslation());
                DAT_102199b8[chosen] = 2;
                DAT_10210b30[chosen] = 0;
                if (DAT_1020d540[chosen] == 0)
                {
                    DAT_1020d540[chosen] = 1;
                    DAT_1021b4c8[chosen] = (char)(chosen % 5);
                    DAT_10867e18[chosen].Copy(DAT_10db6ee8[chosen].matrix.GetTranslation());
                }
                Matrix4 matrix;
                Vec4 origin;
                matrix.SetIdentity();
                matrix.SetTranslation(position);
                DAT_10daaa80.Start(0x3f4, origin, -1, -1, 0, 0, 0, &matrix, 0, 0);
                float heading = Atan2(DAT_10db6ee8[0].matrix.m[12] - DAT_10db6ee8[chosen].matrix.m[12],
                                      DAT_10db6ee8[0].matrix.m[13] - DAT_10db6ee8[chosen].matrix.m[13]) + 3.1415f;
                DAT_10db6ee8[chosen].targetHeading = heading;
                DAT_10db6ee8[chosen].heading = heading;
                FUN_10043cc0(chosen);
            }
        }
    }
    DAT_10c635a8 = alive;
    return 0;
}
