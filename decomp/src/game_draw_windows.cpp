// Game (DAT_104b7200) menu window drawing: the three window slots with their page tabs, entry lists, scroll marks,
// cursor and caption strip. Coordinates are 640x480 screen pixels.

#include <stdio.h>
#include "font_renderer.h"
#include "game.h"
#include "depth_sort.h"
#include "render_state.h"

#define WINDOW_SLOTS 3
#define WINDOW_OPACITY_STEPS 32.0f      // m_0098 counts up to 32 for a fully open window
#define MARK_BLINK_FRAMES 5             // marks are filled up to this frame and outlined after it

#define ENTRY_KIND_MASK 0xff            // low byte of m_5db4: the kind of entry
#define ENTRY_STATE_MASK 0xf00          // bits 8-11 of m_5db4: non-zero for an entry that cannot be chosen
#define ENTRY_KIND_LABEL 0              // plain label
#define ENTRY_KIND_TOGGLE 1             // label followed by ON/OFF from the variable the entry points at
#define ENTRY_KIND_RANGE 2              // label followed by the entry's lower value
#define ENTRY_KIND_ICON 5               // label after the icon drawn by m_0690

#define PAGE_TAB_NONE 0                 // m_010c: the cursor is in the list
#define PAGE_TAB_PREVIOUS 1
#define PAGE_TAB_NEXT 2

#define COLOUR_ENTRY 0xeedebe
#define COLOUR_ENTRY_SELECTED 0xffffff
#define COLOUR_ENTRY_DISABLED 0x3f2f1f
#define COLOUR_MARK 0xffefcf
#define MARK_SCALE 0.9f
#define CAPTION_SCALE 1.2f

#define PAGE_NUMBER_FORMAT "%d\x83y\x81[\x83W"      // "%d page"
#define PAGE_PREVIOUS_LABEL "\x81\x83\x91O\x95\xc5"   // "< previous page"
#define PAGE_NEXT_LABEL "\x8e\x9f\x95\xc5\x81\x84"    // "next page >"
#define TOGGLE_OFF_FORMAT "%ss160nee"
#define TOGGLE_ON_FORMAT "%ss160nm"
#define RANGE_FORMAT "%ss160"
#define RANGE_VALUE_FORMAT "%ss160%d"
#define VALUE_FORMAT "%d"

#define ENTRY_TEXT_X 0x20
#define ENTRY_ICON_TEXT_X 0x34
#define ENTRY_TOP 0x14
#define PAGE_TAB_Y -0x26
#define CAPTION_PANEL_X 0x8c
#define CAPTION_PANEL_Y 0x198
#define CAPTION_PANEL_WIDTH 0x17c
#define CAPTION_PANEL_HEIGHT 0x40
#define CAPTION_TEXT_X 0xa0
#define CAPTION_TEXT_Y 0x1ae

// Draws a list entry's text at column `columnX` of row `row`: dimmed when its state bits are set, white under the
// cursor, cream otherwise.
#define DRAW_ENTRY_IN(columnX, colour)                                                                                \
    textRenderer->DrawOutlined(m_00b0[w] + (columnX), m_0128[w] * row + ENTRY_TOP + m_00bc[w], text, colour, m_7668[w],\
                               (float)m_0098[w] / WINDOW_OPACITY_STEPS, 1.0f)
#define DRAW_ENTRY(columnX)                                                                                           \
    if ((*state & ENTRY_STATE_MASK) != 0)                                                                             \
        DRAW_ENTRY_IN(columnX, COLOUR_ENTRY_DISABLED);                                                                \
    else if (m_0140[w] == entry && m_010c == PAGE_TAB_NONE)                                                           \
        DRAW_ENTRY_IN(columnX, COLOUR_ENTRY_SELECTED);                                                                \
    else                                                                                                              \
        DRAW_ENTRY_IN(columnX, COLOUR_ENTRY)

// 0x100094e0 (Game_DrawWindows). Draws every open window, the one with the smallest fade first in the sort and
// drawn last: its panel, the page tabs (page number, previous and next page) when it has pages, its extra value, the
// visible entries (label, ON/OFF, range value or icon, dimmed when they cannot be chosen), the scroll marks, and for
// the active window the cursor and the caption its callback gives for the entry under the cursor. `frame` makes the
// marks and the cursor blink. Returns 0.
int Game::DrawWindows(int frame)
{
    int order[WINDOW_SLOTS];
    float fades[WINDOW_SLOTS];
    char pageText[0x80];
    char valueText[0x40];
    char toggleText[0x40];
    char rangeText[0x40];
    char caption[0x80];
    const char *text;
    int (*describeEntry)(int cursor, char *caption);

    fades[0] = m_7668[0];
    fades[1] = m_7668[1];
    fades[2] = m_7668[2];
    order[0] = 0;
    order[1] = 1;
    order[2] = 2;
    FUN_10165020(fades, 0, 0, order, WINDOW_SLOTS);
    for (int n = WINDOW_SLOTS - 1; n >= 0; n--)
    {
        int w = order[n];
        if (m_008c[w] == 0)
            continue;
        if (m_00ec[w] == 0)
        {
            DrawPanelAlt(m_00b0[w], m_00bc[w], m_00c8[w], m_00d4[w], m_7668[w], (float)m_0098[w] / WINDOW_OPACITY_STEPS);
            if (m_00ec[w] == 0 && m_0194[w] != 0)
            {
                DrawPanelAlt(m_00b0[w], m_00bc[w] + PAGE_TAB_Y, 0x80, 0x30, m_7668[w],
                             (float)m_0098[w] / WINDOW_OPACITY_STEPS);
                sprintf(pageText, PAGE_NUMBER_FORMAT, *m_0194[w] + 1);
                textRenderer->DrawOutlined(m_00b0[w] + 0x18, m_00bc[w] - 0x1a, pageText, COLOUR_ENTRY_SELECTED, m_7668[w],
                                           (float)m_0098[w] / WINDOW_OPACITY_STEPS, 1.0f);
                DrawPanelAlt(m_00b0[w] + 0x80, m_00bc[w] + PAGE_TAB_Y, 200, 0x30, m_7668[w],
                             (float)m_0098[w] / WINDOW_OPACITY_STEPS);
                sprintf(pageText, PAGE_PREVIOUS_LABEL, *m_0194[w] + 1);
                if (m_010c == PAGE_TAB_PREVIOUS)
                    textRenderer->DrawOutlined(m_00b0[w] + 0xa0, m_00bc[w] - 0x1a, pageText, COLOUR_ENTRY_SELECTED,
                                               m_7668[w], (float)m_0098[w] / WINDOW_OPACITY_STEPS, 1.0f);
                else
                    textRenderer->DrawOutlined(m_00b0[w] + 0xa0, m_00bc[w] - 0x1a, pageText, COLOUR_ENTRY, m_7668[w],
                                               (float)m_0098[w] / WINDOW_OPACITY_STEPS, 1.0f);
                sprintf(pageText, PAGE_NEXT_LABEL, *m_0194[w] + 1);
                if (m_010c == PAGE_TAB_NEXT)
                    textRenderer->DrawOutlined(m_00b0[w] + 0xf0, m_00bc[w] - 0x1a, pageText, COLOUR_ENTRY_SELECTED,
                                               m_7668[w], (float)m_0098[w] / WINDOW_OPACITY_STEPS, 1.0f);
                else
                    textRenderer->DrawOutlined(m_00b0[w] + 0xf0, m_00bc[w] - 0x1a, pageText, COLOUR_ENTRY, m_7668[w],
                                               (float)m_0098[w] / WINDOW_OPACITY_STEPS, 1.0f);
            }
        }
        if (m_01b8[w] != 0)
        {
            sprintf(valueText, VALUE_FORMAT, *m_01b8[w]);
            textRenderer->DrawOutlined(m_01c4[w] + m_00b0[w], m_01d0[w] + m_00bc[w], valueText, COLOUR_ENTRY, m_7668[w],
                                       (float)m_0098[w] / WINDOW_OPACITY_STEPS, 1.0f);
        }
        for (int row = 0; row < m_011c[w] + 1; row++)
        {
            int entry = m_0134[w] + row;
            if (m_00ec[w] != 0 || entry >= 0x80 || m_069c[w][entry][0] == 0)
                continue;
            unsigned int *state = (unsigned int *)&m_5db4[w][entry];
            text = m_069c[w][entry];
            int kind = *state & ENTRY_KIND_MASK;
            if (kind == ENTRY_KIND_ICON)
            {
                if (m_0690 != 0)
                    m_0690(m_00b0[w] + ENTRY_TEXT_X, m_0128[w] * row + ENTRY_TOP + m_00bc[w], entry,
                           (float)m_0098[w] / WINDOW_OPACITY_STEPS);
                DRAW_ENTRY(ENTRY_ICON_TEXT_X);
            }
            else if (kind == ENTRY_KIND_LABEL)
            {
                DRAW_ENTRY(ENTRY_TEXT_X);
            }
            else if (kind == ENTRY_KIND_TOGGLE && m_579c[w].entries[entry] != 0)
            {
                const char *format;
                if (*m_579c[w].entries[entry] == 0)
                    format = TOGGLE_OFF_FORMAT;
                else
                    format = TOGGLE_ON_FORMAT;
                sprintf(toggleText, format, text);
                text = toggleText;
                DRAW_ENTRY(ENTRY_TEXT_X);
            }
            else if (kind == ENTRY_KIND_RANGE)
            {
                int low = m_69e4[w][entry];
                const char *format;
                if (low < 1)
                    format = RANGE_FORMAT;
                else
                    format = RANGE_VALUE_FORMAT;
                sprintf(rangeText, format, text, low);
                text = rangeText;
                DRAW_ENTRY(ENTRY_TEXT_X);
            }
        }
        if (m_0134[w] > 0)
        {
            if (frame > MARK_BLINK_FRAMES)
                textRenderer->DrawText(m_00b0[w] + ENTRY_TEXT_X, m_00bc[w], MARK_UP_OUTLINE, COLOUR_MARK, m_7668[w],
                                       (float)m_0098[w] / WINDOW_OPACITY_STEPS, MARK_SCALE);
            else
                textRenderer->DrawText(m_00b0[w] + ENTRY_TEXT_X, m_00bc[w], MARK_UP, COLOUR_MARK, m_7668[w],
                                       (float)m_0098[w] / WINDOW_OPACITY_STEPS, MARK_SCALE);
        }
        if (m_011c[w] < 0x80 && m_069c[w][m_0134[w] + m_011c[w] + 1][0] != 0)
        {
            if (frame > MARK_BLINK_FRAMES)
                textRenderer->DrawText(m_00b0[w] + ENTRY_TEXT_X, m_0128[w] * m_011c[w] + 0x2c + m_00bc[w],
                                       MARK_DOWN_OUTLINE, COLOUR_MARK, m_7668[w], (float)m_0098[w] / WINDOW_OPACITY_STEPS,
                                       MARK_SCALE);
            else
                textRenderer->DrawText(m_00b0[w] + ENTRY_TEXT_X, m_0128[w] * m_011c[w] + 0x2c + m_00bc[w], MARK_DOWN,
                                       COLOUR_MARK, m_7668[w], (float)m_0098[w] / WINDOW_OPACITY_STEPS, MARK_SCALE);
        }
        if (m_0088 == w)
        {
            if (m_0110[w] != 0 && m_00ec[w] == 0)
            {
                if (m_010c == PAGE_TAB_PREVIOUS)
                {
                    FUN_1013f070(RENDER_BLEND_MODE, BLEND_ADD, 0);
                    DrawCursor(m_00b0[w] + 0x90, m_00bc[w] - 0x1e, (float)m_0098[w] / WINDOW_OPACITY_STEPS, frame);
                }
                else if (m_010c == PAGE_TAB_NEXT)
                {
                    FUN_1013f070(RENDER_BLEND_MODE, BLEND_ADD, 0);
                    DrawCursor(m_00b0[w] + 0xe0, m_00bc[w] - 0x1e, (float)m_0098[w] / WINDOW_OPACITY_STEPS, frame);
                }
                else
                {
                    FUN_1013f070(RENDER_BLEND_MODE, BLEND_ADD, 0);
                    DrawCursor(m_00b0[w] + 6, (m_0140[w] - m_0134[w]) * m_0128[w] + 0xc + m_00bc[w],
                               (float)m_0098[w] / WINDOW_OPACITY_STEPS, frame);
                }
                FUN_1013f070(RENDER_BLEND_MODE, BLEND_ALPHA, 0);
            }
            if (m_0088 == w && (describeEntry = m_7654[w]) != 0)
            {
                caption[0] = 0;
                describeEntry(m_0140[w], caption);
                if (caption[0] != 0)
                {
                    DrawPanelAlt(CAPTION_PANEL_X, CAPTION_PANEL_Y, CAPTION_PANEL_WIDTH, CAPTION_PANEL_HEIGHT, 0,
                                 (float)m_0098[w] / WINDOW_OPACITY_STEPS);
                    textRenderer->DrawOutlined(CAPTION_TEXT_X, CAPTION_TEXT_Y, caption, COLOUR_MARK, 0,
                                               (float)m_0098[w] / WINDOW_OPACITY_STEPS, CAPTION_SCALE);
                }
            }
        }
    }
    return 0;
}
