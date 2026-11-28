// Globals of menus, text, input and the HUD panels.
// Generated from the per-file extern declarations by tools/globals_driver.py; this is the one
// declaration of each of them: include this header, never redeclare one. Comments say what is known.
#ifndef GURUMIN_UI_VARS_H
#define GURUMIN_UI_VARS_H

#include "item_caption.h"
#include "matrix4.h"
#include "menu_text.h"

struct InputEffect;   // dinput_min.h
struct Node;   // node.h

extern "C" char DAT_101d6530[7][7];                         // weekday names, Sunday first (full-width, 6 bytes each)
extern "C" short DAT_101d6578[27];                         // script flag of each map icon of the save summary
extern "C" char DAT_101d65b0[][0x10];                     // icon file names
extern "C" const char DAT_101fc7e4[];                     // c_dfDIMouse
extern "C" const char DAT_101fc9ec[];                     // c_dfDIKeyboard
extern "C" const char DAT_101fcbf4[];                     // c_dfDIJoystick
extern "C" int DAT_10202478;                              // texture slot the movies are decoded into; the movie texture slot
extern "C" int DAT_102024a0;
extern "C" char DAT_102028e8[0x22][0x10];                 // the page textures
extern "C" char DAT_10202a48[];                           // "dpartsbf.bmp": a locked page
extern "C" int DAT_10202b20;                              // 1 = draw text outlines
extern "C" int DAT_10203408;                              // dead zone around the axis centre (in 1/256 units)
extern "C" int DAT_10203428;                              // 1 = text input allowed
extern "C" int DAT_1020b228;                              // texture slot of the map icon, negative = none
extern "C" int DAT_10c6359c;                              // where the map movie is shown: 0 centre, 1 below centre, 2 at (DAT_10c635a0, DAT_10c635a4), 3 the same but shifted by a quarter of the movie size
extern "C" int DAT_10c635a0;                              // movie position x for placements 2 and 3
extern "C" int DAT_10c635a4;                              // movie position y for placements 2 and 3
extern "C" int DAT_10c66d08;                              // non-zero = the map movie is hidden
extern "C" int DAT_1020c2c0;                              // the chosen camera point; current map table entry
extern "C" int DAT_1020c2d0;                              // the movie playing
extern "C" int DAT_1020c330;                              // panel state, 0 = hidden; the text shows in states 8..22
extern "C" char DAT_1020da50[8][0x50];                    // the status panel's text lines
extern "C" char DAT_1020daa0[];
extern "C" char DAT_1020db40[];
extern "C" char DAT_1020db90[];
extern "C" char DAT_1020dbe0[];
extern "C" char DAT_1020dc80;                             // when 4 or more, the last line is yellow
extern "C" char DAT_10210ae8[];                           // first banner text (empty = none)
extern "C" int DAT_10216dc4;                              // number of map table entries; number of camera points
extern "C" int DAT_102194c0;                              // the left page shown
extern "C" int DAT_102194c4;                              // the right page shown
extern "C" int DAT_102239c8;                              // frames per animation step
extern "C" int DAT_1030b870[0x22];                        // its texture slots (-1 = none); texture slots of the pages
extern "C" int DAT_1030c908[0x22];                        // script flag of each page (0 = no page)
extern "C" int DAT_1042e2a8[0x40];
extern "C" int DAT_104483dc;                              // page of the left texture
extern "C" int DAT_104483e0;                              // page of the right texture
extern "C" int DAT_104b721c[0x19];                        // Game texture slots from texBalloon5 (DAT_104b7200 + 0x1c)
extern "C" int DAT_104bf88c;                              // the prompt's size in pixels
extern "C" int DAT_104bf8c0;
extern "C" int DAT_104bfd10;
extern "C" int DAT_104bff00;
extern "C" int DAT_104bff0c;
extern "C" unsigned int DAT_104c0230;                       // costume unlock bits (bit n-1 = costume n for 1-5, 0x20 = 10, 0x40 = 8, 0x80 = 9, 0x100 = 11, 0x200 = 12)
extern "C" unsigned char DAT_104c02c4;                    // selects which of the two outline textures DAT_10836100 holds (odd/even)
extern "C" unsigned char DAT_104c02c5;                    // character slot to draw an outline around (0 = none)
extern "C" unsigned char DAT_104c02c6;                    // 0 = outline added to the scene, otherwise darkened into it
extern "C" unsigned char DAT_104c02c7;                    // opacity of the outline in percent
extern "C" unsigned char DAT_104c02cc;                    // caption scroll timer
extern "C" unsigned char DAT_104c02cd;                    // how far the ability list can scroll
extern "C" char DAT_104c0339;
extern "C" Node *DAT_104c03cc;                            // the turning sheet
extern "C" Node *DAT_104c03d0;                            // the right half
extern "C" Node *DAT_104c03d4;                            // the left half
extern "C" Node *DAT_104c03d8;                            // the book cover
extern "C" int DAT_104c03e0;
extern "C" int DAT_104c03e4;                              // 1 while the pages turn
extern "C" int DAT_1020b444;                              // cleared when a book opens
extern "C" int DAT_10c6504c;                              // the left page last shown in the first book
extern "C" int DAT_10c65050;                              // the left page last shown in the second book
extern "C" int DAT_104c03e8;                              // non-zero in the second book
extern "C" char DAT_1065cb30;                             // a key of the current input frame (DemoRecording::buttons_now[4])
extern "C" char DAT_1065cb31;                             // a key of the current input frame (DemoRecording::buttons_now[5]); closes the tutorial book
extern "C" char DAT_1065cb3e;                             // a key of the current input frame (DemoRecording::buttons_now[0x12])
extern "C" char DAT_1065cb3f;                             // a key of the current input frame (DemoRecording::buttons_now[0x13])
extern "C" int DAT_1065cb48;                              // first analogue value of the input frame (DemoRecording::values_now[0]): left/right
extern "C" int DAT_1065cb4c;                              // second analogue value of the input frame (DemoRecording::values_now[1]): up/down
extern "C" int DAT_1065cb90;                              // non-zero = the mouse is not read in the tutorial book
extern "C" Matrix4 DAT_1086df38;                          // the player's world matrix; viewer matrix
extern "C" char DAT_10875e48[0x40][0x20];                 // movie names
extern "C" ItemName DAT_1088e670[];
extern "C" ItemText DAT_108f667c[];
extern "C" int DAT_10c64a5c;
extern "C" int DAT_10f1bef0[];                            // per game button: what it is mapped to (0 = default)
extern "C" int DAT_10f60c30[];                            // per game button: the key it is mapped to (0 = default)
extern "C" int DAT_10f654a4;                              // non-zero = 32-bit colour mode (else R5G6B5); 1 = 32-bit textures
extern "C" int DAT_10f657b8;                              // saved by the menu windows next to the wheel value
extern "C" int DAT_10f6555c;                              // mouse wheel movement accumulated
extern "C" int DAT_10f65560;                              // joystick sensitivity: the axis range is 256 minus this
extern "C" int DAT_10f65564;                              // non-zero when force feedback is enabled
extern "C" int DAT_10f65568;                              // the joystick has force feedback
extern "C" unsigned char DAT_10f6556c;                    // axes present: 1 = z, 2 = rz, 4 = rx, 8 = ry
extern "C" int DAT_10f65570;                              // joystick enabled
extern "C" int DAT_10f65574;                              // joystick disabled by the settings
extern "C" InputEffect *DAT_10f65578;                     // the force-feedback effect (0 = none)
extern "C" int DAT_10f66998;
extern "C" int DAT_10f6699c;
extern "C" int DAT_10f669a0;
extern "C" int DAT_10f66aa8;
extern "C" int DAT_10f66aac;
extern "C" int DAT_10f66ab0;
extern "C" char DAT_10f66ab8[];
extern "C" char DAT_10f66f00[];                           // the IME mode label
extern "C" int DAT_10f66bbc;
extern "C" int DAT_10f66bc0;
extern "C" int DAT_10f66bc4;
extern "C" int DAT_10f66bc8;                              // 1 when the IME conversion mode was changed and must be restored
extern "C" char DAT_10f66bd0[];                           // composition string
extern "C" char DAT_10f66cd0[];                           // result string
extern "C" int DAT_10f66dd0;
extern "C" int DAT_10f66dd4;
extern "C" char DAT_10f669a8[];                           // the result string of the IME (appended to the text entered)
extern "C" int DAT_10f66ddc;
extern "C" int DAT_10f66de0;
extern "C" int DAT_10f66de4;
extern "C" int DAT_10f66dec;
extern "C" int DAT_10f66de8;
extern "C" int DAT_10f66df0;                              // 1 once input has finished
extern "C" char DAT_10f66df8[];                           // the text entered so far
extern "C" char *DAT_10f66ef8;                            // caller's buffer that receives the entered text (may be 0)
extern "C" int DAT_10f67000;                              // 1 while text input (IME composition) is active
extern "C" MenuText DAT_10f67008[32];                     // IME candidate strings (one line each)
extern "C" char DAT_10f68008[];
extern "C" int DAT_10f68108;
extern "C" int DAT_10f6810c;
extern "C" int DAT_10f68110;
extern "C" int DAT_10f68114;                              // index of the selected candidate
extern "C" char *DAT_10f68118;                            // buffer that receives the text (see FUN_1015fd30)
extern "C" int DAT_10f6811c;

#endif
