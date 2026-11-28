// The mouse cursor callbacks of the tracker (DAT_1020d9e0, Tracker::M_10001010): the tracker passes the cursor
// position in 640x480 game coordinates whenever the mouse moved. Both callbacks hide the system cursor over the
// game window, show it again outside, and draw the game's own cursor sprite (a pointer, or an arrow that turns
// towards the point the mouse walk starts from).

#include <math.h>
#include <windows.h>

#include "tracker.h"
#include "texture_object.h"
#include "math_float.h"

#include "char_vars.h"
#include "game_vars.h"
#include "map_vars.h"
#include "scene_draw_vars.h"
#include "ui_vars.h"
#include "win_vars.h"
#include "field_item_menu.h"

// Game mode (DAT_104bf8c0) in which the script scenes (FUN_1000fd50) may show the turning arrow.
#define MODE_SCRIPT_SCENE 1
// Game mode in which the field (FUN_100225b0) may show the turning arrow.
#define MODE_FIELD 0xf
// The arrow is not shown when x + y reaches this (the cursor is in the lower right corner).
#define ARROW_CORNER_LIMIT 0x410
// Field: DAT_102028dc (the map) between these (exclusive) hides the arrow.
#define NO_ARROW_MAP_FIRST 5
#define NO_ARROW_MAP_LAST 0x19
// The arrow sprite points up-left in the sheet: its angle is this much (225 degrees) minus the direction.
#define ARROW_ANGLE_OFFSET 3.926875f
// Size of the pointer sprite (0x20 wide, 0x24 high at an offset of 4 to the left) and of the arrow (0x20 square).
#define POINTER_SIZE 0x20
#define POINTER_HEIGHT 0x24
#define POINTER_SHIFT 4
#define ARROW_HALF_SIZE 0x10
// Cells of the cursor sheet (texture slot DAT_104b7208) in texture coordinates: the sheet has 8 x 8 cells.
#define CELL 0.125f

// 0x1000fd50. The cursor callback of the script scenes and the game start (see Tracker::M_10001010): shows the game
// cursor while the system cursor is over the window. The arrow shows in game mode 1 outside the corner, the overlay
// and the pauses, when the game built-ins are registered; it turns by the direction DAT_102027e8 / DAT_104c0234.
extern "C" int FUN_1000fd50(int x, int y)
{
    // The system cursor is hidden while it is over the client area of the game window and shown outside it.
    POINT cursor;
    RECT client;
    POINT topLeft;
    POINT bottomRight;
    GetCursorPos(&cursor);
    GetClientRect(DAT_10f6569c, &client);
    topLeft.x = client.left;
    topLeft.y = client.top;
    bottomRight.x = client.right;
    bottomRight.y = client.bottom;
    ClientToScreen(DAT_10f6569c, &topLeft);
    ClientToScreen(DAT_10f6569c, &bottomRight);
    if (DAT_1020d9e0.M_10001050(&cursor, topLeft.x, topLeft.y, bottomRight.x - topLeft.x, bottomRight.y - topLeft.y) != 0)
    {
        while (ShowCursor(0) >= 0)
            ;
        if (DAT_10202800 != 0 && DAT_104bfe6c == 0)
        {
            float angle = ARROW_ANGLE_OFFSET - (float)atan2((double)DAT_102027e8, (double)-DAT_104c0234);
            int arrow = 1;
            if (DAT_104bf8c0 == MODE_SCRIPT_SCENE)
            {
                if (x + y >= ARROW_CORNER_LIMIT)
                    arrow = 0;
                if (DAT_104bfe1c != 0)
                    arrow = 0;
                if (DAT_104c0135 != 0)
                    arrow = 0;
                if (DAT_104bfe78 != 0)
                    arrow = 0;
                if (DAT_104bfe6c != 0)
                    arrow = 0;
            }
            if (DAT_104bf8c0 != MODE_SCRIPT_SCENE || DAT_1032d69c == 0)
                arrow = 0;
            // The game cursor sprite: a plain 32 x 32 sprite with DAT_104c0204 set, else the arrow centred on the cursor and turned
            // by `angle`, or the pointer with its top-left corner near the cursor.
            if (DAT_104c0204 != 0)
            {
                DAT_118d0ba8[DAT_104b7208].DrawTinted(x, y, x + POINTER_SIZE, y + POINTER_SIZE, CELL, CELL * 2, CELL * 2, CELL * 3, 0,
                                                      1.0f, 0, 0);
            }
            else if (arrow != 0)
                DAT_118d0ba8[DAT_104b7208].DrawTinted(x - ARROW_HALF_SIZE, y - ARROW_HALF_SIZE, x + ARROW_HALF_SIZE,
                                                      y + ARROW_HALF_SIZE, CELL, CELL * 2, CELL * 2, CELL * 3, 0, 1.0f, angle, 0);
            else
                DAT_118d0ba8[DAT_104b7208].DrawTinted(x - POINTER_SHIFT, y, x + POINTER_SIZE, y + POINTER_HEIGHT, 0, CELL * 2, CELL,
                                                      CELL * 3, 0, 1.0f, 0, 0);
            return 1;
        }
    }
    else
    {
        while (ShowCursor(1) < 0)
            ;
    }
    return 1;
}

// 0x100225b0. The cursor callback of the field (see Tracker::M_10001010): like FUN_1000fd50, but the arrow turns
// towards the player's position on the screen (DAT_1020c318/DAT_1020c31c), where the mouse walk starts, shows in game
// mode 0xf, and is hidden on some maps (DAT_102028dc 6..24) and while DAT_104c03dc is set. Nothing is drawn while
// the map movie is hidden (DAT_10c66d08).
extern "C" int FUN_100225b0(int x, int y)
{
    POINT cursor;
    RECT client;
    POINT topLeft;
    POINT bottomRight;
    GetCursorPos(&cursor);
    GetClientRect(DAT_10f6569c, &client);
    topLeft.x = client.left;
    topLeft.y = client.top;
    bottomRight.x = client.right;
    bottomRight.y = client.bottom;
    ClientToScreen(DAT_10f6569c, &topLeft);
    ClientToScreen(DAT_10f6569c, &bottomRight);
    if (DAT_1020d9e0.M_10001050(&cursor, topLeft.x, topLeft.y, bottomRight.x - topLeft.x, bottomRight.y - topLeft.y) != 0)
    {
        while (ShowCursor(0) >= 0)
            ;
        if (DAT_10c66d08 != 0 || DAT_10202800 == 0 || DAT_104bfe6c != 0)
            return 1;
        float angle = ARROW_ANGLE_OFFSET - FUN_10001f80((float)(x - DAT_1020c318), (float)(y - DAT_1020c31c));
        int arrow = 1;
        if (DAT_104bf8c0 == MODE_FIELD)
        {
            if (x + y >= ARROW_CORNER_LIMIT)
                arrow = 0;
            if (DAT_104bfe1c != 0)
                arrow = 0;
            if (DAT_104c0135 != 0)
                arrow = 0;
            if (DAT_104bfe78 != 0)
                arrow = 0;
            if (DAT_104bfe6c != 0)
                arrow = 0;
            if (DAT_1032d69c == 0)
                arrow = 0;
            if (DAT_102028dc < NO_ARROW_MAP_LAST && DAT_102028dc > NO_ARROW_MAP_FIRST)
                arrow = 0;
        }
        if (DAT_104bf8c0 != MODE_FIELD || DAT_104c03dc != 0)
            arrow = 0;
        // The game cursor sprite: a plain 32 x 32 sprite with DAT_104c0204 set, else the arrow centred on the cursor and turned
        // by `angle`, or the pointer with its top-left corner near the cursor.
        if (DAT_104c0204 != 0)
        {
            DAT_118d0ba8[DAT_104b7208].DrawTinted(x, y, x + POINTER_SIZE, y + POINTER_SIZE, CELL, CELL * 2, CELL * 2, CELL * 3, 0,
                                                  1.0f, 0, 0);
            return 1;
        }
        if (arrow != 0)
            DAT_118d0ba8[DAT_104b7208].DrawTinted(x - ARROW_HALF_SIZE, y - ARROW_HALF_SIZE, x + ARROW_HALF_SIZE,
                                                  y + ARROW_HALF_SIZE, CELL, CELL * 2, CELL * 2, CELL * 3, 0, 1.0f, angle, 0);
        else
            DAT_118d0ba8[DAT_104b7208].DrawTinted(x - POINTER_SHIFT, y, x + POINTER_SIZE, y + POINTER_HEIGHT, 0, CELL * 2, CELL,
                                                  CELL * 3, 0, 1.0f, 0, 0);
        return 1;
    }
    while (ShowCursor(1) < 0)
        ;
    return 1;
}
