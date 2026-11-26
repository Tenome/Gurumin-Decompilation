// Switching the camera to one of the map's camera paths (map camera data, see camera_reset.cpp): the path is looked
// up by its id and the camera starts at the path frame closest to the camera target, and collecting
// the paths from the scene.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vec4.h"

#include "matrix4.h"

#include "key_track.h"

#include "camera_path_select.h"

#include "camera_vars.h"

extern "C" int FUN_1015c9e0(int mode);               // Screen_StartTransition

// 0x1016b050 (Camera_SelectPath). Id -1 stops the path camera. Otherwise the path with that id becomes current (if
// the camera already runs on it nothing changes; switching paths starts transition 0xf), and the camera starts at
// the frame of the path closest to the camera target (distance with z counted twice). Returns 0.
extern "C" int FUN_1016b050(int id)
{
    if (id == -1)
        DAT_1101d890 = 0;
    else
    {
        int previous = DAT_1101d8a4;
        for (int i = 0; i < DAT_1101d89c; i++)
        {
            if (DAT_10f8d938[i] == id)
            {
                DAT_1101d8a4 = i;
                break;
            }
        }
        if (DAT_1101d890 != 0)
        {
            if (DAT_1101d8a4 == previous)
                return 0;
            FUN_1015c9e0(0xf);
        }
        int path = DAT_1101d8a4;
        float nearest = 0.0f;
        Vec4 point;
        Vec4 target;
        target.Copy(DAT_1101d884->world.GetTranslation());
        float length = (float)DAT_118e69f8[path].frames[DAT_118e69f8[path].count - 1];
        float bestFrame;
        int bestPath;
        int frame = 0;
        for (float f = 0.0f; f < length; f = (float)++frame)
        {
            DAT_118e69f8[path].Sample(frame, &point.x, &point.y, &point.z, &point.w, 0);
            float distance = point.DistanceZWeighted(target);
            if (nearest >= distance || frame == 0)
            {
                nearest = distance;
                bestFrame = f;
                bestPath = path;
            }
        }
        DAT_1101d8a0 = 1;
        DAT_1101d8a4 = bestPath;
        DAT_1101c800 = bestFrame;
        DAT_1101d890 = 1;
        DAT_10faa678 = 1;
    }
    return 0;
}

#include "node_data.h"

#include "node.h"


// 0x1016ac90 (Camera_CollectPaths). Adds every camera path below `node` (kind 2, name not ending in 'b') to the
// path tables at *count: the path id is the last character of its name (atol), the camera track comes from the
// node, the target track from "<name>.Target", and optional tracks from "<name>.Target2" and "<name>b" (all looked up
// under `root`). Returns 0.
extern "C" int FUN_1016ac90(Node *root, Node *node, int *count)
{
    if (node->pathKind == 2 && node->name[strlen(node->name) - 1] != 'b')
    {
        char name[0x100];
        sprintf(name, "%sb", node->name);
        Node *second = root->Find(name, 0);
        sprintf(name, "%s.Target", node->name);
        Node *target = root->Find(name, 0);
        sprintf(name, "%s.Target2", node->name);
        Node *target2 = root->Find(name, 0);
        if (target != 0 && node->extra != 0 && target->extra != 0)
        {
            DAT_11af3928[*count].CopyFrom(&node->extra->tracks[NODE_TRACK_TRANSLATION]);
            DAT_118e69f8[*count].CopyFrom(&target->extra->tracks[NODE_TRACK_TRANSLATION]);
            DAT_10f8d938[*count] = atol(&node->name[strlen(node->name) - 1]);
            DAT_11018740[*count] = 1;
            DAT_11018628[*count] = 0;
            DAT_10fdeb10[*count] = 0;
            DAT_10ff17e0[*count].m_000c = 0;
            if (target2 != 0)
            {
                DAT_11af3ba8[*count].CopyFrom(&target2->extra->tracks[NODE_TRACK_TRANSLATION]);
                DAT_11018628[*count] = 1;
            }
            if (second != 0)
            {
                DAT_118e96f8[*count].CopyFrom(&second->extra->tracks[NODE_TRACK_TRANSLATION]);
                DAT_10fdeb10[*count] = 1;
            }
            DAT_118e69f8[*count].MarkAll();
            DAT_10ff17e0[*count].m_0000 = node->m_187c;
            DAT_10ff17e0[*count].m_0004 = node->m_187d;
            DAT_10ff17e0[*count].m_0008 = 0;
            if ((target->flags & 0x10) != 0)
                DAT_10ff17e0[*count].m_000c = 1;
            if ((target->flags & 0x20) != 0)
                DAT_10ff17e0[*count].m_0008 = 1;
            if ((node->flags & 0x20) != 0)
                DAT_10ff17e0[*count].m_0008 = 1;
            if ((node->flags & 0x40) != 0)
                DAT_11018740[*count] = 0;
            (*count)++;
        }
    }
    for (int i = 0; i < node->childCount; i++)
        FUN_1016ac90(root, node->children[i], count);
    return 0;
}
