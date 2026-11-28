// The labels of the message boxes: each open message gets a framed panel above its speaker, with the speaker's name in
// outlined text on top and up to four lines of text below.

#include "font_renderer.h"
#include "game.h"

#include "anim_model.h"
#include "char_vars.h"
#include "game_vars.h"
#include "matrix4.h"
#include "message_labels.h"
#include "message_text.h"
#include "node.h"
#include "ui_vars.h"

#define MESSAGE_LABEL_COUNT 0x7c
#define MESSAGE_LABEL_SCREEN_WIDTH 640
#define MESSAGE_LABEL_SCREEN_HALF_WIDTH 320.0f
#define MESSAGE_LABEL_SCREEN_HALF_HEIGHT 240.0f
#define MESSAGE_LABEL_ANCHOR_HEIGHT 20.0f   // the label hangs above the speaker's head by this much
#define MESSAGE_LABEL_MARGIN 20             // space between the text block and the panel edge
#define MESSAGE_LABEL_PANEL_PADDING 40      // the panel is this much wider than its widest line
#define MESSAGE_LABEL_LINE_HEIGHT 20
#define MESSAGE_LABEL_NAME_HEIGHT -1.1f
#define MESSAGE_LABEL_NAME_BASE 32
#define MESSAGE_LABEL_MIN_Y 32
#define MESSAGE_LABEL_RIGHT_EDGE 0x26c
#define MESSAGE_LABEL_FADE_STEPS 15.0f
#define MESSAGE_LABEL_DEPTH_SCALE 0.01f
#define MESSAGE_LABEL_FADE_DEPTH_SCALE 0.0001f
#define MESSAGE_LABEL_NAME_SCALE 0.88000005f
#define MESSAGE_LABEL_TEXT_SCALE 1.1f
#define MESSAGE_LABEL_NAME_COLOUR 0xffffff
#define MESSAGE_LABEL_NAME_SHADOW_COLOUR 0x603913
#define MESSAGE_LABEL_TEXT_COLOUR 0xffffff
#define MESSAGE_LABEL_LINE_TOP 16

#define WIDER(a, b) ((a) < (b) ? (b) : (a))

// Left edge of the label's text block on screen: the speaker's x minus half the widest line. Written out in the
// original (a named local changes the generated code), so it stays a macro here.
#define LABEL_LEFT (position.x - (float)half)
// The label's fade, 0..1 (DAT_104481d8[i] counts up to MESSAGE_LABEL_FADE_STEPS); evaluated again for every draw call.
// Deliberately without outer parentheses: a parenthesised argument changes the stack slots the compiler picks.
#define LABEL_ALPHA (float)DAT_104481d8[i] / MESSAGE_LABEL_FADE_STEPS

// 0x1000cf80 (Message_DrawLabels). Returns 0 at once while the game is not in mode 2 or 5 and DAT_104c0135 is set.
// Otherwise draws, for every open message box whose first line is not empty, a label at the screen position of the
// speaker (the head node of the character, or its matrix when it has none) as seen through `view`: a panel sized to the
// widest of its four lines (kept inside the screen), the speaker's name drawn with an outline at the top and the lines
// below it; the whole label fades with DAT_104481d8. Returns 1.
// Not byte-identical yet: one scheduling difference in the fourth outline draw (see open.tsv).
extern "C" int FUN_1000cf80(Matrix4 *view)
{
    if (DAT_104bf8c0 != 2 && DAT_104bf8c0 != 5 && DAT_104c0135 != 0)
        return 0;
    int i = 0;
    do
    {
        AnimModel *model = &DAT_10db6ee8[i];
        MessageText *message = &DAT_10453208[i];
        if (DAT_104bfb08[i] != 0 && message->lines[0][0] != 0)
        {
            Vec4 position;
            if (model->headNode != 0)
                position.Copy(model->headNode->worldMatrix.GetTranslation());
            else
                position.Copy(model->matrix.GetTranslation());
            position.z = position.z + MESSAGE_LABEL_ANCHOR_HEIGHT;
            position.Copy(Project(position, *view));
            position.x = (position.x + 1.0f) * MESSAGE_LABEL_SCREEN_HALF_WIDTH;
            DAT_102108cc = 1;
            position.y = (2.0f - (position.y + 1.0f)) * MESSAGE_LABEL_SCREEN_HALF_HEIGHT;
            int lineCount = 0;
            int width0;
            int width1;
            int width2;
            int width3;
            if (message->lines[0][0] != 0)
            {
                width0 = DAT_1020e538.TextWidth(message->lines[0], MESSAGE_LABEL_TEXT_SCALE);
                lineCount = 1;
            }
            else
                width0 = 0;
            if (message->lines[1][0] != 0)
            {
                width1 = DAT_1020e538.TextWidth(message->lines[1], MESSAGE_LABEL_TEXT_SCALE);
                lineCount = 2;
            }
            else
                width1 = 0;
            if (message->lines[2][0] != 0)
            {
                width2 = DAT_1020e538.TextWidth(message->lines[2], MESSAGE_LABEL_TEXT_SCALE);
                lineCount = 3;
            }
            else
                width2 = 0;
            if (message->lines[3][0] != 0)
            {
                width3 = DAT_1020e538.TextWidth(message->lines[3], MESSAGE_LABEL_TEXT_SCALE);
                lineCount = 4;
            }
            else
                width3 = 0;
            int width = WIDER(WIDER(WIDER(width0, width1), width2), width3);
            float fade = (float)DAT_104481d8[i];
            float depth = position.z * MESSAGE_LABEL_DEPTH_SCALE - fade * MESSAGE_LABEL_FADE_DEPTH_SCALE;
            int nameHeight =
                MESSAGE_LABEL_NAME_BASE - (int)((float)(lineCount * MESSAGE_LABEL_LINE_HEIGHT) * MESSAGE_LABEL_NAME_HEIGHT);
            int half = width / 2;
            if (LABEL_LEFT - (float)MESSAGE_LABEL_MARGIN < 0.0f)
                position.x = (float)(half + MESSAGE_LABEL_MARGIN);
            if ((float)MESSAGE_LABEL_SCREEN_WIDTH <
                (LABEL_LEFT - (float)MESSAGE_LABEL_MARGIN) + (float)(width + MESSAGE_LABEL_PANEL_PADDING))
                position.x = (float)((half - width) + MESSAGE_LABEL_RIGHT_EDGE);
            int top = (int)position.y - nameHeight;
            if (top < MESSAGE_LABEL_MIN_Y)
                top = MESSAGE_LABEL_MIN_Y;
            if (depth < 0.0f)
                depth = 0.0f;
            DAT_104b7200.DrawPanel((int)(LABEL_LEFT - (float)MESSAGE_LABEL_MARGIN), top,
                                   width + MESSAGE_LABEL_PANEL_PADDING, nameHeight, depth, fade / MESSAGE_LABEL_FADE_STEPS);
            DAT_102108cc = 0;
            // Name: four copies one pixel off to each side (outline), then two shadow-coloured copies on top.
            DAT_1020e538.DrawText((int)(LABEL_LEFT + 17.0f), top + 1, message->speaker,
                                  MESSAGE_LABEL_NAME_COLOUR, depth, LABEL_ALPHA, MESSAGE_LABEL_NAME_SCALE);
            DAT_1020e538.DrawText((int)((LABEL_LEFT + 16.0f) - 1.0f), top + 1, message->speaker,
                                  MESSAGE_LABEL_NAME_COLOUR, depth, LABEL_ALPHA, MESSAGE_LABEL_NAME_SCALE);
            DAT_1020e538.DrawText((int)(LABEL_LEFT + 17.0f), top - 1, message->speaker,
                                  MESSAGE_LABEL_NAME_COLOUR, depth, LABEL_ALPHA, MESSAGE_LABEL_NAME_SCALE);
            DAT_1020e538.DrawText((int)((LABEL_LEFT + 16.0f) - 1.0f), top - 1, message->speaker,
                                  MESSAGE_LABEL_NAME_COLOUR, depth, LABEL_ALPHA, MESSAGE_LABEL_NAME_SCALE);
            DAT_1020e538.DrawText((int)(LABEL_LEFT + 16.0f), top, message->speaker,
                                  MESSAGE_LABEL_NAME_SHADOW_COLOUR, depth, LABEL_ALPHA, MESSAGE_LABEL_NAME_SCALE);
            DAT_1020e538.DrawText((int)(LABEL_LEFT + 16.0f), top, message->speaker,
                                  MESSAGE_LABEL_NAME_SHADOW_COLOUR, depth, LABEL_ALPHA, MESSAGE_LABEL_NAME_SCALE);
            DAT_102108cc = 1;
            // The text lines, 22 pixels apart, only the non-empty ones.
            if (message->lines[0][0] != 0)
                DAT_1020e538.DrawText((int)LABEL_LEFT, top + MESSAGE_LABEL_LINE_TOP, message->lines[0],
                                      MESSAGE_LABEL_TEXT_COLOUR, depth, LABEL_ALPHA, MESSAGE_LABEL_TEXT_SCALE);
            if (message->lines[1][0] != 0)
                DAT_1020e538.DrawText((int)LABEL_LEFT, (int)((float)(top + MESSAGE_LABEL_LINE_TOP) + 22.0f), message->lines[1],
                                      MESSAGE_LABEL_TEXT_COLOUR, depth, LABEL_ALPHA, MESSAGE_LABEL_TEXT_SCALE);
            if (message->lines[2][0] != 0)
                DAT_1020e538.DrawText((int)LABEL_LEFT, (int)((float)(top + MESSAGE_LABEL_LINE_TOP) + 44.0f), message->lines[2],
                                      MESSAGE_LABEL_TEXT_COLOUR, depth, LABEL_ALPHA, MESSAGE_LABEL_TEXT_SCALE);
            if (message->lines[3][0] != 0)
                DAT_1020e538.DrawText((int)LABEL_LEFT, (int)((float)(top + MESSAGE_LABEL_LINE_TOP) + 66.0f), message->lines[3],
                                      MESSAGE_LABEL_TEXT_COLOUR, depth, LABEL_ALPHA, MESSAGE_LABEL_TEXT_SCALE);
            DAT_102108cc = 0;
        }
        i++;
    } while (i < MESSAGE_LABEL_COUNT);
    return 1;
}
