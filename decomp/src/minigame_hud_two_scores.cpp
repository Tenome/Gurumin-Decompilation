// Map 0x15 mini game HUD: the count-down / start / finish banner cut from the "minigm" sheet, and two score counters.

#include <stdlib.h>
#include "font_renderer.h"
#include "map_vars.h"
#include "minigame_hud.h"
#include "render_state.h"
#include "texture_object.h"

// Banner timeline: frames 1..89 show the count-down digits (one per second), frames 90..119 the start word; once
// the map timer is negative the finish word fades in over BANNER_FRAMES_PER_STEP frames.
#define BANNER_DIGIT_FRAMES 0x5a
#define BANNER_END_FRAMES 0x78
#define BANNER_FRAMES_PER_STEP 30
#define BANNER_FRAMES_PER_STEP_F 30.0f

// Centre of the 640x480 layout the banner shrinks towards.
#define BANNER_CENTRE_X 0x140
#define BANNER_CENTRE_Y 0x78

// Digit cells of the sheet: 32 texels wide on a 128 texel sheet, inset by half a texel.
#define SHEET_WIDTH 128.0f
#define DIGIT_CELL_TEXELS 32.0f
#define DIGIT_CELL_FIRST_TEXEL 0.5f
#define DIGIT_CELL_LAST_TEXEL 31.5f

// Rows (in 256ths of the sheet height) of the digits, the start word and the finish word.
#define DIGIT_V0 (129.0f / 256.0f)
#define DIGIT_V1 (191.0f / 256.0f)
#define START_V0 (1.0f / 256.0f)
#define START_V1 (63.0f / 256.0f)
#define FINISH_V0 (65.0f / 256.0f)
#define FINISH_V1 (127.0f / 256.0f)
#define WORD_U0 (1.0f / 256.0f)
#define WORD_U1 (255.0f / 256.0f)

// The banner starts BANNER_GROWTH times larger than its target: size = (base - fade * rate) * BANNER_GROWTH. The glow
// draws it a second time, additive, at GLOW_ALPHA of the alpha with its own rates.
#define GLOW_ALPHA 0.75f
#define BANNER_GROWTH 1.1f
#define DIGIT_BASE 16
#define WORD_BASE 64
#define DIGIT_RATE -12.0f
#define DIGIT_GLOW_RATE -24.0f
#define WORD_WIDTH_RATE -24.0f
#define WORD_HEIGHT_RATE -6.0f
#define WORD_GLOW_WIDTH_RATE -48.0f
#define WORD_GLOW_HEIGHT_RATE -12.0f

// Score counters: the player's score right-aligned beside its label, popping with a growing glow, and the second
// (pink) score at the right edge popping the same way around its centre.
#define SCORE_LABEL_X 0x21f
#define SCORE_LABEL_Y 0x176
#define SCORE_NUMBER_RIGHT 0x216
#define SCORE_NUMBER_Y 0x174
#define SECOND_SCORE_X 0x230
#define SCORE_SIZING_SCALE 1.9f
#define SCORE_BASE_SCALE 1.8f
#define SCORE_POP_SCALE_DIVISOR 37.5f
#define SCORE_POP_RISE_DIVISOR 15.0f
#define SCORE_POP_RISE 4.0f
#define SCORE_POP_GLOW 0.35f
#define SCORE_RADIX 10
#define COLOUR_WHITE 0xffffff
#define COLOUR_PINK 0xff8888

// The one-character label drawn after the score (cp932 0x967b).
#define SCORE_LABEL_TEXT "{"

#define SCALED(base, fade, rate) ((int)((float)((base) - (int)((fade) * (rate))) * BANNER_GROWTH))

// Draws one banner quad of half size (halfW, halfH) around the layout centre.
#define BANNER_QUAD(slot, halfW, halfH, u0, v0, u1, v1, alpha)                                                          \
    DAT_118d0ba8[slot].DrawTinted(BANNER_CENTRE_X - (halfW), BANNER_CENTRE_Y - (halfH),                  \
                                          (halfW) + BANNER_CENTRE_X, (halfH) + BANNER_CENTRE_Y, u0, v0, u1, v1, \
                                          0.0f, alpha, 0.0f, 0.0f)

// Draws the additive copy of the banner at GLOW_ALPHA of `alpha`, restoring the blend mode afterwards.
#define BANNER_GLOW(halfW, halfH, u0, v0, u1, v1, alpha)                                                              {                                                                                                                     int previousMode = DAT_10202b14;                                                                                  FUN_1013f070(RENDER_BLEND_MODE, BLEND_ADD, 0);                                                                    BANNER_QUAD(DAT_102028cc, halfW, halfH, u0, v0, u1, v1, (alpha) * GLOW_ALPHA);                                    FUN_1013f070(RENDER_BLEND_MODE, previousMode, 0);                                                             }

// 0x100308e0 (MapMinigame_DrawHudTwoScores). Map 0x15 variant of MapMinigame_DrawHud: the same start / finish banner
// (count-down digits, start word, finish word, each with an additive glow copy), then the player's score beside its
// label and a second pink score at the right edge, each popping larger with a glow while its pop counter runs.
// Draws no banner when the map holds no textures. Returns 0.
extern "C" int FUN_100308e0(void)
{
    char scoreText[12];
    float fade;
    if (DAT_102028c8 < 0)
        return 0;
    int timer = DAT_10c6505c;
    int slot = DAT_102028cc;
    if (slot != -1 || (slot = DAT_102028cc = FUN_10161920("minigm"), slot != -1))
    {
        if (DAT_1020c2e8 < BANNER_DIGIT_FRAMES && DAT_1020c2e8 > 0)
        {
            int step = DAT_1020c2e8 / BANNER_FRAMES_PER_STEP;
            fade = (float)(BANNER_FRAMES_PER_STEP - DAT_1020c2e8 % BANNER_FRAMES_PER_STEP) / BANNER_FRAMES_PER_STEP_F;
            int halfW = SCALED(DIGIT_BASE, fade, DIGIT_RATE);
            int glowHalf = SCALED(DIGIT_BASE, fade, DIGIT_GLOW_RATE);
            float u1 = ((float)step * DIGIT_CELL_TEXELS + DIGIT_CELL_LAST_TEXEL) / SHEET_WIDTH;
            float u0 = ((float)step * DIGIT_CELL_TEXELS + DIGIT_CELL_FIRST_TEXEL) / SHEET_WIDTH;
            BANNER_QUAD(slot, halfW, halfW, u0, DIGIT_V0, u1, DIGIT_V1, fade);
            BANNER_GLOW(glowHalf, glowHalf, u0, DIGIT_V0, u1, DIGIT_V1, fade);
        }
        else if (DAT_1020c2e8 < BANNER_END_FRAMES && DAT_1020c2e8 > BANNER_DIGIT_FRAMES - 1)
        {
            fade = (float)(BANNER_FRAMES_PER_STEP - DAT_1020c2e8 % BANNER_FRAMES_PER_STEP) / BANNER_FRAMES_PER_STEP_F;
            int halfW = SCALED(WORD_BASE, fade, WORD_WIDTH_RATE);
            int halfH = SCALED(DIGIT_BASE, fade, WORD_HEIGHT_RATE);
            int glowHalfW = SCALED(WORD_BASE, fade, WORD_GLOW_WIDTH_RATE);
            int glowHalfH = SCALED(DIGIT_BASE, fade, WORD_GLOW_HEIGHT_RATE);
            BANNER_QUAD(slot, halfW, halfH, WORD_U0, START_V0, WORD_U1, START_V1, fade);
            BANNER_GLOW(glowHalfW, glowHalfH, WORD_U0, START_V0, WORD_U1, START_V1, fade);
        }
        else
        {
            if (timer >= 0)
                goto drawScore;
            if (timer > -BANNER_FRAMES_PER_STEP)
                fade = (float)(timer + BANNER_FRAMES_PER_STEP) / BANNER_FRAMES_PER_STEP_F;
            else
                fade = 0.0f;
            int halfW = SCALED(WORD_BASE, fade, WORD_WIDTH_RATE);
            int halfH = SCALED(DIGIT_BASE, fade, WORD_HEIGHT_RATE);
            int glowHalfW = SCALED(WORD_BASE, fade, WORD_GLOW_WIDTH_RATE);
            int glowHalfH = SCALED(DIGIT_BASE, fade, WORD_GLOW_HEIGHT_RATE);
            BANNER_QUAD(slot, halfW, halfH, WORD_U0, FINISH_V0, WORD_U1, FINISH_V1, 1.0f);
            BANNER_GLOW(glowHalfW, glowHalfH, WORD_U0, FINISH_V0, WORD_U1, FINISH_V1, fade);
        }
    }
drawScore:
    _itoa(DAT_10c65060, scoreText, SCORE_RADIX);
    int width = DAT_1020e538.TextWidth(scoreText, SCORE_SIZING_SCALE);
    DAT_1020e538.DrawOutlined(SCORE_LABEL_X, SCORE_LABEL_Y, SCORE_LABEL_TEXT, COLOUR_WHITE, 0, 1.0f, SCORE_BASE_SCALE);
    int halfWidth = (int)(((float)width + 0.5f) / 2.0f);
    if (DAT_1020c2ec != 0)
    {
        int grownHalf = (int)(((float)DAT_1020e538.TextWidth(scoreText, (float)DAT_1020c2ec / SCORE_POP_SCALE_DIVISOR +
                                                                            SCORE_BASE_SCALE) +
                               0.5f) /
                              2.0f);
        int x = SCORE_NUMBER_RIGHT - halfWidth - grownHalf;
        float pop = (float)DAT_1020c2ec;
        float rise = pop / SCORE_POP_RISE_DIVISOR;
        int y = SCORE_NUMBER_Y - (int)(rise * SCORE_POP_RISE + 0.5f);
        DAT_1020e538.DrawOutlined(x, y, scoreText, COLOUR_WHITE, 0, 1.0f, pop / SCORE_POP_SCALE_DIVISOR + SCORE_BASE_SCALE);
        int previousMode = DAT_10202b14;
        FUN_1013f070(RENDER_BLEND_MODE, BLEND_ADD, 0);
        DAT_1020e538.DrawOutlined(x, y, scoreText, COLOUR_WHITE, 0, rise * SCORE_POP_GLOW,
                                  (float)DAT_1020c2ec / SCORE_POP_SCALE_DIVISOR + SCORE_BASE_SCALE);
        FUN_1013f070(RENDER_BLEND_MODE, previousMode, 0);
    }
    else
    {
        DAT_1020e538.DrawOutlined(SCORE_NUMBER_RIGHT - width, SCORE_NUMBER_Y, scoreText, COLOUR_WHITE, 0, 1.0f,
                                  SCORE_BASE_SCALE);
    }
    _itoa(DAT_1020c324, scoreText, SCORE_RADIX);
    if (DAT_1020c2e4 != 0)
    {
        float grownWidth = (float)DAT_1020e538.TextWidth(scoreText, (float)DAT_1020c2e4 / SCORE_POP_SCALE_DIVISOR +
                                                                        SCORE_BASE_SCALE);
        int growth = (int)((grownWidth - (float)DAT_1020e538.TextWidth(scoreText, SCORE_BASE_SCALE)) / 2.0f + 0.5f);
        float pop = (float)DAT_1020c2e4;
        float rise = pop / SCORE_POP_RISE_DIVISOR;
        int y = SCORE_NUMBER_Y - (int)(rise * SCORE_POP_RISE + 0.5f);
        int x = SECOND_SCORE_X - growth;
        DAT_1020e538.DrawOutlined(x, y, scoreText, COLOUR_PINK, 0, 1.0f, pop / SCORE_POP_SCALE_DIVISOR + SCORE_BASE_SCALE);
        int previousMode = DAT_10202b14;
        FUN_1013f070(RENDER_BLEND_MODE, BLEND_ADD, 0);
        DAT_1020e538.DrawOutlined(x, y, scoreText, COLOUR_WHITE, 0, rise * SCORE_POP_GLOW,
                                  (float)DAT_1020c2e4 / SCORE_POP_SCALE_DIVISOR + SCORE_BASE_SCALE);
        FUN_1013f070(RENDER_BLEND_MODE, previousMode, 0);
    }
    else
    {
        DAT_1020e538.DrawOutlined(SECOND_SCORE_X, SCORE_NUMBER_Y, scoreText, COLOUR_PINK, 0, 1.0f, SCORE_BASE_SCALE);
    }
    return 0;
}
