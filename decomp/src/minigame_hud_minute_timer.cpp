// Map 0x18 mini game HUD: the minutes and seconds timer (M:SS.hh), the count-down / start / finish banner cut from the
// "minigm" sheet, and two score counters.

#include <stdlib.h>
#include "font_renderer.h"
#include "map_vars.h"
#include "minigame_banner.h"
#include "minigame_hud.h"
#include "minigame_timer.h"
#include "render_state.h"
#include "texture_object.h"

#define SECONDS_PER_MINUTE 60
#define TIMER_START_WORD_FRAMES 0x14fa

// The minutes digit and the colon after it, left of the seconds digits.
#define TIMER_MINUTES_LEFT 0x19e
#define TIMER_COLON_LEFT 0x1be

// Score counters: the player's score right-aligned beside its colon label, popping with a growing glow, and the
// second (pink) score at the right edge popping the same way around its centre.
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

// The one-character label drawn after the score (cp932 0x81 0x46, a full-width colon).
#define SCORE_LABEL_TEXT "\x81\x46"

// Half of a text width, rounded to the nearest pixel.
static inline int HalfWidth(int width)
{
    return (int)(((float)width + 0.5f) / 2.0f);
}

// 0x100311b0 (MapMinigame_DrawHudMinuteTimer). Map 0x18 variant: draws the remaining time as M:SS.hh (digits cut from
// the map's count-down texture), then the banner (count-down digits, the finish word once the timer has run out,
// or the start word once the timer is past TIMER_START_WORD_FRAMES), each as a shrinking quad plus an additive copy at
// GLOW_ALPHA of the alpha; then the player's score beside a colon label and a second pink score, each popping larger
// with a glow while its pop counter runs. Draws no banner when the map holds no textures. Returns 0.
extern "C" int FUN_100311b0(void)
{
    char scoreText[12];
    // Half sizes of the banner quad and of its glow copy, shared by the banner branches (declared once here: as
    // per-branch locals their spill slots differ from the original).
    int halfW, halfH, glowHalfW, glowHalfH;
    float fade;
    if (DAT_102028c8 < 0)
        return 0;
    int timer = DAT_10c6505c;
    float frames = (float)(timer % TIMER_FRAMES_PER_SECOND);
    int seconds = timer / TIMER_FRAMES_PER_SECOND;
    int minutes = seconds / SECONDS_PER_MINUTE;
    int secondsInMinute = seconds % SECONDS_PER_MINUTE;
    int secondsTens = secondsInMinute / 10;
    int secondsOnes = secondsInMinute % 10;
    int hundredths = (int)(frames * TIMER_HUNDREDTHS_PER_FRAME);
    int hundredthsTens = hundredths / 10;
    int hundredthsOnes = hundredths % 10;
    if (timer < 0)
    {
        minutes = secondsOnes = 0;
        hundredthsTens = secondsTens = 0;
        hundredthsOnes = 0;
    }
    DRAW_TIMER_DIGIT(TIMER_MINUTES_LEFT, TIMER_DIGIT_TOP, TIMER_COLON_LEFT, TIMER_DIGIT_BOTTOM, minutes);
    DAT_118d0ba8[DAT_102028c8].DrawColoured(TIMER_COLON_LEFT, TIMER_DIGIT_TOP, TIMER_TENS_LEFT, TIMER_DIGIT_BOTTOM,
                                            TIMER_DOT_U0, TIMER_DOT_V0, TIMER_DOT_U1, TIMER_DOT_V1, 0.0f, 1.0f, 0.0f,
                                            1.0f, 1.0f, 1.0f);
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
            fade = (float)(BANNER_FRAMES_PER_STEP - DAT_1020c2e8 % BANNER_FRAMES_PER_STEP) / BANNER_FRAMES_PER_STEP_F;
            halfW = SCALED(DIGIT_BASE, fade, DIGIT_RATE);
            int glowHalf = SCALED(DIGIT_BASE, fade, DIGIT_GLOW_RATE);
            float u1 = ((float)step * DIGIT_CELL_TEXELS + DIGIT_CELL_LAST_TEXEL) / SHEET_WIDTH;
            float u0 = ((float)step * DIGIT_CELL_TEXELS + DIGIT_CELL_FIRST_TEXEL) / SHEET_WIDTH;
            BANNER_QUAD(slot, halfW, halfW, u0, DIGIT_V0, u1, DIGIT_V1, fade);
            BANNER_GLOW(glowHalf, glowHalf, u0, DIGIT_V0, u1, DIGIT_V1, fade);
        }
        else if (minutes == 0 && secondsTens == 0 && secondsOnes < TIMER_COUNT_DOWN_SECONDS)
        {
            if (timer > 0)
            {
                fade = frames / BANNER_FRAMES_PER_STEP_F;
                halfW = SCALED(DIGIT_BASE, fade, DIGIT_RATE);
                int glowHalf = SCALED(DIGIT_BASE, fade, DIGIT_GLOW_RATE);
                float u1 = ((float)(2 - secondsOnes) * DIGIT_CELL_TEXELS + DIGIT_CELL_LAST_TEXEL) / SHEET_WIDTH;
                float u0 = ((float)(2 - secondsOnes) * DIGIT_CELL_TEXELS + DIGIT_CELL_FIRST_TEXEL) / SHEET_WIDTH;
                BANNER_QUAD(slot, halfW, halfW, u0, DIGIT_V0, u1, DIGIT_V1, fade);
                BANNER_GLOW(glowHalf, glowHalf, u0, DIGIT_V0, u1, DIGIT_V1, fade);
            }
            else
            {
                if (timer > -BANNER_FRAMES_PER_STEP)
                    fade = (float)(timer + BANNER_FRAMES_PER_STEP) / BANNER_FRAMES_PER_STEP_F;
                else
                    fade = 0.0f;
                halfW = SCALED(WORD_BASE, fade, WORD_WIDTH_RATE);
                halfH = SCALED(DIGIT_BASE, fade, WORD_HEIGHT_RATE);
                glowHalfW = SCALED(WORD_BASE, fade, WORD_GLOW_WIDTH_RATE);
                glowHalfH = SCALED(DIGIT_BASE, fade, WORD_GLOW_HEIGHT_RATE);
                BANNER_QUAD(slot, halfW, halfH, WORD_U0, FINISH_V0, WORD_U1, FINISH_V1, 1.0f);
                BANNER_GLOW(glowHalfW, glowHalfH, WORD_U0, FINISH_V0, WORD_U1, FINISH_V1, fade);
            }
        }
        else
        {
            if (timer > TIMER_START_WORD_FRAMES)
            {
                fade = frames / BANNER_FRAMES_PER_STEP_F;
                halfW = SCALED(WORD_BASE, fade, WORD_WIDTH_RATE);
                halfH = SCALED(DIGIT_BASE, fade, WORD_HEIGHT_RATE);
                glowHalfW = SCALED(WORD_BASE, fade, WORD_GLOW_WIDTH_RATE);
                glowHalfH = SCALED(DIGIT_BASE, fade, WORD_GLOW_HEIGHT_RATE);
                BANNER_QUAD(slot, halfW, halfH, WORD_U0, START_V0, WORD_U1, START_V1, fade);
                BANNER_GLOW(glowHalfW, glowHalfH, WORD_U0, START_V0, WORD_U1, START_V1, fade);
            }
            else if (timer < 0)
            {
                if (timer > -BANNER_FRAMES_PER_STEP)
                    fade = (float)(timer + BANNER_FRAMES_PER_STEP) / BANNER_FRAMES_PER_STEP_F;
                else
                    fade = 0.0f;
                halfW = SCALED(WORD_BASE, fade, WORD_WIDTH_RATE);
                halfH = SCALED(DIGIT_BASE, fade, WORD_HEIGHT_RATE);
                glowHalfW = SCALED(WORD_BASE, fade, WORD_GLOW_WIDTH_RATE);
                glowHalfH = SCALED(DIGIT_BASE, fade, WORD_GLOW_HEIGHT_RATE);
                BANNER_QUAD(slot, halfW, halfH, WORD_U0, FINISH_V0, WORD_U1, FINISH_V1, 1.0f);
                BANNER_GLOW(glowHalfW, glowHalfH, WORD_U0, FINISH_V0, WORD_U1, FINISH_V1, fade);
            }
        }
    }
    _itoa(DAT_10c65060, scoreText, SCORE_RADIX);
    int width = DAT_1020e538.TextWidth(scoreText, SCORE_SIZING_SCALE);
    DAT_1020e538.DrawOutlined(SCORE_LABEL_X, SCORE_LABEL_Y, SCORE_LABEL_TEXT, COLOUR_WHITE, 0, 1.0f, SCORE_BASE_SCALE);
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
    _itoa(DAT_10c65330, scoreText, SCORE_RADIX);
    if (DAT_1020c2e4 != 0)
    {
        float grownWidth = (float)DAT_1020e538.TextWidth(scoreText, (float)DAT_1020c2e4 / SCORE_POP_SCALE_DIVISOR +
                                                                        SCORE_BASE_SCALE);
        float baseWidth = (float)DAT_1020e538.TextWidth(scoreText, SCORE_BASE_SCALE);
        int growth = (int)((grownWidth - baseWidth) / 2.0f + 0.5f);
        float pop = (float)DAT_1020c2e4;
        float rise = pop / SCORE_POP_RISE_DIVISOR;
        int y = SCORE_NUMBER_Y - (int)(rise * SCORE_POP_RISE + 0.5f);
        DAT_1020e538.DrawOutlined(SECOND_SCORE_X - growth, y, scoreText, COLOUR_PINK, 0, 1.0f,
                                  pop / SCORE_POP_SCALE_DIVISOR + SCORE_BASE_SCALE);
        int previousMode = DAT_10202b14;
        FUN_1013f070(RENDER_BLEND_MODE, BLEND_ADD, 0);
        DAT_1020e538.DrawOutlined(SECOND_SCORE_X - growth, y, scoreText, COLOUR_WHITE, 0,
                                  rise * SCORE_POP_GLOW, (float)DAT_1020c2e4 / SCORE_POP_SCALE_DIVISOR + SCORE_BASE_SCALE);
        FUN_1013f070(RENDER_BLEND_MODE, previousMode, 0);
    }
    else
    {
        DAT_1020e538.DrawOutlined(SECOND_SCORE_X, SCORE_NUMBER_Y, scoreText, COLOUR_PINK, 0, 1.0f, SCORE_BASE_SCALE);
    }
    return 0;
}
