// Sound effects from the "wav" folder, positional or not.

#include <stdio.h>

#include "vec3.h"

#include "char_vars.h"
#include "game_vars.h"
#include "sound_vars.h"
#include "menu_sound.h"

extern "C" char *FUN_1013f910(const char *name);  // full path of a data file
// 0x10010310: plays `path` at (x, y, z) through the 3D sound code (`loop` = 0 plays once); returns its handle.
extern "C" int FUN_10010310(const char *path, float x, float y, float z, int loop, float volume);
// 0x10010420 (app_0013.cpp): plays `path` without position, looping when `loop` is set; returns its handle.
extern "C" int FUN_10010420(const char *path, int unused1, int unused2, int loop, float volume);
// 0x100016c0 (soundarc.cpp): plays a sound file once; the volume goes over as the bits of a float here.
extern "C" int FUN_100016c0(const char *file, const int *x, const int *y, const int *z, int pan, int channel,
                            float volume);

#include "sound_def.h"


// 0x1003ac20 (Sound_PlayWav). Plays "wav\<name>.<extension>" at `position` (0 = not positional) unless sound
// effects are off. The volume is scaled first: by 0.8 for positional sounds when DAT_104bfe6c is set, otherwise by
// 0.95 (positional) or 0.65 (not positional). Returns -1.
extern "C" int FUN_1003ac20(const char *name, const Vec3 *position, float volume)
{
    char path[256];
    if (DAT_104bfe6c != 0)
    {
        if (position != 0)
            volume = volume * 0.8;
    }
    else
    {
        if (position == 0)
            volume = volume * 0.65;
        else
            volume = volume * 0.95;
    }
    if (DAT_104c01b0 == 0)
    {
        sprintf(path, "wav\\%s.%s", name, DAT_108db428);
        if (position != 0)
            FUN_10010310(path, position->x, position->y, position->z, 0, volume);
        else
            FUN_100016c0(FUN_1013f910(path), 0, 0, 0, 0, 0, volume);
    }
    return -1;
}

// 0x1003ad20 (Sound_PlayEffect). Plays sound effect `sound` from the effect table at `position` (0 = not
// positional), unless effects are off. The volume is turned down like in Sound_PlayWav (0.8 in the DAT_104bfe6c
// mode, else 0.65 when not positional) and, for positional sounds, multiplied by the effect's own volume percent.
// A looping effect is only started when it is not already playing; its handle is kept in DAT_10447338 and
// returned. Returns -1 otherwise.
extern "C" int FUN_1003ad20(int sound, const Vec3 *position, float volume)
{
    char path[256];
    if (DAT_108dc158[sound].notPositional != 0)
        position = 0;
    if (DAT_104bfe6c != 0)
        volume = volume * 0.8;
    else if (position == 0)
        volume = volume * 0.65;
    if (DAT_104c01b0 == 0)
    {
        sprintf(path, "wav\\%s.%s", DAT_108dc158[sound].name, DAT_108db428);
        if (position != 0)
        {
            int loop = DAT_108dc158[sound].loop;
            float percent = (float)DAT_108dc158[sound].volume;
            if (loop != 0)
            {
                if (DAT_108dc158[sound].volume == 0)
                    percent = 100.0f;
                if (DAT_10447338[sound] == -11)
                {
                    int handle = FUN_10010310(path, position->x, position->y, position->z, loop,
                                              percent * volume / 100.0f);
                    DAT_10447338[sound] = handle;
                    return handle;
                }
            }
            else
            {
                if (DAT_108dc158[sound].volume == 0)
                    percent = 100.0f;
                FUN_10010310(path, position->x, position->y, position->z, 0, percent * volume / 100.0f);
            }
        }
        else
        {
            if (DAT_108dc158[sound].loop != 0)
            {
                if (DAT_10447338[sound] == -11)
                {
                    int handle = FUN_10010420(path, 0, 0, DAT_108dc158[sound].loop, volume);
                    DAT_10447338[sound] = handle;
                    return handle;
                }
            }
            else
            {
                FUN_100016c0(FUN_1013f910(path), 0, 0, 0, 0, 0, volume);
            }
        }
    }
    return -1;
}

// Sound effect of the menu cursor moving from one entry to the next.
#define SOUND_MENU_CURSOR 0x38d

// 0x1003b160. The sound callback of the menu windows (Game::m_7660): plays the cursor sound when the cursor moved to
// another entry. Opening and closing a window make no sound here. Always returns 0.
extern "C" int FUN_1003b160(int event)
{
    if (event == MENU_SOUND_CURSOR_MOVED)
        FUN_1003ad20(SOUND_MENU_CURSOR, 0, 1.0f);
    return 0;
}
