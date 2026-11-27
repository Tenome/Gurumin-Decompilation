// Releasing the per-map data when a map is left or the game is reset (called by Game_Reset, Game_Startup and the
// map loader 0x10050460): the map's object records, models and textures are freed and every per-map table is
// cleared.

#include "last_position.h"
#include <string.h>

#include "vec4.h"

#include "matrix4.h"

#include "node.h"
#include "ui_vars.h"

// A map object record, 0x44 bytes: three blocks of its own.
#include "char_state.h"

#include "engine_memory.h"
#include "region_view.h"
#include "camera_vars.h"
#include "char_tables.h"
#include "map_vars.h"
#include "sound_vars.h"

#include "char_vars.h"

extern "C" int FUN_10174c70(int slot, int force);    // Texture_Release

// 0x10023d30 (Map_Unload). Frees the map object records, the held models and their data, resets the per-map
// matrices and vectors to identity and zero, clears the per-map tables, the character states and last positions,
// and releases the two map textures.
extern "C" void FUN_10023d30(void)
{
    int i;
    if (DAT_104c03c0 != 0)
    {
        for (i = 0; i < DAT_102564a0; i++)
        {
            DAT_11b1ba40(DAT_104c03c0[i].vertices);
            DAT_11b1ba40(DAT_104c03c0[i].points);
            DAT_11b1ba40(DAT_104c03c0[i].buffer0004);
            DAT_104c03c0[i].vertices = 0;
            DAT_104c03c0[i].points = 0;
            DAT_104c03c0[i].buffer0004 = 0;
        }
        DAT_11b1ba40(DAT_104c03c0);
        DAT_104c03c0 = 0;
    }
    if (DAT_104c0394 != 0)
    {
        DAT_11b1ba40(DAT_104c0394);
        DAT_104c0394 = 0;
    }
    if (DAT_104c0390 != 0)
    {
        DAT_104c0390->Release();
        DAT_11b1ba40(DAT_104c0390);
        DAT_104c0390 = 0;
    }
    if (DAT_104c039c != 0)
    {
        DAT_104c039c->Release();
        DAT_11b1ba40(DAT_104c039c);
        DAT_104c039c = 0;
        DAT_11b1ba40(DAT_104c03a0);
        DAT_104c03a0 = 0;
        DAT_104c03a8->Release();
        DAT_11b1ba40(DAT_104c03a8);
        DAT_104c03a8 = 0;
        DAT_104c03ac = 0;
    }
    memset(DAT_103086b0, 0, sizeof(DAT_103086b0));
    memset(DAT_1030b8f8, 0, sizeof(DAT_1030b8f8));
    memset(DAT_104483e8, 0, sizeof(DAT_104483e8));
    memset(DAT_10254598, 0, sizeof(DAT_10254598));
    DAT_104c0350 = 0;
    memset(DAT_10255160, 0, sizeof(DAT_10255160));
    for (i = 0; i < 0x80; i++)
    {
        for (int j = 0; j < 6; j++)
            DAT_10868838[i][j].SetIdentity();
        DAT_104c4438[i].Fill(0.0f);
        DAT_10453008[i] = 0;
        memset(DAT_1021bdf8[i], 0, sizeof(DAT_1021bdf8[i]));
    }
    for (i = 0; i < 8; i++)
        DAT_10edbd58[i].Fill(0.0f);
    DAT_104c0354 = 0;
    DAT_10866d98.Fill(0.0f);
    DAT_10edbd48.Fill(0.0f);
    DAT_104d29d8.Fill(0.0f);
    DAT_10f08aa0.Fill(0.0f);
    memset(DAT_1049efd8, 0, sizeof(DAT_1049efd8));
    for (i = 0; i < 3; i++)
    {
        for (int j = 0; j < 12; j++)
        {
            DAT_10255960[i].slots[j] = 0;
            if (j == 9)
                DAT_10255960[i].picture = -1;
        }
    }
    memset(DAT_103088d0, 0, sizeof(DAT_103088d0));
    DAT_1020da50[0][0] = 0;
    DAT_1020da50[1][0] = 0;
    DAT_1020da50[2][0] = 0;
    DAT_1020da50[3][0] = 0;
    DAT_1020da50[4][0] = 0;
    DAT_1020da50[5][0] = 0;
    DAT_1020da50[6][0] = 0;
    DAT_1020da50[7][0] = 0;
    DAT_102028d4 = -1;
    DAT_104c038c = 0;
    for (i = 0; i < 6; i++)
    {
        DAT_10f08880[i].SetIdentity();
        DAT_102108d0[i] = 0;
    }
    memset(DAT_10447310, 0, sizeof(DAT_10447310));
    memset(DAT_1021a398, 0, sizeof(DAT_1021a398));
    for (i = 0; i < 0x80; i++)
    {
        DAT_1020e138[i] = 0;
        DAT_10253a58[i] = 0;
        DAT_1042dba8[i] = 0;
        DAT_1020d540[i] = 0;
        DAT_10867e18[i].Fill(0.0f);
        DAT_1021b4c8[i] = 0;
        DAT_10256420[i] = 0;
        DAT_10254390[i] = 0;
        DAT_1021f940[i] = 0;
    }
    memset(DAT_1021bbf8, 0, sizeof(DAT_1021bbf8));
    memset(DAT_1042dea8, 0, sizeof(DAT_1042dea8));
    memset(DAT_1021b728, 0, sizeof(DAT_1021b728));
    memset(DAT_10255f10, 0, sizeof(DAT_10255f10));
    DAT_104c0390 = 0;
    DAT_104c039c = 0;
    DAT_104c03a0 = 0;
    DAT_104c03a4 = 0;
    DAT_104c03a8 = 0;
    DAT_104c03ac = 0;
    DAT_102028dc = 1;
    DAT_104c03b8 = 0;
    DAT_104c03bc = 0;
    memset(DAT_102199b8, 0, sizeof(DAT_102199b8));
    for (i = 0; i < 0x80; i++)
    {
        memset(&DAT_1020b430[i], 0, sizeof(DAT_1020b430[i]));
        // The position and the last step of the slot, as the first two of its four vectors.
        for (int vectorIndex = 0; vectorIndex < 2; vectorIndex++)
            LastPositionVector(i * 4 + vectorIndex).Fill(0.0f);
    }
    if (DAT_102028c8 > -1)
    {
        FUN_10174c70(DAT_102028c8, 1);
        DAT_102028c8 = -1;
    }
    if (DAT_102028cc > -1)
    {
        FUN_10174c70(DAT_102028cc, 1);
        DAT_102028cc = -1;
    }
    memset(DAT_1021d018, 0, sizeof(DAT_1021d018));
    memset(DAT_10446e00, 0, sizeof(DAT_10446e00));
}
