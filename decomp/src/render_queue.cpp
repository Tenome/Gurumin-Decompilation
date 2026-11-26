// Queue of nodes to draw this frame (up to 0x40 entries). Each entry stores the node, flags, two vectors,
// the current light colour and optional 0x80-byte extra data and matrix (each pool holds up to 0x30).

#include "vec4.h"

#include "matrix4.h"


struct Node;

#include "render_vars.h"

#include "engine_memory.h"

// RenderQueue_Push: appends `node` with its parameters. The extra data and matrix are copied into their pools
// and the entry stores their index (0xff = none). Full queues keep overwriting the last entry.
extern "C" int FUN_10199880(Node *node, const void *extra, const Matrix4 *matrix, const Vec4 *a, const Vec4 *b, char flags)
{
    DAT_11b00070[DAT_11b044d0] = node;
    char option = (char)DAT_10f65520;
    DAT_11b00030[DAT_11b044d0] = flags;
    DAT_11b00270[DAT_11b044d0] = option;
    DAT_11af3f68[DAT_11b044d0] = (char)0xff;
    DAT_11afffa8[DAT_11b044d0] = (char)0xff;
    DAT_11b04518[DAT_11b044d0].Copy(*a);
    DAT_11b04918[DAT_11b044d0].Copy(*b);
    DAT_11b07128[DAT_11b044d0].Copy(DAT_10f66948);
    DAT_11b00170[DAT_11b044d0] = DAT_10f655c4;
    if (extra != 0)
    {
        DAT_11b07530(DAT_11b04d18[DAT_11b044dc], extra, sizeof(DAT_11b04d18[0]));
        DAT_11afffa8[DAT_11b044d0] = (char)DAT_11b044dc;
        if (DAT_11b044dc < 0x2f)
            DAT_11b044dc++;
    }
    if (matrix != 0)
    {
        DAT_11b06518[DAT_11b002b8] = *matrix;
        DAT_11af3f68[DAT_11b044d0] = (char)DAT_11b002b8;
        if (DAT_11b002b8 < 0x2f)
            DAT_11b002b8++;
    }
    if (DAT_11b044d0 < 0x3f)
        DAT_11b044d0++;
    return 0;
}
