// Drawing the background movie of the map (and the map icon behind it) at the lower right of the Map screen.

#include <stdlib.h>

#include "field_hud.h"
#include "mmv_player.h"
#include "texture_object.h"
#include "game_vars.h"
#include "map_vars.h"
#include "ui_vars.h"

extern "C" void FUN_1013f070(int mode, int a, int b);  // sets the 2D blend mode

#define MOVIE_FADE_IN_FRAMES 15
#define MOVIE_FADE_OUT_START 0x50
#define MOVIE_FADE_OUT_CENTRE 0x62
#define MOVIE_SHADOW_GREY 0.65f
#define MOVIE_SCREEN_WIDTH 0x280
#define MOVIE_SCREEN_HEIGHT 0x1e0

// 0x1001ed80 (Map_DrawMovie). Draws the decoded movie texture, shrunk to a half and placed by DAT_10c6359c, with a
// dark four-way shadow behind it, and the map icon (128x128) around its centre. The alpha follows the frame counter
// (fade in over 15 frames, out after 80) and the map screen's own fade. Nothing is drawn while the movie is hidden
// or not yet decoded. Returns 0.
extern "C" int FUN_1001ed80(int mapNumber)
{
    int x;
    int y;
    float screenFade = (float)abs(DAT_102028dc - 0x14) / 20.0f;
    float movieFade;
    if (DAT_104bfe70 < MOVIE_FADE_IN_FRAMES)
        movieFade = (float)DAT_104bfe70 / 15.0f;
    else if (DAT_104bfe70 > MOVIE_FADE_OUT_START)
        movieFade = (float)abs(DAT_104bfe70 - MOVIE_FADE_OUT_CENTRE) / 15.0f;
    else
        movieFade = 1.0f;
    float iconAlpha = movieFade * screenFade;
    FUN_1013f070(7, 1, 0);
    if (DAT_104bfe70 != 0 && DAT_10c66d08 == 0 && DAT_10f08ab0.m_004b != 0)
    {
        if (DAT_10c6359c == 0)
        {
            x = 0x140 - DAT_10f08ab0.width / 4;
            y = 0xf0 - DAT_10f08ab0.height / 4;
        }
        else if (DAT_10c6359c == 1)
        {
            x = 0x140 - DAT_10f08ab0.width / 4;
            y = 0x8c - DAT_10f08ab0.height / 4;
        }
        else if (DAT_10c6359c == 2)
        {
            y = DAT_10c635a4;
            x = DAT_10c635a0;
        }
        else if (DAT_10c6359c == 3)
        {
            x = DAT_10c635a0 - DAT_10f08ab0.width / 4;
            y = DAT_10c635a4 - DAT_10f08ab0.height / 4;
        }
        int movieWidth = DAT_10f08ab0.width;
        int movieHeight = DAT_10f08ab0.height;
        int iconCentreY = movieHeight * 3 / 8 + y;
        int cropLeft = 0;
        int cropTop = 0;
        int iconCentreX = movieWidth / 4 + x;
        int drawWidth = movieWidth / 2;
        int drawHeight = movieHeight / 2;
        if (x < 0)
        {
            cropLeft = -x;
            x = 0;
        }
        if (y < 0)
        {
            cropTop = -y;
            y = 0;
        }
        if (drawWidth + x > MOVIE_SCREEN_WIDTH - 1)
            drawWidth = MOVIE_SCREEN_WIDTH - x;
        if (drawHeight + y > MOVIE_SCREEN_HEIGHT - 1)
            drawHeight = MOVIE_SCREEN_HEIGHT - y;
        if (DAT_1020b228 > -1)
            DAT_118d0ba8[DAT_1020b228].DrawTinted(iconCentreX - 0x40, iconCentreY - 0x40, iconCentreX + 0x40,
                                                  iconCentreY + 0x40, 1.0f / 256.0f, 1.0f / 256.0f, 255.0f / 256.0f,
                                                  255.0f / 256.0f, 0.0f, iconAlpha, 0.0f, 0.0f);
        FUN_1013f070(0, 10, 0);
        float shadow = screenFade * MOVIE_SHADOW_GREY;
        float v1 = (float)(movieHeight - 2) / 512.0f;
        int bottom = drawHeight - cropTop + y;
        int right = drawWidth - cropLeft + x;
        float u1 = (float)(movieWidth - 2) / 1024.0f;
        float v0 = (float)(cropTop + 2) / 512.0f;
        float u0 = (float)(cropLeft + 2) / 1024.0f;
        DAT_118d0ba8[DAT_10202478].DrawColoured(x - 4, y + 0xc, right - 4, bottom + 0xc, u0, v0, u1, v1, 0.0f, screenFade, 0.0f, shadow,
                           shadow, shadow);
        DAT_118d0ba8[DAT_10202478].DrawColoured(x + 4, y + 0xc, right + 4, bottom + 0xc, u0, v0, u1, v1, 0.0f, screenFade, 0.0f, shadow,
                           shadow, shadow);
        DAT_118d0ba8[DAT_10202478].DrawColoured(x - 4, y + 0x14, right - 4, bottom + 0x14, u0, v0, u1, v1, 0.0f, screenFade, 0.0f,
                           shadow, shadow, shadow);
        DAT_118d0ba8[DAT_10202478].DrawColoured(x + 4, y + 0x14, right + 4, bottom + 0x14, u0, v0, u1, v1, 0.0f, screenFade, 0.0f,
                           shadow, shadow, shadow);
        FUN_1013f070(0, 1, 0);
        DAT_118d0ba8[DAT_10202478].DrawTinted(x, y + 0x10, right, bottom + 0x10, u0, v0, u1, v1, 0.0f, screenFade, 0.0f, 0.0f);
        DAT_118d0ba8[DAT_10202478].DrawTinted(x, y + 0x10, right, bottom + 0x10, u0, v0, u1, v1, 0.0f, screenFade, 0.0f, 0.0f);
        FUN_1013f070(0, 0, 0);
    }
    return 0;
}
