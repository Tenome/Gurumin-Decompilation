// Writing a save game (gdataNN.sav, see docs/SAVE_FORMAT.md): header 9999, a random key, then the scrambled body
// (the 0x30000-byte thumbnail and the game state) and its size, sum and check. The items ever obtained are also
// recorded as bit sets in system.sav. C++ rather than extern "C": the function keeps an exception frame for its
// GameFile local.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game_file.h"

#include "texture_object.h"
#include "char_slot.h"

#include "char_vars.h"
#include "game_vars.h"

extern "C" int FUN_101603c0(const char *name, int record, unsigned int *value);   // SystemSav_ReadRecord
extern "C" int FUN_1015fea0(const char *name, int record, unsigned int value);    // SystemSav_WriteRecord
extern "C" int FUN_10161120(char *out, const char *path);                         // Path_GetBaseName
extern "C" int FUN_101614c0(unsigned char *data, int size, char *name, int *sum, unsigned int *check);   // ItmScramble

// 0x100106e0 (Save_Write). Adds the items obtained so far to the bit sets in system.sav records 0x81-0x83, then
// writes save file `path`: header 9999 and a random key, followed by the body scrambled with the file's base name
// (every second of its first four characters shifted by the key): the thumbnail, the map, the start position,
// DAT_104bfd18, a 0x158-byte block (filled from DAT_1049f200), the player's slot record, the script flags,
// DAT_103150c0, the play time, a zero, DAT_10202464 and DAT_1049f200; then the body size, sum and check.
// Returns 1, or 0 when the file cannot be created.
int FUN_100106e0(const char *path)
{
    unsigned int obtained[4];
    FUN_101603c0(DAT_10256110, 0x81, &obtained[0]);
    FUN_101603c0(DAT_10256110, 0x82, &obtained[1]);
    FUN_101603c0(DAT_10256110, 0x83, &obtained[2]);
    for (int i = 0; i < 0x60; i++)
    {
        if (DAT_1042de28[i] != 0)
            obtained[i / 32] |= 1 << (i % 32);
    }
    FUN_1015fea0(DAT_10256110, 0x81, obtained[0]);
    FUN_1015fea0(DAT_10256110, 0x82, obtained[1]);
    FUN_1015fea0(DAT_10256110, 0x83, obtained[2]);
    GameFile file;
    char name[0x100];
    sprintf(name, "%s", path);
    file.Open(name, "wb");
    if (file.handle != 0)
    {
        unsigned char *body = (unsigned char *)operator new(0x64000);
        int size = 0;
        int time = DAT_104c00dc;
        int key = 9999;
        file.Write(&key, 1, 4);
        key = rand();
        file.Write(&key, 1, 4);
        unsigned int check;
        unsigned char thumbnail[0x30000];
        DAT_118d0ba8[DAT_108360f8].CaptureRgb24(thumbnail);
        memcpy(body, thumbnail, 0x30000);
        int map = DAT_10c62ed0;
        // the start position is copied as raw bits
        int x = *(int *)&DAT_108029b8.x;
        int y = *(int *)&DAT_108029b8.y;
        int z = *(int *)&DAT_108029b8.z;
        int w = *(int *)&DAT_108029b8.w;
        // the state behind the thumbnail, written with a cursor (`size` follows it)
        size += 0x30000;
        unsigned char *p = body + 0x30000;
        *(int *)p = map;
        p += 4;
        size += 4;
        *(int *)p = x;
        p += 4;
        size += 4;
        *(int *)p = y;
        p += 4;
        size += 4;
        *(int *)p = z;
        p += 4;
        size += 4;
        *(int *)p = w;
        p += 4;
        size += 4;
        *(int *)p = DAT_104bfd18;
        p += 4;
        size += 4;
        memcpy(p, DAT_1049f200, 0x140);
        p += 0x140;
        size += 0x140;
        memcpy(p, DAT_1049f200, 0x14);
        p += 0x14;
        size += 0x14;
        memcpy(p, &DAT_10daaae8[0], sizeof(CharSlot));
        p += 0x188;
        size += 0x188;
        memcpy(p, DAT_10c62edc, 0x4000);
        p += 0x4000;
        size += 0x4000;
        memcpy(p, DAT_103150c0, 0x80);
        p += 0x80;
        size += 0x80;
        *(int *)p = time;
        p += 4;
        size += 4;
        *(int *)p = 0;
        p += 4;
        size += 4;
        memcpy(p, DAT_10202464, 0x10);
        p += 0x10;
        size += 0x10;
        memcpy(p, DAT_1049f200, 0x18000);
        size += 0x18000;
        char baseName[0x100];
        FUN_10161120(baseName, path);
        baseName[0] += (char)key;
        baseName[2] += (char)key;
        baseName[4] += (char)key;
        baseName[6] += (char)key;
        int sum;
        FUN_101614c0(body, size, baseName, &sum, &check);
        file.Write(body, 1, size);
        file.Write(&size, 1, 4);
        file.Write(&sum, 1, 4);
        file.Write(&check, 1, 4);
        file.Close();
        free(body);
        return 1;
    }
    return 0;
}
