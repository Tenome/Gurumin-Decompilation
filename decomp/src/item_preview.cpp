// The preview of the selected item: its 3D model drawn over the item menu with its name below.

#include <stdio.h>
#include <string.h>

#include "vec4.h"

#include "matrix4.h"
#include "node.h"
#include "debris.h"
#include "font_renderer.h"
#include "game.h"
#include "game_vars.h"
#include "item_caption.h"
#include "item_info.h"
#include "message_labels.h"
#include "render_vars.h"
#include "tracker.h"
#include "ui_vars.h"

Matrix4 TranslationMatrix(float x, float y, float z);   // 0x101b1360
Matrix4 RotationX(float angle);                         // 0x101b1ad0
Matrix4 RotationY(float angle);                         // 0x101b1bb0
Matrix4 RotationZ(float angle);                         // 0x101b1b40
Matrix4 Scaling(float x, float y, float z);             // 0x101b13c0

extern "C" void FUN_1000cc30(void);                     // Hud_DrawNameBanners (name_banners.cpp)
extern "C" void FUN_1000dba0(void);                     // Fade_Draw (screen_fade.cpp)

// Position of the item name and the sizes of the preview model.
#define ITEM_PREVIEW_TEXT_X 0xcc
#define ITEM_PREVIEW_TEXT_Y 0x16c
#define ITEM_PREVIEW_TEXT_COLOUR 0xffefcf
#define ITEM_PREVIEW_DEFAULT_SCALE 100
#define ITEM_PREVIEW_HEIGHT_UNIT 320.0f
#define ITEM_PREVIEW_SCALE_STEP 0.01f

// Shown instead of the name while no item is selected (Shift-JIS "not obtained.").
#define ITEM_NOT_OBTAINED "\x93\xfc\x8e\xe8\x82\xb5\x82\xc4\x82\xa2\x82\xdc\x82\xb9\x82\xf1\x81\x42"

// 0x10038070 (DrawItemPreview). A C++ function (its temporary matrices need an EH frame). Draws the item menu scene: the menu windows (with the prompt's size), the name banners
// and the scene objects, then the model of the selected item (the cursor of the current window picks it from the item
// list) turned by DAT_104c02f8, scaled by its table percentage and raised by its table height, with its name (or the
// fallback text) at the bottom; with no item selected the "not obtained" line is drawn instead. Ends with the fade and
// the cursor report. Returns 0.
int DrawItemPreview(void)
{
    DAT_104b7200.DrawWindows(DAT_104bf88c);
    DAT_104b7200.DrawWindowMarks(DAT_104bf88c);
    if (DAT_104bf8d4 != 0)
        FUN_1000cc30();
    FUN_1000cf80(&DAT_108392d8);
    if (DAT_104b7200.m_0110[DAT_104b7200.m_0088] == 0)
        DAT_104bfd30 = 0;
    else
        DAT_104bfd30 = DAT_104b7200.m_0140[DAT_104b7200.m_0088];
    if (DAT_1042e2a8[DAT_104bfd30] >= 0)
    {
        Matrix4 placement;
        Matrix4 raised;
        // The table entry's scale (0 = use the default) and height are read together before the scale is chosen.
        int previewScale = DAT_1088e6b0[DAT_1042e2a8[DAT_104bfd30]].previewScale;
        int height = DAT_1088e6b0[DAT_1042e2a8[DAT_104bfd30]].previewHeight;
        int scale = ITEM_PREVIEW_DEFAULT_SCALE;
        if (previewScale != 0)
            scale = previewScale;
        placement = Scaling((float)scale * ITEM_PREVIEW_SCALE_STEP, (float)scale * ITEM_PREVIEW_SCALE_STEP,
                            (float)scale * ITEM_PREVIEW_SCALE_STEP) *
                    RotationZ(DAT_104c02f8) * RotationY(0) * RotationX(0) * TranslationMatrix(0, 0, 0);
        DAT_104d2a08[DAT_1042e2a8[DAT_104bfd30]].StartProximityPass(placement.m, &DAT_108392d8, &DAT_108392d8, 0, 0);
        raised = DAT_108392d8 * TranslationMatrix(0, (float)height / ITEM_PREVIEW_HEIGHT_UNIT, 0);
        DAT_104d2a08[DAT_1042e2a8[DAT_104bfd30]].QueueDraw(&placement, &raised, 0, -1, 0, 0);
        char name[0x100];
        char text[0x100];
        name[0] = 0;
        strcpy(name, DAT_1088e670[DAT_1042e2a8[DAT_104bfd30]].name);
        if (name[0] == 0)
            strcpy(name, DAT_108f667c[DAT_1042e2a8[DAT_104bfd30]].name);
        sprintf(text, name);
        DAT_1020e538.DrawOutlined(ITEM_PREVIEW_TEXT_X, ITEM_PREVIEW_TEXT_Y, text, ITEM_PREVIEW_TEXT_COLOUR, 0, 1.0f,
                                  0.95f);
    }
    else
        DAT_1020e538.DrawOutlined(ITEM_PREVIEW_TEXT_X, ITEM_PREVIEW_TEXT_Y, ITEM_NOT_OBTAINED, ITEM_PREVIEW_TEXT_COLOUR, 0,
                                  1.0f, 0.95f);
    FUN_1000dba0();
    DAT_1020d9e0.M_100014b0();
    return 0;
}
