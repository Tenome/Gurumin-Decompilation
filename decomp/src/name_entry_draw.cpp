// The scene routine of the name entry (game mode 10): draws the name box with the typed name and the IME, then the
// difficulty description that goes with the face picture on the player model, or, once the name is confirmed, the
// summary of name and difficulty. NameEntry_Update installs it as the scene routine (DAT_10f6553c) on its first frame.

#include "vec4.h"
#include "matrix4.h"
#include "node.h"
#include "anim_model.h"
#include "font_renderer.h"
#include "game.h"
#include "game_load_data.h"
#include "game_vars.h"
#include "hud_frame.h"
#include "name_entry.h"
#include "char_vars.h"
#include "tracker.h"
#include "ui_vars.h"

// Shift-JIS texts. "s004", "s064" and "s075" in the descriptions are the text engine's position tags.
#define TEXT_NAME "\x96\xbc\x91O"
#define TEXT_DIFFICULTY "\x93\xef\x88\xd5\x93x"
#define TEXT_LABEL_CRAZY "\x82" "b\x82\x92\x82\x81\x82\x9a\x82\x99"
#define TEXT_LABEL_PLEASURE "\x82o\x82\x8c\x82\x85\x82\x81\x82\x93\x82\x95\x82\x92\x82\x85"
#define TEXT_LABEL_BEGINNER "\x82" "a\x82\x85\x82\x87\x82\x89\x82\x8e\x82\x8e\x82\x85\x82\x92"
#define TEXT_LABEL_HARD "\x82g\x82\x81\x82\x92\x82\x84"
#define TEXT_LABEL_NORMAL "\x82m\x82\x8f\x82\x92\x82\x8d\x82\x81\x82\x8c"
#define TEXT_NORMAL_ENEMY "\x93G\x82\xcc\x8b\xad\x82\xb3s064\x81" "Fs075\x95\x81\x92\xca\x83\x8c\x83x\x83\x8b"
#define TEXT_NORMAL_OTHER "s004\x82\xbb\x82\xcc\x91\xbcs064\x81" "Fs075\x82" "f\x82\x81\x82\x8d\x82\x85\x82n\x82\x96\x82\x85\x82\x92\x8c\xe3\x82\xc9\x94\xbc\x95\xaa\x89\xf1\x95\x9c\x82\xb5\x82\xc4\x8d\xc4\x83X\x83^\x81[\x83g\x81" "B"
#define TEXT_PLAYFUL_ENEMY "\x93G\x82\xcc\x8b\xad\x82\xb3s064\x81" "Fs075\x82\xa8\x97V\x82\xd1\x83\x8c\x83x\x83\x8b"
#define TEXT_PLAYFUL_JUMP "s004\x82\xbb\x82\xcc\x91\xbcs064\x81" "Fs075\x83W\x83\x83\x83\x93\x83v\x82\xaa\x8e\xa9\x93\xae\x93I\x82\xc9\x95\xe2\x90\xb3\x81" "B\x95X\x82\xc5\x8a\x8a\x82\xe7\x82\xc8\x82\xa2\x81" "B"
#define TEXT_PLAYFUL_HEAL "s075\x82" "f\x82\x81\x82\x8d\x82\x85\x82n\x82\x96\x82\x85\x82\x92\x8c\xe3\x82\xc9\x91S\x89\xf1\x95\x9c\x82\xb5\x82\xc4\x8d\xc4\x83X\x83^\x81[\x83g\x81" "B"
#define TEXT_PLAYFUL_TRAPS "s075\x83g\x83\x89\x83" "b\x83v\x82\xe2\x8a\x8a\x97\x8e\x83_\x83\x81\x81[\x83W\x82\xaa\x8f\xac\x82\xb3\x82\xa2\x81" "B"
#define TEXT_NIGHTMARE_ENEMY "\x93G\x82\xcc\x8b\xad\x82\xb3s064\x81" "Fs075\x88\xab\x96\xb2\x83\x8c\x83x\x83\x8b"
#define TEXT_NIGHTMARE_OTHER "s004\x82\xbb\x82\xcc\x91\xbcs064\x81" "Fs075\x82" "f\x82\x81\x82\x8d\x82\x85\x82n\x82\x96\x82\x85\x82\x92\x8c\xe3\x82\xcc\x89\xf1\x95\x9c\x97\xca\x82\xaa\x8c\xb8\x8f\xad\x81" "B"
#define TEXT_NIGHTMARE_FALL "s075\x8a\x8a\x97\x8e\x82\xcc\x83_\x83\x81\x81[\x83W\x82\xaa\x94{\x91\x9d\x81" "B"
#define TEXT_GOD_ENEMY "\x93G\x82\xcc\x8b\xad\x82\xb3s064\x81" "Fs075\x90_\x83\x8c\x83x\x83\x8b"
#define TEXT_GOD_OTHER "s004\x82\xbb\x82\xcc\x91\xbcs064\x81" "Fs075\x82" "f\x82\x81\x82\x8d\x82\x85\x82n\x82\x96\x82\x85\x82\x92\x8c\xe3\x82\xcc\x89\xf1\x95\x9c\x97\xca\x82\xaa\x92\xb4\x8c\x83\x8c\xb8\x8f\xad\x81" "B"
#define TEXT_GOD_FALL "s075\x8a\x8a\x97\x8e\x8e\x9e\x82\xcc\x83_\x83\x81\x81[\x83W\x82\xaa\x82\xb3\x82\xe7\x82\xc9\x94{\x81X\x91\x9d\x81" "B"
#define TEXT_HELL_ENEMY "\x93G\x82\xcc\x8b\xad\x82\xb3s064\x81" "Fs075\x92n\x8d\x96\x83\x8c\x83x\x83\x8b"
#define TEXT_HELL_OTHER "s004\x82\xbb\x82\xcc\x91\xbcs064\x81" "Fs075\x82" "f\x82\x81\x82\x8d\x82\x85\x82n\x82\x96\x82\x85\x82\x92\x8c\xe3\x82\xcc\x89\xf1\x95\x9c\x97\xca\x82\xaa\x92\xb4\x8c\xb8\x8f\xad\x81" "B"
#define TEXT_HELL_FALL "s075\x8a\x8a\x97\x8e\x8e\x9e\x82\xcc\x83_\x83\x81\x81[\x83W\x82\xaa\x82\xb3\x82\xe7\x82\xc9\x94{\x91\x9d\x81" "B"
#define TEXT_BROKEN_MESSAGES "\x81H\x81H\x81Hs064\x81" "Fs075\x83\x81\x83" "b\x83Z\x81[\x83W\x82\xaa\x89\xf3\x82\xea\x82\xe9\x81" "B"

// Name box (state below NAME_ENTRY_CONFIRM, typing): it slides in with the entry tick.
#define SLIDE_X_PER_TICK 0xd4          // horizontal slide distance per tick, in tenths of a pixel
#define SLIDE_Y_SHIFT 8                // vertical slide: 256 tenths of a pixel per tick, upwards
#define SLIDE_TENTHS 10.0f
#define NAME_BOX_X 0x100
#define NAME_BOX_Y 0x174
#define NAME_BOX_WIDTH 0x80
#define NAME_BOX_HEIGHT 0x34
#define NAME_TEXT_X 0x114              // where the typed name starts inside the box
#define NAME_TEXT_Y 0x184
#define NAME_BOX_RIGHT 0x170           // right and bottom edges the IME text is clipped to
#define NAME_BOX_BOTTOM 0x1a8
#define CANDIDATE_BOX_Y 0x15c          // the candidate list grows upwards from here
#define CANDIDATE_TEXT_Y 0x16c
#define CANDIDATE_ROW_HEIGHT 16
#define CANDIDATE_BOX_PADDING_X 0x28
#define CANDIDATE_BOX_PADDING_Y 0x24
#define IME_LABEL_X 0x244
#define IME_LABEL_Y 0x1cc
#define IME_LABEL_SCALE 0.6f
#define IME_COLOUR 0xffffff
#define IME_SELECTED_COLOUR 0xff2020
#define IME_COMPOSING_COLOUR 0x2020ff

// The confirmed name (state 10): "name" label, name box and the difficulty description panel; both slide in with
// the tick.
#define CONFIRM_LABEL_PANEL_X 0xc0
#define CONFIRM_LABEL_PANEL_WIDTH 0x4c
#define CONFIRM_LABEL_X 0xd4
#define CONFIRM_NAME_PANEL_WIDTH 0x90
#define DESCRIPTION_PANEL_X 0x40
#define DESCRIPTION_PANEL_Y 0x154
#define DESCRIPTION_PANEL_WIDTH 0x208
#define DESCRIPTION_PANEL_HEIGHT 0x78
#define DESCRIPTION_X 0x60
#define DESCRIPTION_Y_FIRST 0x168
#define DESCRIPTION_Y_SECOND 0x17c
#define DESCRIPTION_Y_THIRD 0x190
#define DESCRIPTION_Y_FOURTH 0x1a4
#define DESCRIPTION_SCALE 0.9f
#define DESCRIPTION_COLOUR 0xffffff

// The summary after the name was confirmed (state above 10): fixed panels with the name and the difficulty.
#define SUMMARY_NAME_LABEL_PANEL_X 0x194
#define SUMMARY_NAME_LABEL_PANEL_Y 0x74
#define SUMMARY_NAME_LABEL_X 0x1a8
#define SUMMARY_NAME_LABEL_Y 0x84
#define SUMMARY_NAME_PANEL_X 0x1d4
#define SUMMARY_NAME_X 0x1e8
#define SUMMARY_LABEL_PANEL_WIDTH 0x4c
#define SUMMARY_NAME_PANEL_WIDTH 0x90
#define SUMMARY_DIFFICULTY_LABEL_PANEL_X 0x18c
#define SUMMARY_DIFFICULTY_LABEL_PANEL_Y 0x9c
#define SUMMARY_DIFFICULTY_LABEL_PANEL_WIDTH 0x5c
#define SUMMARY_DIFFICULTY_LABEL_X 0x1a0
#define SUMMARY_DIFFICULTY_LABEL_Y 0xac
#define SUMMARY_DIFFICULTY_PANEL_X 0x1dc
#define SUMMARY_DIFFICULTY_PANEL_WIDTH 0xa8
#define SUMMARY_DIFFICULTY_X 0x1f0

#define NAME_ENTRY_CONFIRM 10          // DAT_104bfd14 while the confirmed name is shown
#define PANEL_DEPTH 0.0f
#define PANEL_ALPHA 1.0f

// Difficulty pages the description shows (the cursor position of the difficulty list); each page has its own face
// picture (DAT_1042d9d8[page]).
#define DIFFICULTY_NORMAL 0
#define DIFFICULTY_PLAYFUL 1
#define DIFFICULTY_NIGHTMARE 2
#define DIFFICULTY_HELL 3
#define DIFFICULTY_GOD 4

// Difficulty settings the summary names: DAT_10c66cc0 (2 = crazy, 1 = pleasure) and DAT_10c635f8 (1 = beginner,
// 2 = hard, anything else normal).
#define SETTING_CRAZY 2
#define SETTING_PLEASURE 1
#define SETTING_BEGINNER 1
#define SETTING_HARD 2

// 0x10038520 (NameEntry scene routine). Draws the menu windows and marks, then, while the name is
// typed (state below 10) a name box slides in with the typed name, the IME composition and candidate list and the
// input mode label. In state 10 the "name" label and the name slide in above a panel that describes the difficulty
// under the cursor (or DAT_10c635f8) and puts that difficulty's face picture on the player model. Later states show
// the name and the chosen difficulty in two fixed rows. Then the name banners, the fade and the cursor report run.
// Returns 0.
extern "C" int FUN_10038520(void)
{
    DAT_104b7200.DrawWindows(DAT_104bf88c);
    DAT_104b7200.DrawWindowMarks(DAT_104bf88c);
    int slideX = (int)((float)(DAT_104c0232 * SLIDE_X_PER_TICK) / SLIDE_TENTHS);
    int slideY = (int)((float)(-DAT_104c0232 << SLIDE_Y_SHIFT) / SLIDE_TENTHS);
    if (DAT_104bfd14 < NAME_ENTRY_CONFIRM)
    {
        DAT_10db6ee8[0].M_10161a20(DAT_1042d9d8[DIFFICULTY_NORMAL]);
        DAT_104b7200.DrawPanelAlt(slideX + NAME_BOX_X, slideY + NAME_BOX_Y, NAME_BOX_WIDTH, NAME_BOX_HEIGHT,
                                  PANEL_DEPTH, PANEL_ALPHA);
        FUN_1015fbf0(&DAT_1020e538, slideX + NAME_TEXT_X, slideY + NAME_TEXT_Y, slideX + NAME_BOX_X,
                     slideY + NAME_BOX_Y, slideX + NAME_BOX_RIGHT, slideY + NAME_BOX_BOTTOM, IME_COLOUR,
                     IME_SELECTED_COLOUR, IME_COMPOSING_COLOUR);
        int candidates = FUN_1015fa90();
        if (candidates != 0)
        {
            int width = FUN_1015fac0(&DAT_1020e538);
            DAT_104b7200.DrawPanelAlt(slideX + NAME_BOX_X, slideY + CANDIDATE_BOX_Y - candidates * CANDIDATE_ROW_HEIGHT,
                                      width + CANDIDATE_BOX_PADDING_X,
                                      candidates * CANDIDATE_ROW_HEIGHT + CANDIDATE_BOX_PADDING_Y, PANEL_DEPTH,
                                      PANEL_ALPHA);
            FUN_1015fb20(&DAT_1020e538, slideX + NAME_TEXT_X,
                         slideY + CANDIDATE_TEXT_Y - candidates * CANDIDATE_ROW_HEIGHT, IME_COLOUR,
                         IME_SELECTED_COLOUR);
        }
        DAT_1020e538.DrawText(IME_LABEL_X, IME_LABEL_Y, DAT_10f66f00, IME_COLOUR, PANEL_DEPTH, PANEL_ALPHA,
                              IME_LABEL_SCALE);
    }
    else if (DAT_104bfd14 == NAME_ENTRY_CONFIRM)
    {
        DAT_104b7200.DrawPanelAlt(slideX + CONFIRM_LABEL_PANEL_X, slideY + NAME_BOX_Y, CONFIRM_LABEL_PANEL_WIDTH,
                                  NAME_BOX_HEIGHT, PANEL_DEPTH, (float)DAT_104c0232 / SLIDE_TENTHS);
        DAT_1020e538.DrawText(slideX + CONFIRM_LABEL_X, slideY + NAME_TEXT_Y, TEXT_NAME, IME_COLOUR, PANEL_DEPTH,
                              (float)DAT_104c0232 / SLIDE_TENTHS, 1.0f);
        DAT_104b7200.DrawPanelAlt(slideX + NAME_BOX_X, slideY + NAME_BOX_Y, CONFIRM_NAME_PANEL_WIDTH,
                                  NAME_BOX_HEIGHT, PANEL_DEPTH, PANEL_ALPHA);
        DAT_1020e538.DrawText(slideX + NAME_TEXT_X, slideY + NAME_TEXT_Y, (char *)DAT_10255a08, IME_COLOUR,
                              PANEL_DEPTH, PANEL_ALPHA, 1.0f);
        DAT_104b7200.DrawPanelAlt(DESCRIPTION_PANEL_X, DESCRIPTION_PANEL_Y, DESCRIPTION_PANEL_WIDTH,
                                  DESCRIPTION_PANEL_HEIGHT, PANEL_DEPTH, PANEL_ALPHA);
        int page = DAT_104b7200.m_0110[DAT_104b7200.m_0088] == 0 ? 0 : DAT_104b7200.m_0140[DAT_104b7200.m_0088];
        if (DAT_104bfd14 != NAME_ENTRY_CONFIRM)
            page = DAT_10c635f8;
        if (page == DIFFICULTY_NORMAL)
        {
            DAT_10db6ee8[0].M_10161a20(DAT_1042d9d8[DIFFICULTY_NORMAL]);
            DAT_1020e538.DrawText(DESCRIPTION_X, DESCRIPTION_Y_FIRST, TEXT_NORMAL_ENEMY, DESCRIPTION_COLOUR, PANEL_DEPTH, PANEL_ALPHA,
                                  DESCRIPTION_SCALE);
            DAT_1020e538.DrawText(DESCRIPTION_X, DESCRIPTION_Y_SECOND, TEXT_NORMAL_OTHER, DESCRIPTION_COLOUR, PANEL_DEPTH, PANEL_ALPHA,
                                  DESCRIPTION_SCALE);
        }
        else if (page == DIFFICULTY_PLAYFUL)
        {
            DAT_10db6ee8[0].M_10161a20(DAT_1042d9d8[DIFFICULTY_PLAYFUL]);
            DAT_1020e538.DrawText(DESCRIPTION_X, DESCRIPTION_Y_FIRST, TEXT_PLAYFUL_ENEMY, DESCRIPTION_COLOUR, PANEL_DEPTH, PANEL_ALPHA,
                                  DESCRIPTION_SCALE);
            DAT_1020e538.DrawText(DESCRIPTION_X, DESCRIPTION_Y_SECOND, TEXT_PLAYFUL_JUMP, DESCRIPTION_COLOUR, PANEL_DEPTH, PANEL_ALPHA,
                                  DESCRIPTION_SCALE);
            DAT_1020e538.DrawText(DESCRIPTION_X, DESCRIPTION_Y_THIRD, TEXT_PLAYFUL_HEAL, DESCRIPTION_COLOUR, PANEL_DEPTH, PANEL_ALPHA,
                                  DESCRIPTION_SCALE);
            DAT_1020e538.DrawText(DESCRIPTION_X, DESCRIPTION_Y_FOURTH, TEXT_PLAYFUL_TRAPS, DESCRIPTION_COLOUR, PANEL_DEPTH, PANEL_ALPHA,
                                  DESCRIPTION_SCALE);
        }
        else if (page == DIFFICULTY_NIGHTMARE)
        {
            DAT_10db6ee8[0].M_10161a20(DAT_1042d9d8[DIFFICULTY_NIGHTMARE]);
            DAT_1020e538.DrawText(DESCRIPTION_X, DESCRIPTION_Y_FIRST, TEXT_NIGHTMARE_ENEMY, DESCRIPTION_COLOUR, PANEL_DEPTH, PANEL_ALPHA,
                                  DESCRIPTION_SCALE);
            DAT_1020e538.DrawText(DESCRIPTION_X, DESCRIPTION_Y_SECOND, TEXT_NIGHTMARE_OTHER, DESCRIPTION_COLOUR, PANEL_DEPTH, PANEL_ALPHA,
                                  DESCRIPTION_SCALE);
            DAT_1020e538.DrawText(DESCRIPTION_X, DESCRIPTION_Y_THIRD, TEXT_NIGHTMARE_FALL, DESCRIPTION_COLOUR, PANEL_DEPTH, PANEL_ALPHA,
                                  DESCRIPTION_SCALE);
        }
        else if (page == DIFFICULTY_GOD)
        {
            DAT_10db6ee8[0].M_10161a20(DAT_1042d9d8[DIFFICULTY_GOD]);
            DAT_1020e538.DrawText(DESCRIPTION_X, DESCRIPTION_Y_FIRST, TEXT_GOD_ENEMY, DESCRIPTION_COLOUR, PANEL_DEPTH, PANEL_ALPHA,
                                  DESCRIPTION_SCALE);
            DAT_1020e538.DrawText(DESCRIPTION_X, DESCRIPTION_Y_SECOND, TEXT_GOD_OTHER, DESCRIPTION_COLOUR, PANEL_DEPTH, PANEL_ALPHA,
                                  DESCRIPTION_SCALE);
            DAT_1020e538.DrawText(DESCRIPTION_X, DESCRIPTION_Y_THIRD, TEXT_GOD_FALL, DESCRIPTION_COLOUR, PANEL_DEPTH, PANEL_ALPHA,
                                  DESCRIPTION_SCALE);
            DAT_1020e538.DrawText(DESCRIPTION_X, DESCRIPTION_Y_FOURTH, TEXT_BROKEN_MESSAGES, DESCRIPTION_COLOUR, PANEL_DEPTH, PANEL_ALPHA,
                                  DESCRIPTION_SCALE);
        }
        else
        {
            DAT_10db6ee8[0].M_10161a20(DAT_1042d9d8[DIFFICULTY_HELL]);
            DAT_1020e538.DrawText(DESCRIPTION_X, DESCRIPTION_Y_FIRST, TEXT_HELL_ENEMY, DESCRIPTION_COLOUR, PANEL_DEPTH, PANEL_ALPHA,
                                  DESCRIPTION_SCALE);
            DAT_1020e538.DrawText(DESCRIPTION_X, DESCRIPTION_Y_SECOND, TEXT_HELL_OTHER, DESCRIPTION_COLOUR, PANEL_DEPTH, PANEL_ALPHA,
                                  DESCRIPTION_SCALE);
            DAT_1020e538.DrawText(DESCRIPTION_X, DESCRIPTION_Y_THIRD, TEXT_HELL_FALL, DESCRIPTION_COLOUR, PANEL_DEPTH, PANEL_ALPHA,
                                  DESCRIPTION_SCALE);
            DAT_1020e538.DrawText(DESCRIPTION_X, DESCRIPTION_Y_FOURTH, TEXT_BROKEN_MESSAGES, DESCRIPTION_COLOUR, PANEL_DEPTH, PANEL_ALPHA,
                                  DESCRIPTION_SCALE);
        }
    }
    else
    {
        DAT_104b7200.DrawPanelAlt(SUMMARY_NAME_LABEL_PANEL_X, SUMMARY_NAME_LABEL_PANEL_Y, SUMMARY_LABEL_PANEL_WIDTH,
                                  NAME_BOX_HEIGHT, PANEL_DEPTH, PANEL_ALPHA);
        DAT_1020e538.DrawText(SUMMARY_NAME_LABEL_X, SUMMARY_NAME_LABEL_Y, TEXT_NAME, IME_COLOUR, PANEL_DEPTH,
                              PANEL_ALPHA, 1.0f);
        DAT_104b7200.DrawPanelAlt(SUMMARY_NAME_PANEL_X, SUMMARY_NAME_LABEL_PANEL_Y, SUMMARY_NAME_PANEL_WIDTH,
                                  NAME_BOX_HEIGHT, PANEL_DEPTH, PANEL_ALPHA);
        DAT_1020e538.DrawText(SUMMARY_NAME_X, SUMMARY_NAME_LABEL_Y, (char *)DAT_10255a08, IME_COLOUR, PANEL_DEPTH,
                              PANEL_ALPHA, 1.0f);
        DAT_104b7200.DrawPanelAlt(SUMMARY_DIFFICULTY_LABEL_PANEL_X, SUMMARY_DIFFICULTY_LABEL_PANEL_Y,
                                  SUMMARY_DIFFICULTY_LABEL_PANEL_WIDTH, NAME_BOX_HEIGHT, PANEL_DEPTH, PANEL_ALPHA);
        DAT_1020e538.DrawText(SUMMARY_DIFFICULTY_LABEL_X, SUMMARY_DIFFICULTY_LABEL_Y, TEXT_DIFFICULTY, IME_COLOUR,
                              PANEL_DEPTH, PANEL_ALPHA, 1.0f);
        DAT_104b7200.DrawPanelAlt(SUMMARY_DIFFICULTY_PANEL_X, SUMMARY_DIFFICULTY_LABEL_PANEL_Y,
                                  SUMMARY_DIFFICULTY_PANEL_WIDTH, NAME_BOX_HEIGHT, PANEL_DEPTH, PANEL_ALPHA);
        if (DAT_10c66cc0 == SETTING_CRAZY)
            DAT_1020e538.DrawText(SUMMARY_DIFFICULTY_X, SUMMARY_DIFFICULTY_LABEL_Y, TEXT_LABEL_CRAZY, IME_COLOUR, PANEL_DEPTH,
                                  PANEL_ALPHA, 1.0f);
        else if (DAT_10c66cc0 == SETTING_PLEASURE)
            DAT_1020e538.DrawText(SUMMARY_DIFFICULTY_X, SUMMARY_DIFFICULTY_LABEL_Y, TEXT_LABEL_PLEASURE, IME_COLOUR, PANEL_DEPTH,
                                  PANEL_ALPHA, 1.0f);
        else if (DAT_10c635f8 == SETTING_BEGINNER)
            DAT_1020e538.DrawText(SUMMARY_DIFFICULTY_X, SUMMARY_DIFFICULTY_LABEL_Y, TEXT_LABEL_BEGINNER, IME_COLOUR, PANEL_DEPTH,
                                  PANEL_ALPHA, 1.0f);
        else if (DAT_10c635f8 == SETTING_HARD)
            DAT_1020e538.DrawText(SUMMARY_DIFFICULTY_X, SUMMARY_DIFFICULTY_LABEL_Y, TEXT_LABEL_HARD, IME_COLOUR, PANEL_DEPTH,
                                  PANEL_ALPHA, 1.0f);
        else
            DAT_1020e538.DrawText(SUMMARY_DIFFICULTY_X, SUMMARY_DIFFICULTY_LABEL_Y, TEXT_LABEL_NORMAL, IME_COLOUR, PANEL_DEPTH,
                                  PANEL_ALPHA, 1.0f);
    }
    if (DAT_104bf8d4 != 0)
        FUN_1000cc30();
    FUN_1000dba0();
    DAT_1020d9e0.M_100014b0();
    return 0;
}
