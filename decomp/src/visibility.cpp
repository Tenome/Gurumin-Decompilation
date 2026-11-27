// Visibility cells. A map is split into up to 64 boxes; each cell lists the cells visible from it, and only
// the visible cells are drawn. DAT_11afffe8[i] = cell i visible this frame.

#include <string.h>
#include "world.h"

#include "vec4.h"

#include "visibility.h"


#include "node.h"
#include "map_vars.h"
#include "anim_model.h"
#include "char_state.h"

#include "camera_vars.h"
#include "char_tables.h"
#include "render_vars.h"


// Finds the cell boxes that contain the eye position and marks them and all cells listed as visible from them.
// Not byte-identical yet (see open.tsv).
int Node::UpdateVisibleCells()
{
    if (visibilityCells != 0)
    {
        Vec4 eye;
        Box2 *box = visibilityCells->boxes;
        if (DAT_10f65520 != 0)
            eye.Copy(DAT_10f66908);
        else
            eye.Copy(DAT_10f668e8);
        memset(DAT_11afffe8, 0, sizeof(DAT_11afffe8));
        for (int i = 0; i < 0x40; i++, box++)
        {
            if (visibilityCells->cells[i].count != 0 && eye.InBox(box))
            {
                DAT_11afffe8[i] = 1;
                for (int j = 0; j < visibilityCells->cells[i].count; j++)
                    DAT_11afffe8[visibilityCells->cells[i].neighbours[j]] = 1;
            }
        }
        return 1;
    }
    return 0;
}

// 1 when `pos` lies in a currently visible cell (or the map has no visibility data), else 0. Used to skip
// updating or drawing objects that cannot be seen.
extern "C" int FUN_10015860(Vec4 pos)
{
    VisData *visibilityCells = DAT_10255f08->visibilityCells;
    if (visibilityCells == 0)
        return 1;
    Box2 *box = visibilityCells->boxes;
    for (int i = 0; i < 0x40; i++, box++)
    {
        if (DAT_11afffe8[i] != 0 && pos.InBox(box))
            return 1;
    }
    return 0;
}

// Characters (world.h): matrix and nearCells are used here.

// 0x10015710 (Vis_IsCharacterVisible). 1 when character `index` stands in a currently visible cell (or the map has no visibility data), else 0.
extern "C" int FUN_10015710(int index)
{
    Vec4 pos;
    if (DAT_10255f08->visibilityCells == 0)
        return 1;
    pos.Copy(DAT_10db6ee8[index].matrix.GetTranslation());
    Box2 *boxes = DAT_10255f08->visibilityCells->boxes;
    for (int i = 0; i < 0x40; i++)
    {
        if (DAT_11afffe8[i] != 0 && pos.InBox(&boxes[i]))
            return 1;
    }
    return 0;
}

// Not byte-identical yet: only the schedule of the row pointer differs (the original adds the table base with a lea
// after copying the box pointer).
// Marks the cell boxes within 75 units of character `index` in its row of DAT_1021d338 and points the character's
// nearCells at that row. Returns 1 when the map has no visibility data (nearCells stays 0), else 0.
extern "C" int FUN_100157b0(int index)
{
    Vec4 pos;
    int i = 0;
    DAT_10db6ee8[index].nearCells = 0;
    if (DAT_10255f08->visibilityCells == 0)
        return 1;
    pos.Copy(DAT_10db6ee8[index].matrix.GetTranslation());
    Box2 *boxes = DAT_10255f08->visibilityCells->boxes;
    char *row = DAT_1021d338[index];
    for (; i < 0x40; i++)
    {
        row[i] = 0;
        if (pos.InBoxMargin(&boxes[i], 75.0f) != 0)
            row[i] = 1;
    }
    DAT_10db6ee8[index].nearCells = row;
    return 0;
}
