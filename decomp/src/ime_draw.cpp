// Drawing of the Japanese IME composition string (name entry and text input).

#include "font_renderer.h"
#include "ui_vars.h"

#define IME_TEXT_DEPTH 0.0f   // z value of the composition text
#define IME_TEXT_ALPHA 1.0f
#define IME_TEXT_SCALE 1.0f

// Ime_DrawComposition: draws the text before the cursor, the clause being converted (selected or composing
// colour) and the rest of the composition string with `renderer`, each starting where the previous one ended,
// all clipped to the rectangle (a, b)-(c, d). Returns 0 when no composition is active.
// Each arm ends with its own copy of the last draw (the compiler merges the equal ones). Not byte-identical yet: the
// original keeps the selected-colour copy in line after the composing test and puts the composing copy last.
extern "C" int FUN_1015fbf0(FontRenderer *renderer, int x, int y, int a, int b, int c, int d, int colour, int selectedColour, int composingColour)
{
    if (DAT_10f67000 == 0)
        return 0;
    int width = renderer->DrawTextClipped(x, y, DAT_10f66cd0, a, b, c, d, colour, IME_TEXT_DEPTH, IME_TEXT_ALPHA, IME_TEXT_SCALE);
    if (DAT_10f66bc4 != 0)
    {
        width += renderer->DrawTextClipped(x + width, y, DAT_10f68008, a, b, c, d, selectedColour, IME_TEXT_DEPTH, IME_TEXT_ALPHA, IME_TEXT_SCALE);
        renderer->DrawTextClipped(x + width, y, DAT_10f66ab8, a, b, c, d, selectedColour, IME_TEXT_DEPTH, IME_TEXT_ALPHA, IME_TEXT_SCALE);
        return 1;
    }
    width += renderer->DrawTextClipped(x + width, y, DAT_10f68008, a, b, c, d, composingColour, IME_TEXT_DEPTH, IME_TEXT_ALPHA, IME_TEXT_SCALE);
    if (DAT_10f6699c == 0)
    {
        renderer->DrawTextClipped(x + width, y, DAT_10f66ab8, a, b, c, d, composingColour, IME_TEXT_DEPTH, IME_TEXT_ALPHA, IME_TEXT_SCALE);
        return 1;
    }
    renderer->DrawTextClipped(x + width, y, DAT_10f66ab8, a, b, c, d, selectedColour, IME_TEXT_DEPTH, IME_TEXT_ALPHA, IME_TEXT_SCALE);
    return 1;
}
