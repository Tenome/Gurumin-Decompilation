// Turning the tutorial book to the next or previous pair of pages (run from the tutorial mode 0x10065450). Each
// of the 0x22 pages has a texture name (tute_lp_iti...) and a script flag that unlocks it; locked pages in the
// second book (DAT_104c03e8) show the placeholder dpartsbf.bmp instead.

#include <string.h>
#include "texture_load.h"
#include "char_state.h"
#include "anim_model.h"

// "最後のページです。" ("This is the last page."), Shift-JIS.
#define LAST_PAGE_TEXT "\x8d\xc5\x8c\xe3\x82\xcc\x83\x79\x81\x5b\x83\x57\x82\xc5\x82\xb7\x81\x42"
// "最初のページです。" ("This is the first page."), Shift-JIS.
#define FIRST_PAGE_TEXT "\x8d\xc5\x8f\x89\x82\xcc\x83\x79\x81\x5b\x83\x57\x82\xc5\x82\xb7\x81\x42"

#include "node.h"

#include "game.h"

#include "texture_manager.h"

#include "game_vars.h"
#include "ui_vars.h"
#include "tutorial_book.h"


extern "C" int FUN_1001e1a0(const char *name, const char *alphaName);  // Texture_Load
extern "C" int FUN_1000ed70(const char *text, int style, int frames);  // shows a message
extern "C" int FUN_1003ad20(int id, const float *position, float volume);  // Sound_PlayEffect

// 0x1004f4f0 (TutorialBook_NextPage). On the last page (0x0e, or 0x1c in the second book) shows LAST_PAGE_TEXT
// (at most every 60 frames). Otherwise loads the cover and the shown pages again, then looks for the next two
// pages after the right one: the first becomes the left page, the second the right (a locked page in the second
// book shows dpartsbf.bmp). When one is found the sheet turns ("pageup", textures paper2/paper) over the halves,
// the left page is remembered in flag 0x85c (0x85d in the second book) and the page sound 0x3be plays. With no page
// left, the message box is set to LAST_PAGE_TEXT instead. Returns 0.
extern "C" int FUN_1004f4f0(void)
{
    int found = 0;
    DAT_104c03e0 = 0;
    if (DAT_104c03e8 != 0 ? DAT_102194c0 > 0x1c : DAT_102194c0 > 0xe)
    {

        DAT_104c03e4 = 0;
        if (DAT_1020b430[PLAYER_SLOT].messageDelay == 0)
        {
            DAT_1020b430[PLAYER_SLOT].messageDelay = 0x3c;
            FUN_1000ed70(LAST_PAGE_TEXT, 0, 0x28);
        }
        return 0;
    }
    DAT_104c03e4 = 1;
    int cover = FUN_1001e1a0(DAT_102028e8[0], 0);
    DAT_1030b870[0] = cover;
    int newLeft = cover;
    int left = FUN_1001e1a0(DAT_102028e8[DAT_104483dc], 0);
    DAT_1030b870[DAT_104483dc] = left;
    int right = FUN_1001e1a0(DAT_102028e8[DAT_104483e0], 0);
    DAT_1030b870[DAT_104483e0] = right;
    DAT_10836c70.ResetFrameTimer();
    for (int i = DAT_102194c4 + 1; i < 0x22; i++)
    {
        if (DAT_1030c908[i] == 0)
            continue;
        if (found == 0)
        {
            if (DAT_104c03e8 == 0 || DAT_1030c908[i] == -1 ||
                (DAT_1030c908[i] > 0 && DAT_10874848.TestScriptFlag(DAT_1030c908[i]) != 0))
            {
                newLeft = FUN_1001e1a0(DAT_102028e8[i], 0);
                DAT_1030b870[i] = newLeft;
                DAT_104483dc = i;
            }
            else
            {
                newLeft = FUN_1001e1a0(DAT_10202a48, 0);
                DAT_1030b870[0x16] = newLeft;
                DAT_104483dc = 0x16;
            }
            found = 1;
            DAT_102194c0 = i;
        }
        else
        {
            if (DAT_104c03e8 != 0)
            {
                if (DAT_1030c908[i] == -1)
                {
                    right = FUN_1001e1a0(DAT_102028e8[i], 0);
                    DAT_1030b870[i] = right;
                    DAT_104483e0 = i;
                }
                else if (DAT_1030c908[i] > 0 && DAT_10874848.TestScriptFlag(DAT_1030c908[i]) != 0)
                {
                    right = FUN_1001e1a0(DAT_102028e8[i], 0);
                    DAT_1030b870[i] = right;
                    DAT_104483e0 = i;
                }
                else
                {
                    right = FUN_1001e1a0(DAT_10202a48, 0);
                    DAT_1030b870[0x16] = right;
                    DAT_104483e0 = 0x16;
                }
            }
            else
            {
                right = FUN_1001e1a0(DAT_102028e8[i], 0);
                DAT_1030b870[i] = right;
                DAT_104483e0 = i;
            }
            DAT_102194c4 = i;
            found++;
            break;
        }
    }
    if (found == 0)
    {
        DAT_104c03e4 = 0;
        DAT_104c03e0 = 0;
        DAT_104bfd4c = 0x28;
        DAT_104c01fc = 0;
        DAT_104bf8c8 = 1;
        DAT_104bf8cc = 0;
        DAT_104bf8d0 = 0;
        DAT_10467ea0[0] = 0;
        strcpy(DAT_10219bc0, LAST_PAGE_TEXT);
        return 0;
    }
    if (found < 2)
        DAT_102194c4 = 0;
    if (DAT_104c03e8 != 0)
        DAT_10c62edc[0x85d] = DAT_102194c0;
    else
        DAT_10c62edc[0x85c] = DAT_102194c0;
    if (DAT_104c03cc != 0)
    {
        DAT_104c03cc->SetMaterialValue("paper2", newLeft);
        DAT_104c03cc->SetMaterialValue("paper", right);
        DAT_104c03cc->RestartColourPlugs("pageup", DAT_102239c8 * 2);
    }
    if (DAT_104c03d0 != 0)
    {
        DAT_104c03d0->SetMaterialValue("paper", cover);
        DAT_104c03d0->RestartColourPlugs("stop0", DAT_102239c8 * 2);
    }
    if (DAT_104c03d4 != 0)
    {
        DAT_104c03d4->SetMaterialValue("paper2", left);
        DAT_104c03d4->RestartColourPlugs("stop0", DAT_102239c8 * 2);
    }
    DAT_10836c70.ResetFrameTimer();
    FUN_1003ad20(0x3be, 0, 1.0f);
    return 0;
}

// 0x1004f860 (TutorialBook_PrevPage). The mirror of TutorialBook_NextPage: on the first page (below 3, or 0x19 in
// the second book) shows FIRST_PAGE_TEXT (at most every 60 frames). Otherwise loads the cover and the shown pages
// again and looks backwards from the page before the left one: the first page found becomes the right page, the
// second the left. The sheet turns back ("pagedown", textures paper/paper2) over the halves ("stop1"), the left
// page is remembered in flag 0x85c (0x85d in the second book) and the page sound 0x3be plays. With no page before,
// the message box is set to FIRST_PAGE_TEXT instead. Returns 0.
extern "C" int FUN_1004f860(void)
{
    int atFirstPage;
    if (DAT_104c03e8 != 0)
        atFirstPage = DAT_102194c0 < 0x19;
    else
        atFirstPage = DAT_102194c0 < 3;
    DAT_104c03e0 = 0;
    if (atFirstPage)
    {
        DAT_104c03e4 = 0;
        if (DAT_1020b430[PLAYER_SLOT].messageDelay == 0)
        {
            DAT_1020b430[PLAYER_SLOT].messageDelay = 0x3c;
            FUN_1000ed70(FIRST_PAGE_TEXT, 0, 0x28);
        }
        return 0;
    }
    DAT_104c03e4 = 2;
    int found = 0;
    int cover = FUN_1001e1a0(DAT_102028e8[0], 0);
    DAT_1030b870[0] = cover;
    int newRight = cover;
    int newLeft = cover;
    int left = FUN_1001e1a0(DAT_102028e8[DAT_104483dc], 0);
    DAT_1030b870[DAT_104483dc] = left;
    int right = FUN_1001e1a0(DAT_102028e8[DAT_104483e0], 0);
    DAT_1030b870[DAT_104483e0] = right;
    DAT_10836c70.ResetFrameTimer();
    for (int i = DAT_102194c0 - 1 > 0 ? DAT_102194c0 - 1 : 0; i >= 0; i--)
    {
        int flag = DAT_1030c908[i];
        if (flag == 0)
            continue;
        if (found == 0)
        {
            if (DAT_104c03e8 == 0 || flag == -1 || (flag > 0 && DAT_10874848.TestScriptFlag(flag) != 0))
            {
                newRight = FUN_1001e1a0(DAT_102028e8[i], 0);
                DAT_1030b870[i] = newRight;
                DAT_104483e0 = i;
            }
            else
            {
                newRight = FUN_1001e1a0(DAT_10202a48, 0);
                DAT_1030b870[0x16] = newRight;
                DAT_104483e0 = 0x16;
            }
            found = 1;
            DAT_102194c4 = i;
        }
        else
        {
            if (DAT_104c03e8 != 0)
            {
                if (DAT_1030c908[i] == -1)
                {
                    newLeft = FUN_1001e1a0(DAT_102028e8[i], 0);
                    DAT_1030b870[i] = newLeft;
                    DAT_104483dc = i;
                }
                else if (DAT_1030c908[i] > 0 && DAT_10874848.TestScriptFlag(DAT_1030c908[i]) != 0)
                {
                    newLeft = FUN_1001e1a0(DAT_102028e8[i], 0);
                    DAT_1030b870[i] = newLeft;
                    DAT_104483dc = i;
                }
                else
                {
                    newLeft = FUN_1001e1a0(DAT_10202a48, 0);
                    DAT_1030b870[0x16] = newLeft;
                    DAT_104483dc = 0x16;
                }
            }
            else
            {
                newLeft = FUN_1001e1a0(DAT_102028e8[i], 0);
                DAT_1030b870[i] = newLeft;
                DAT_104483dc = i;
            }
            found++;
            DAT_102194c0 = i;
            break;
        }
    }
    if (DAT_104c03e8 != 0)
        DAT_10c62edc[0x85d] = DAT_102194c0;
    else
        DAT_10c62edc[0x85c] = DAT_102194c0;
    if (found == 0)
    {
        DAT_104c03e4 = 0;
        DAT_104c03e0 = 0;
        DAT_104bfd4c = 0x28;
        DAT_104c01fc = 0;
        DAT_104bf8c8 = 1;
        DAT_104bf8cc = 0;
        DAT_104bf8d0 = 0;
        DAT_10467ea0[0] = 0;
        strcpy(DAT_10219bc0, FIRST_PAGE_TEXT);
        return 0;
    }
    if (found < 2)
        DAT_102194c0 = DAT_102194c4;
    if (DAT_104c03cc != 0)
    {
        DAT_104c03cc->SetMaterialValue("paper", newRight);
        DAT_104c03cc->SetMaterialValue("paper2", left);
        DAT_104c03cc->RestartColourPlugs("pagedown", DAT_102239c8 * 2);
    }
    if (DAT_104c03d0 != 0)
    {
        DAT_104c03d0->SetMaterialValue("paper", right);
        DAT_104c03d0->RestartColourPlugs("stop1", DAT_102239c8 * 2);
    }
    if (DAT_104c03d4 != 0)
    {
        DAT_104c03d4->SetMaterialValue("paper2", newLeft);
        DAT_104c03d4->RestartColourPlugs("stop1", DAT_102239c8 * 2);
    }
    DAT_10836c70.ResetFrameTimer();
    FUN_1003ad20(0x3be, 0, 1.0f);
    return 0;
}

// 0x1004f250 (TutorialBook_Close). Starts closing the book (state 5): loads the cover texture (LoadTexture, inline)
// and the first two pages from the left one on, the sheet turns back ("pagedown") with the first page and the
// cover, the halves stop ("stop0" with the second page, "stop2") and the cover closes ("close"); the book sound
// 0x3bd plays. Returns 0.
extern "C" int FUN_1004f250(void)
{
    DAT_104c03e4 = 5;
    DAT_104c03e0 = 0;
    int cover = LoadTexture(DAT_102028e8[0], 0);
    int found = 0;
    DAT_1030b870[0] = cover;
    int left = cover;
    int right = cover;
    for (int i = DAT_102194c0; i < 0x22; i++)
    {
        if (DAT_1030c908[i] == 0)
            continue;
        if (found == 0)
        {
            if (DAT_104c03e8 == 0 || DAT_1030c908[i] == -1 ||
                (DAT_1030c908[i] > 0 && DAT_10874848.TestScriptFlag(DAT_1030c908[i]) != 0))
            {
                left = FUN_1001e1a0(DAT_102028e8[i], 0);
                DAT_1030b870[i] = left;
                found = 1;
            }
            else
            {
                left = FUN_1001e1a0(DAT_10202a48, 0);
                DAT_1030b870[0x16] = left;
                found = 1;
            }
        }
        else
        {
            if (DAT_104c03e8 != 0)
            {
                if (DAT_1030c908[i] == -1)
                {
                    right = FUN_1001e1a0(DAT_102028e8[i], 0);
                    DAT_1030b870[i] = right;
                }
                else if (DAT_1030c908[i] > 0 && DAT_10874848.TestScriptFlag(DAT_1030c908[i]) != 0)
                {
                    right = FUN_1001e1a0(DAT_102028e8[i], 0);
                    DAT_1030b870[i] = right;
                }
                else
                {
                    right = FUN_1001e1a0(DAT_10202a48, 0);
                    DAT_1030b870[0x16] = right;
                }
            }
            else
            {
                right = FUN_1001e1a0(DAT_102028e8[i], 0);
                DAT_1030b870[i] = right;
            }
            break;
        }
    }
    if (DAT_104c03cc != 0)
    {
        DAT_104c03cc->SetMaterialValue("paper2", left);
        DAT_104c03cc->SetMaterialValue("paper", cover);
        DAT_104c03cc->RestartColourPlugs("pagedown", DAT_102239c8 * 2);
    }
    if (DAT_104c03d0 != 0)
    {
        DAT_104c03d0->SetMaterialValue("paper", right);
        DAT_104c03d0->RestartColourPlugs("stop0", DAT_102239c8 * 2);
    }
    if (DAT_104c03d4 != 0)
        DAT_104c03d4->RestartColourPlugs("stop2", DAT_102239c8 * 2);
    if (DAT_104c03d8 != 0)
        DAT_104c03d8->RestartColourPlugs("close", DAT_102239c8 * 2);
    DAT_10836c70.ResetFrameTimer();
    FUN_1003ad20(0x3bd, 0, 1.0f);
    return 0;
}
