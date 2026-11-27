// Map 0x17 mini game HUD: the seconds timer (two digits, a dot, two hundredths), the count-down / start / finish
// banner cut from the "minigm" sheet, and the score counter.

#include <stdlib.h>
#include "font_renderer.h"
#include "map_vars.h"
#include "minigame_banner.h"
#include "minigame_hud.h"
#include "minigame_timer.h"
#include "render_state.h"
#include "texture_object.h"

#define TIMER_START_WORD_FRAMES 0x6ea

// Score counter: the label at the right edge, the number right-aligned beside it, popping with a growing glow.
#define SCORE_LABEL_X 0x22e
#define SCORE_LABEL_Y 0x180
#define SCORE_NUMBER_RIGHT 0x22a
#define SCORE_NUMBER_Y 0x174
#define SCORE_SIZING_SCALE 1.9f
#define SCORE_BASE_SCALE 1.8f
#define SCORE_POP_SCALE_DIVISOR 37.5f
#define SCORE_POP_RISE_DIVISOR 15.0f
#define SCORE_POP_RISE 4.0f
#define SCORE_POP_GLOW 0.35f
#define SCORE_RADIX 10
#define COLOUR_WHITE 0xffffff

// The two-character label drawn after the score (cp932 0x9240 0x82ab).
#define SCORE_LABEL_TEXT "\x92\x40\x82\xab"

// Half of a text width, rounded to the nearest pixel.
static inline int HalfWidth(int width)
{
    return (int)(((float)width + 0.5f) / 2.0f);
}

// 0x1002f520 (MapMinigame_DrawTimerHud). Draws the remaining time as SS.hh (digits cut from the map's count-down
// texture), then the start banner (count-down digits, the start word), the finish word once the timer has run out
// or the start word again from frame TIMER_START_WORD_FRAMES on, each as a shrinking quad plus an additive copy at
// GLOW_ALPHA of the alpha; then the score with a growing glow while it pops. Draws nothing when the map holds no
// textures. Returns 0.
extern "C" int FUN_1002f520(void)
{
    char scoreText[12];
    if (DAT_102028c8 < 0)
        return 0;
    int timer = DAT_10c6505c;
    float frames = (float)(timer % TIMER_FRAMES_PER_SECOND);
    int seconds = timer / TIMER_FRAMES_PER_SECOND;
    int secondsTens = seconds / 10;
    int secondsOnes = seconds % 10;
    int hundredths = (int)(frames * TIMER_HUNDREDTHS_PER_FRAME);
    int hundredthsTens = hundredths / 10;
    int hundredthsOnes = hundredths % 10;
    if (timer < 0)
    {
        secondsTens = 0;
        secondsOnes = 0;
        hundredthsTens = 0;
        hundredthsOnes = 0;
    }
    DRAW_TIMER_DIGIT(TIMER_TENS_LEFT, TIMER_DIGIT_TOP, TIMER_ONES_LEFT, TIMER_DIGIT_BOTTOM, secondsTens);
    DRAW_TIMER_DIGIT(TIMER_ONES_LEFT, TIMER_DIGIT_TOP, TIMER_DOT_LEFT, TIMER_DIGIT_BOTTOM, secondsOnes);
    DAT_118d0ba8[DAT_102028c8].DrawColoured(TIMER_DOT_LEFT, TIMER_SMALL_DIGIT_TOP, TIMER_HUNDREDTHS_LEFT,
                                            TIMER_DIGIT_BOTTOM, TIMER_DOT_U0, TIMER_DOT_V0, TIMER_DOT_U1,
                                            TIMER_DOT_V1, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    DRAW_TIMER_DIGIT(TIMER_HUNDREDTHS_LEFT, TIMER_SMALL_DIGIT_TOP, TIMER_HUNDREDTHS_ONES_LEFT, TIMER_DIGIT_BOTTOM,
                   hundredthsTens);
    DRAW_TIMER_DIGIT(TIMER_HUNDREDTHS_ONES_LEFT, TIMER_SMALL_DIGIT_TOP, TIMER_HUNDREDTHS_END, TIMER_DIGIT_BOTTOM,
                   hundredthsOnes);
    int slot = DAT_102028cc;
    if (slot != -1 || (slot = FUN_10161920("minigm"), DAT_102028cc = slot, slot != -1))
    {
        if (DAT_1020c2e8 < BANNER_DIGIT_FRAMES && DAT_1020c2e8 > 0)
        {
            int step = DAT_1020c2e8 / BANNER_FRAMES_PER_STEP;
            float fade = (float)(BANNER_FRAMES_PER_STEP - DAT_1020c2e8 % BANNER_FRAMES_PER_STEP) / BANNER_FRAMES_PER_STEP_F;
            int halfW = SCALED(DIGIT_BASE, fade, DIGIT_RATE);
            int glowHalf = SCALED(DIGIT_BASE, fade, DIGIT_GLOW_RATE);
            float u1 = ((float)step * DIGIT_CELL_TEXELS + DIGIT_CELL_LAST_TEXEL) / SHEET_WIDTH;
            float u0 = ((float)step * DIGIT_CELL_TEXELS + DIGIT_CELL_FIRST_TEXEL) / SHEET_WIDTH;
            BANNER_QUAD(slot, halfW, halfW, u0, DIGIT_V0, u1, DIGIT_V1, fade);
            BANNER_GLOW(glowHalf, glowHalf, u0, DIGIT_V0, u1, DIGIT_V1, fade);
        }
        else if (secondsTens == 0 && secondsOnes < TIMER_COUNT_DOWN_SECONDS)
        {
            if (timer > 0)
            {
                float fade = frames / BANNER_FRAMES_PER_STEP_F;
                int halfW = SCALED(DIGIT_BASE, fade, DIGIT_RATE);
                int glowHalf = SCALED(DIGIT_BASE, fade, DIGIT_GLOW_RATE);
                float u1 = ((float)(2 - secondsOnes) * DIGIT_CELL_TEXELS + DIGIT_CELL_LAST_TEXEL) / SHEET_WIDTH;
                float u0 = ((float)(2 - secondsOnes) * DIGIT_CELL_TEXELS + DIGIT_CELL_FIRST_TEXEL) / SHEET_WIDTH;
                BANNER_QUAD(slot, halfW, halfW, u0, DIGIT_V0, u1, DIGIT_V1, fade);
                BANNER_GLOW(glowHalf, glowHalf, u0, DIGIT_V0, u1, DIGIT_V1, fade);
            }
            else
            {
                float fade;
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
        else if (timer > TIMER_START_WORD_FRAMES)
        {
            float fade = frames / BANNER_FRAMES_PER_STEP_F;
            int halfW = SCALED(WORD_BASE, fade, WORD_WIDTH_RATE);
            int halfH = SCALED(DIGIT_BASE, fade, WORD_HEIGHT_RATE);
            int glowHalfW = SCALED(WORD_BASE, fade, WORD_GLOW_WIDTH_RATE);
            int glowHalfH = SCALED(DIGIT_BASE, fade, WORD_GLOW_HEIGHT_RATE);
            BANNER_QUAD(slot, halfW, halfH, WORD_U0, START_V0, WORD_U1, START_V1, fade);
            BANNER_GLOW(glowHalfW, glowHalfH, WORD_U0, START_V0, WORD_U1, START_V1, fade);
        }
    }
    _itoa(DAT_10c65060, scoreText, SCORE_RADIX);
    int width = DAT_1020e538.TextWidth(scoreText, SCORE_SIZING_SCALE);
    DAT_1020e538.DrawOutlined(SCORE_LABEL_X, SCORE_LABEL_Y, SCORE_LABEL_TEXT, COLOUR_WHITE, 0, 1.0f, 1.0f);
    if (DAT_1020c2ec != 0)
    {
        // Both half widths are used by the pop draw and by its glow copy, which keeps the first one ahead of the grown
        // TextWidth call; `width` is reused for the half width of the grown text.
        int halfWidth = HalfWidth(width);
        width =
            HalfWidth(DAT_1020e538.TextWidth(scoreText, (float)DAT_1020c2ec / SCORE_POP_SCALE_DIVISOR + SCORE_BASE_SCALE));
        float pop = (float)DAT_1020c2ec;
        float rise = pop / SCORE_POP_RISE_DIVISOR;
        int y = SCORE_NUMBER_Y - (int)(rise * SCORE_POP_RISE + 0.5f);
        DAT_1020e538.DrawOutlined(SCORE_NUMBER_RIGHT - halfWidth - width, y, scoreText, COLOUR_WHITE, 0, 1.0f,
                                  pop / SCORE_POP_SCALE_DIVISOR + SCORE_BASE_SCALE);
        int previousMode = DAT_10202b14;
        FUN_1013f070(RENDER_BLEND_MODE, BLEND_ADD, 0);
        DAT_1020e538.DrawOutlined(SCORE_NUMBER_RIGHT - halfWidth - width, y, scoreText, COLOUR_WHITE, 0,
                                  rise * SCORE_POP_GLOW, (float)DAT_1020c2ec / SCORE_POP_SCALE_DIVISOR + SCORE_BASE_SCALE);
        FUN_1013f070(RENDER_BLEND_MODE, previousMode, 0);
    }
    else
    {
        DAT_1020e538.DrawOutlined(SCORE_NUMBER_RIGHT - width, SCORE_NUMBER_Y, scoreText, COLOUR_WHITE, 0, 1.0f,
                                  SCORE_BASE_SCALE);
    }
    return 0;
}
