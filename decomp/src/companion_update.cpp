// The player's three companions (run from 0x100d24a0): each companion k of character `index` lives in character
// slot index + 2 + k and keeps its health and revival state in the owner's records. The three updates are the
// same code for k = 0, 1 and 2.

#include "last_position.h"
#include "vec4.h"

#include "matrix4.h"

Matrix4 RotationZ(float angle);                      // 0x101b1b40

#include "node.h"

#include "anim_model.h"
#include "char_state.h"
#include "char_slot.h"

#include "char_tables.h"
#include "char_vars.h"

extern "C" int FUN_10065b10(int index, int sound, int always, int optional);  // Character_PlaySound
extern "C" int FUN_1000e750(void);
extern "C" int FUN_1001d8b0(int index, int effect, Vec4 position, int parent, int unused, const Matrix4 *owner,
                            int value, int frame, const Matrix4 *matrix, const void *options);

// 0x100b0250 (Companion0_Update, a C++ function: it has an EH frame for its matrices). Keeps companion 0 of character `index` (slot index + 2) in range and runs its
// down/revive cycle (state slot 4): 0 = active (checks health, waits outside the areas 0x13 and 0x30, goes down
// below its maximum), 1 = counting to 60 then revives with effect 0x36d2, sound 0x194 and 30 points, 2 = counts
// down to active, 10 and above counts down with effect 0x36d5 at 70 and comes back at 11. Returns 0.
int Companion0_Update(int index)
{
    int slot = index;   // companion 0: the companion's own slot is slot + 2, like its twins Companion1/2
    if (DAT_103153b0[slot + 2] > 15)
        DAT_103153b0[slot + 2] = 15;
    if (DAT_10219770[slot + 2] > 5)
        DAT_10219770[slot + 2] = 5;
    if (DAT_10315350[slot + 2] > 0)
        DAT_10315350[slot + 2] = 0;
    unsigned char level = DAT_102550e0[slot + 2];
    if (level != 0)
    {
        if (DAT_102550e0[index] >= level)
            level = DAT_102550e0[index];
        DAT_102550e0[index] = level;
    }
    int mode = DAT_1020b430[slot + 2].downMode;
    if (mode != 0)
    {
        if (mode == 1)
        {
            DAT_104686b0[slot + 2] = 2;
            if (DAT_1020b430[slot + 2].downTimer == 1)
                FUN_10065b10(index, 0x192, 1, 0);
            if (++DAT_1020b430[slot + 2].downTimer == 0x3c)
            {
                DAT_1020b430[slot + 2].downMode = 0;
                Vec4 position;
                Matrix4 matrix;
                if (DAT_1021bdf8[slot + 2][1] != 0)
                {
                    DAT_1021bdf8[slot + 2][1]->SetFlags(0x20);
                    matrix = DAT_1021bdf8[slot + 2][1]->worldMatrix;
                }
                else
                    matrix = RotationZ(DAT_10db6ee8[index].heading);
                if (DAT_1021bdf8[slot + 2][2] != 0)
                    FUN_1001d8b0(index, 0x36d2, position, -1, -1, &DAT_1021bdf8[slot + 2][2]->worldMatrix, 0, 0, &matrix,
                                 0);
                else
                    FUN_1001d8b0(index, 0x36d2, position, -1, -1, 0, 0, 0, &matrix, 0);
                FUN_10065b10(index, 0x194, 1, 0);
                DAT_10c64ad8 += 0x1e;
                FUN_1000e750();
            }
        }
        else if (mode == 2)
        {
            DAT_104686b0[slot + 2] = 2;
            if (--DAT_1020b430[slot + 2].downTimer == 0)
                DAT_1020b430[slot + 2].downMode = 0;
        }
        else if (mode >= 10)
        {
            DAT_1020b430[slot + 2].downMode = mode - 1;
            if (mode - 1 == 0x46)
            {
                Vec4 position;
                Matrix4 matrix;
                if (DAT_1021bdf8[slot + 2][1] != 0)
                    matrix = DAT_1021bdf8[slot + 2][1]->worldMatrix;
                else
                    matrix = RotationZ(DAT_10db6ee8[index].heading);
                if (DAT_1021bdf8[slot + 2][2] != 0)
                    FUN_1001d8b0(index, 0x36d5, position, -1, -1, &DAT_1021bdf8[slot + 2][2]->worldMatrix, 0, 0, &matrix,
                                 0);
                else
                    FUN_1001d8b0(index, 0x36d5, position, -1, -1, 0, 0, 0, &matrix, 0);
            }
            if (DAT_1020b430[slot + 2].downMode == 0xb)
            {
                if (DAT_1021bdf8[slot + 2][1] != 0)
                {
                    DAT_1021bdf8[slot + 2][1]->ClearFlags(0x20);
                    int health = DAT_10daaae8[slot + 2].stats[STAT_HEALTH] + 200;
                    if (DAT_1020b430[index].companionHealthCap + 2000 <= health)
                        health = DAT_1020b430[index].companionHealthCap + 2000;
                    DAT_1020b430[slot + 2].reviveHealth = health;
                    DAT_10daaae8[slot + 2].stats[STAT_HEALTH] = DAT_10daaae8[slot + 2].stats[STAT_MAX_HEALTH];
                    DAT_10db6ee8[slot + 2].root->M_10194730(0x2f, 0x400);
                    DAT_10daaae8[slot + 2].m_0141 = 0;
                    DAT_1021f3c0[slot + 2] = 3;
                }
                DAT_104686b0[slot + 2] = 2;
                DAT_1020b430[slot + 2].downMode = 2;
            }
        }
    }
    else
    {
        int health = DAT_10daaae8[slot + 2].stats[STAT_HEALTH];
        if (health > 70000)
        {
            int special = DAT_104c00e0;
            int area = DAT_104c00e8;
            if ((special == 0 || (area != 0x13 && area != 0x30)) && DAT_1020b430[slot + 2].hideCount == 0 &&
                DAT_10c655f0 != 6)
            {
                DAT_10daaae8[slot + 2].held = 1;
                DAT_10daaae8[slot + 2].m_0141 = 0;
                DAT_10802bf8[slot + 2].position.z = 0;
            }
            else
            {
                DAT_10daaae8[slot + 2].held = 0;
                DAT_10daaae8[slot + 2].stats[STAT_VALUE_4] = 999;
                DAT_10daaae8[slot + 2].m_0141 = 0;
                if (DAT_1020b430[index].keepCompanionUp == 0)
                    DAT_10802bf8[slot + 2].position.z = -50.0f;
                if (special != 0 && (area == 0x13 || area == 0x30))
                    DAT_1020b430[slot + 2].hideCount = 2;
                else
                    DAT_1020b430[slot + 2].hideCount--;
            }
            if (DAT_1042d9e8[slot + 2] != 0)
            {
                unsigned char shown = DAT_104686b0[slot + 2];
                DAT_1042d9e8[slot + 2] = 0;
                DAT_10daaae8[slot + 2].stats[STAT_HEALTH] = health - 1;
                if (shown > 6)
                    DAT_104686b0[slot + 2] = 6;
            }
            if (DAT_10daaae8[slot + 2].stats[STAT_HEALTH] < DAT_10daaae8[slot + 2].stats[STAT_MAX_HEALTH] - 5 ||
                (DAT_10c635f8 == 1 && DAT_10daaae8[slot + 2].stats[STAT_HEALTH] < DAT_10daaae8[slot + 2].stats[STAT_MAX_HEALTH]))
            {
                int revived = DAT_1020b430[slot + 2].reviveHealth;
                DAT_1020b430[slot + 2].downMode = 1;
                DAT_10daaae8[slot + 2].held = 0;
                if (revived != 0)
                    DAT_10daaae8[slot + 2].stats[STAT_HEALTH] = revived;
                else
                    DAT_10daaae8[slot + 2].stats[STAT_HEALTH] = DAT_10daaae8[index].stats[STAT_MAX_HEALTH] + 2000;
                DAT_10daaae8[slot + 2].stats[STAT_VALUE_4] = DAT_10daaae8[index].stats[STAT_VALUE_4];
                DAT_104686b0[slot + 2] = 0x1e;
                DAT_10802bf8[slot + 2].position.z = 0;
            }
        }
        else if (health > 2000)
        {
            DAT_10daaae8[slot + 2].m_0141 = 0;
            if (DAT_104686b0[slot + 2] == 7)
            {
                if (DAT_1020d4c0[slot + 2] > 4)
                {
                    if (DAT_104c00f8 != 0)
                        DAT_10f08080[index].Copy(DAT_10836c74 - DAT_10db6ee8[index].matrix.GetTranslation());
                    DAT_1020d4c0[slot + 2] = 6;
                }
                else
                {
                    if (DAT_104c00f8 != 0)
                        DAT_10f08080[index].Copy(DAT_10836c74 - DAT_10db6ee8[index].matrix.GetTranslation());
                    DAT_1020d4c0[slot + 2] = 4;
                }
            }
        }
        else
        {
            DAT_10daaae8[slot + 2].stats[STAT_HEALTH] = 2000;
            DAT_10daaae8[slot + 2].m_0141 = 1;
        }
    }
    return 0;
}

// 0x100b0880 (Companion1_Update, a C++ function: it has an EH frame for its matrices). Keeps companion 1 of character `index` (slot index + 3) in range and runs its
// down/revive cycle (state slot 4): 0 = active (checks health, waits outside the areas 0x13 and 0x30, goes down
// below its maximum), 1 = counting to 60 then revives with effect 0x36d1, sound 0x194 and 30 points, 2 = counts
// down to active, 10 and above counts down with effect 0x36d4 at 70 and comes back at 11. Returns 0.
int Companion1_Update(int index)
{
    int slot = index + 1;
    if (DAT_103153b0[slot + 2] > 15)
        DAT_103153b0[slot + 2] = 15;
    if (DAT_10219770[slot + 2] > 5)
        DAT_10219770[slot + 2] = 5;
    if (DAT_10315350[index + 3] > 0)
        DAT_10315350[index + 3] = 0;
    unsigned char level = DAT_102550e0[index + 3];
    if (level != 0)
    {
        if (DAT_102550e0[index] >= level)
            level = DAT_102550e0[index];
        DAT_102550e0[index] = level;
    }
    int mode = DAT_1020b430[slot + 2].downMode;
    if (mode != 0)
    {
        if (mode == 1)
        {
            DAT_104686b0[index + 3] = 2;
            if (DAT_1020b430[slot + 2].downTimer == 1)
                FUN_10065b10(index, 0x192, 1, 0);
            if (++DAT_1020b430[slot + 2].downTimer == 0x3c)
            {
                DAT_1020b430[slot + 2].downMode = 0;
                Vec4 position;
                Matrix4 matrix;
                if (DAT_1021bdf8[slot + 2][1] != 0)
                {
                    DAT_1021bdf8[slot + 2][1]->SetFlags(0x20);
                    matrix = DAT_1021bdf8[slot + 2][1]->worldMatrix;
                }
                else
                    matrix = RotationZ(DAT_10db6ee8[index].heading);
                if (DAT_1021bdf8[slot + 2][2] != 0)
                    FUN_1001d8b0(index, 0x36d1, position, -1, -1, &DAT_1021bdf8[slot + 2][2]->worldMatrix, 0, 0, &matrix,
                                 0);
                else
                    FUN_1001d8b0(index, 0x36d1, position, -1, -1, 0, 0, 0, &matrix, 0);
                FUN_10065b10(index, 0x194, 1, 0);
                DAT_10c64ad8 += 0x1e;
                FUN_1000e750();
            }
        }
        else if (mode == 2)
        {
            DAT_1020b430[slot + 2].downTimer--;
            DAT_104686b0[index + 3] = 2;
            if (DAT_1020b430[slot + 2].downTimer == 0)
                DAT_1020b430[slot + 2].downMode = 0;
        }
        else if (mode >= 10)
        {
            DAT_1020b430[slot + 2].downMode = mode - 1;
            if (mode - 1 == 0x46)
            {
                Vec4 position;
                Matrix4 matrix;
                if (DAT_1021bdf8[slot + 2][1] != 0)
                    matrix = DAT_1021bdf8[slot + 2][1]->worldMatrix;
                else
                    matrix = RotationZ(DAT_10db6ee8[index].heading);
                if (DAT_1021bdf8[slot + 2][2] != 0)
                    FUN_1001d8b0(index, 0x36d4, position, -1, -1, &DAT_1021bdf8[slot + 2][2]->worldMatrix, 0, 0, &matrix,
                                 0);
                else
                    FUN_1001d8b0(index, 0x36d4, position, -1, -1, 0, 0, 0, &matrix, 0);
                FUN_10065b10(index, 0x193, 1, 0);
            }
            if (DAT_1020b430[slot + 2].downMode == 0xb)
            {
                if (DAT_1021bdf8[slot + 2][1] != 0)
                {
                    DAT_1021bdf8[slot + 2][1]->ClearFlags(0x20);
                    int health = DAT_10daaae8[slot + 2].stats[STAT_HEALTH] + 200;
                    if (DAT_1020b430[index].companionHealthCap + 2000 <= health)
                        health = DAT_1020b430[index].companionHealthCap + 2000;
                    DAT_1020b430[slot + 2].reviveHealth = health;
                    DAT_10daaae8[slot + 2].stats[STAT_HEALTH] = DAT_10daaae8[slot + 2].stats[STAT_MAX_HEALTH];
                    DAT_10db6ee8[slot + 2].root->M_10194730(0x2f, 0x400);
                    DAT_10daaae8[slot + 2].m_0141 = 0;
                    DAT_1021f3c0[index + 3] = 3;
                }
                DAT_1020b430[slot + 2].downMode = 2;
                DAT_104686b0[index + 3] = 2;
            }
        }
    }
    else
    {
        int health = DAT_10daaae8[slot + 2].stats[STAT_HEALTH];
        if (health > 70000)
        {
            int special = DAT_104c00e0;
            int area = DAT_104c00e8;
            DAT_10daaae8[slot + 2].m_0141 = 0;
            if ((special == 0 || (area != 0x13 && area != 0x30)) && DAT_1020b430[slot + 2].hideCount == 0 &&
                DAT_10c655f0 != 6)
            {
                DAT_10daaae8[slot + 2].held = 1;
                DAT_10daaae8[slot + 2].m_0141 = 0;
                DAT_10802bf8[slot + 2].position.z = 0;
            }
            else
            {
                DAT_10daaae8[slot + 2].held = 0;
                DAT_10daaae8[slot + 2].stats[STAT_VALUE_4] = 999;
                if (DAT_1020b430[index].keepCompanionUp == 0)
                    DAT_10802bf8[slot + 2].position.z = -50.0f;
                if (special != 0 && (area == 0x13 || area == 0x30))
                    DAT_1020b430[slot + 2].hideCount = 2;
                else
                    DAT_1020b430[slot + 2].hideCount--;
            }
            if (DAT_1042d9e8[index + 3] != 0)
            {
                unsigned char shown = DAT_104686b0[index + 3];
                DAT_1042d9e8[index + 3] = 0;
                DAT_10daaae8[slot + 2].stats[STAT_HEALTH] = health - 1;
                if (shown > 6)
                    DAT_104686b0[index + 3] = 6;
            }
            if (DAT_10daaae8[slot + 2].stats[STAT_HEALTH] < DAT_10daaae8[slot + 2].stats[STAT_MAX_HEALTH] - 5 ||
                (DAT_10c635f8 == 1 && DAT_10daaae8[slot + 2].stats[STAT_HEALTH] < DAT_10daaae8[slot + 2].stats[STAT_MAX_HEALTH]))
            {
                int revived = DAT_1020b430[slot + 2].reviveHealth;
                DAT_1020b430[slot + 2].downMode = 1;
                DAT_10daaae8[slot + 2].held = 0;
                if (revived != 0)
                    DAT_10daaae8[slot + 2].stats[STAT_HEALTH] = revived;
                else
                    DAT_10daaae8[slot + 2].stats[STAT_HEALTH] = DAT_10daaae8[index].stats[STAT_MAX_HEALTH] + 2000;
                DAT_10daaae8[slot + 2].stats[STAT_VALUE_4] = DAT_10daaae8[index].stats[STAT_VALUE_4];
                DAT_104686b0[index + 3] = 0x1e;
                DAT_10802bf8[slot + 2].position.z = 0;
            }
        }
        else if (health > 2000)
        {
            DAT_10daaae8[slot + 2].m_0141 = 0;
            if (DAT_104686b0[index + 3] == 7)
            {
                if (DAT_1020d4c0[index + 3] > 4)
                {
                    if (DAT_104c00f8 != 0)
                        DAT_10f08080[index].Copy(DAT_10836c74 - DAT_10db6ee8[index].matrix.GetTranslation());
                    DAT_1020d4c0[index + 3] = 6;
                }
                else
                {
                    if (DAT_104c00f8 != 0)
                        DAT_10f08080[index].Copy(DAT_10836c74 - DAT_10db6ee8[index].matrix.GetTranslation());
                    DAT_1020d4c0[index + 3] = 4;
                }
            }
        }
        else
        {
            DAT_10daaae8[slot + 2].stats[STAT_HEALTH] = 2000;
            DAT_10daaae8[slot + 2].m_0141 = 1;
        }
    }
    return 0;
}

// 0x100b0f00 (Companion2_Update, a C++ function: it has an EH frame for its matrices). Keeps companion 2 of character `index` (slot index + 4) in range and runs its
// down/revive cycle (state slot 4): 0 = active (checks health, waits outside the areas 0x13 and 0x30, goes down
// below its maximum), 1 = counting to 60 then revives with effect 0x36d0, sound 0x194 and 30 points, 2 = counts
// down to active, 10 and above counts down with effect 0x36d4 at 70 and comes back at 11. Returns 0.
int Companion2_Update(int index)
{
    int slot = index + 2;
    if (DAT_103153b0[slot + 2] > 15)
        DAT_103153b0[slot + 2] = 15;
    if (DAT_10219770[slot + 2] > 5)
        DAT_10219770[slot + 2] = 5;
    if (DAT_10315350[index + 4] > 0)
        DAT_10315350[index + 4] = 0;
    unsigned char level = DAT_102550e0[index + 4];
    if (level != 0)
    {
        if (DAT_102550e0[index] >= level)
            level = DAT_102550e0[index];
        DAT_102550e0[index] = level;
    }
    int mode = DAT_1020b430[slot + 2].downMode;
    if (mode != 0)
    {
        if (mode == 1)
        {
            DAT_104686b0[index + 4] = 2;
            if (DAT_1020b430[slot + 2].downTimer == 1)
                FUN_10065b10(index, 0x192, 1, 0);
            if (++DAT_1020b430[slot + 2].downTimer == 0x3c)
            {
                DAT_1020b430[slot + 2].downMode = 0;
                Vec4 position;
                Matrix4 matrix;
                if (DAT_1021bdf8[slot + 2][1] != 0)
                {
                    DAT_1021bdf8[slot + 2][1]->SetFlags(0x20);
                    matrix = DAT_1021bdf8[slot + 2][1]->worldMatrix;
                    FUN_1001d8b0(index, 0x36d0, position, -1, -1, &DAT_1021bdf8[slot + 2][1]->worldMatrix, 0, 0, &matrix, 0);
                }
                else
                {
                    matrix = RotationZ(DAT_10db6ee8[index].heading);
                    FUN_1001d8b0(index, 0x36d0, position, -1, -1, 0, 0, 0, &matrix, 0);
                }
                FUN_10065b10(index, 0x194, 1, 0);
                DAT_10c64ad8 += 0x1e;
                FUN_1000e750();
            }
        }
        else if (mode == 2)
        {
            if (--DAT_1020b430[slot + 2].downTimer == 0)
                DAT_1020b430[slot + 2].downMode = 0;
            DAT_104686b0[index + 4] = 2;
        }
        else if (mode >= 10)
        {
            DAT_1020b430[slot + 2].downMode = mode - 1;
            if (mode - 1 == 0x46)
            {
                Vec4 position;
                Matrix4 matrix;
                if (DAT_1021bdf8[slot + 2][1] != 0)
                {
                    matrix = DAT_1021bdf8[slot + 2][1]->worldMatrix;
                    FUN_1001d8b0(index, 0x36d3, position, -1, -1, &DAT_1021bdf8[slot + 2][1]->worldMatrix, 0, 0, &matrix, 0);
                }
                else
                {
                    matrix = RotationZ(DAT_10db6ee8[index].heading);
                    FUN_1001d8b0(index, 0x36d3, position, -1, -1, 0, 0, 0, &matrix, 0);
                }
            }
            if (DAT_1020b430[slot + 2].downMode == 0xb)
            {
                if (DAT_1021bdf8[slot + 2][1] != 0)
                {
                    DAT_1021bdf8[slot + 2][1]->ClearFlags(0x20);
                    int health = DAT_10daaae8[slot + 2].stats[STAT_HEALTH] + 200;
                    if (DAT_1020b430[index].companionHealthCap + 2000 <= health)
                        health = DAT_1020b430[index].companionHealthCap + 2000;
                    DAT_1020b430[slot + 2].reviveHealth = health;
                    DAT_10daaae8[slot + 2].stats[STAT_HEALTH] = DAT_10daaae8[slot + 2].stats[STAT_MAX_HEALTH];
                    DAT_10db6ee8[slot + 2].root->M_10194730(0x2f, 0x400);
                    DAT_10daaae8[slot + 2].m_0141 = 0;
                    DAT_1021f3c0[index + 4] = 3;
                }
                DAT_1020b430[slot + 2].downMode = 2;
                DAT_104686b0[index + 4] = 2;
            }
        }
    }
    else
    {
        int health = DAT_10daaae8[slot + 2].stats[STAT_HEALTH];
        if (health > 70000)
        {
            int special = DAT_104c00e0;
            int area = DAT_104c00e8;
            DAT_10daaae8[slot + 2].m_0141 = 0;
            if ((special == 0 || (area != 0x13 && area != 0x30)) && DAT_1020b430[slot + 2].hideCount == 0 &&
                DAT_10c655f0 != 6)
            {
                DAT_10daaae8[slot + 2].held = 1;
                DAT_10daaae8[slot + 2].m_0141 = 0;
            }
            else
            {
                DAT_10daaae8[slot + 2].held = 0;
                DAT_10daaae8[slot + 2].stats[STAT_VALUE_4] = 999;
                if (special != 0 && (area == 0x13 || area == 0x30))
                    DAT_1020b430[slot + 2].hideCount = 2;
                else
                    DAT_1020b430[slot + 2].hideCount--;
            }
            if (DAT_1042d9e8[index + 4] != 0)
            {
                unsigned char shown = DAT_104686b0[index + 4];
                DAT_1042d9e8[index + 4] = 0;
                DAT_10daaae8[slot + 2].stats[STAT_HEALTH] = health - 1;
                if (shown > 6)
                    DAT_104686b0[index + 4] = 6;
            }
            if (DAT_10daaae8[slot + 2].stats[STAT_HEALTH] < DAT_10daaae8[slot + 2].stats[STAT_MAX_HEALTH] - 10 ||
                (DAT_10c635f8 == 1 && DAT_10daaae8[slot + 2].stats[STAT_HEALTH] < DAT_10daaae8[slot + 2].stats[STAT_MAX_HEALTH]))
            {
                DAT_1020b430[slot + 2].downMode = 1;
                int revived = DAT_1020b430[slot + 2].reviveHealth;
                DAT_10daaae8[slot + 2].held = 0;
                if (revived != 0)
                    DAT_10daaae8[slot + 2].stats[STAT_HEALTH] = revived;
                else
                    DAT_10daaae8[slot + 2].stats[STAT_HEALTH] = DAT_10daaae8[index].stats[STAT_MAX_HEALTH] + 2000;
                DAT_10daaae8[slot + 2].stats[STAT_VALUE_4] = DAT_10daaae8[index].stats[STAT_VALUE_4];
                DAT_104686b0[index + 4] = 0x3c;
            }
        }
        else if (health > 2000)
        {
            DAT_10daaae8[slot + 2].m_0141 = 0;
            if (DAT_104686b0[index + 4] == 7)
            {
                if (DAT_1020d4c0[index + 4] > 4)
                {
                    if (DAT_104c00f8 != 0)
                        DAT_10f08080[index].Copy(DAT_10836c74 - DAT_10db6ee8[index].matrix.GetTranslation());
                    DAT_1020d4c0[index + 4] = 6;
                }
                else
                {
                    if (DAT_104c00f8 != 0)
                        DAT_10f08080[index].Copy(DAT_10836c74 - DAT_10db6ee8[index].matrix.GetTranslation());
                    DAT_1020d4c0[index + 4] = 4;
                }
            }
        }
        else
        {
            DAT_10daaae8[slot + 2].stats[STAT_HEALTH] = 2000;
            DAT_10daaae8[slot + 2].m_0141 = 1;
        }
    }
    return 0;
}

typedef char check_character[sizeof(AnimModel) == 0x249c ? 1 : -1];
