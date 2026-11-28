// The host callback for screen effects (slot 1 of DAT_1101de40): the effect scripts call it with a command number
// and up to four values.

#include "vec4.h"
#include "doppler_loops.h"
#include "partner_follow.h"
#include "route_flier.h"
#include "target_race.h"
#include "game_load_data.h"

// Effect-script commands (the first argument of the hook, a float).
#define SCREEN_COMMAND_SOUND 0.0f        // play the sound effect numbered `value`
#define SCREEN_COMMAND_TITLE 1.0f        // show the caption of the chapter title
#define SCREEN_COMMAND_COLOUR_FADE 2.0f  // fade the screen colour to (a, b, c)
#define SCREEN_COMMAND_KNOCK_BACK 4.0f   // knock the player back

// Caption id shown by the title command.
#define TITLE_CAPTION "4537"
// Number of frames of the colour fade.
#define COLOUR_FADE_FRAMES 0xf

// 0x1005c250. Always returns 0.
extern "C" int FUN_1005c250(float command, float a, float b, float c, const Vec4 *position)
{
    if (command == SCREEN_COMMAND_SOUND)
    {
        FUN_1003ad20((int)c, position, 1.0f);
        return 0;
    }
    if (command == SCREEN_COMMAND_TITLE)
    {
        FUN_1016b230(TITLE_CAPTION);
        return 0;
    }
    if (command == SCREEN_COMMAND_COLOUR_FADE)
    {
        FUN_1000ec70(COLOUR_FADE_FRAMES, a, b, c, 1.0f);
        return 0;
    }
    if (command == SCREEN_COMMAND_KNOCK_BACK)
        FUN_10055ee0();
    return 0;
}
