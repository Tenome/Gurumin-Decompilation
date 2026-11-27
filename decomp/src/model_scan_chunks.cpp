// Model files (.it3): the chunk pre-pass that loads the animation of the nodes a model file names.

#include <stdio.h>
#include <string.h>

#include "game_file.h"

#include "node.h"
#include "node_data.h"
#include "node_channel.h"
#include "texture_ref.h"
#include "engine_memory.h"
#include "render_vars.h"

extern "C" int FUN_10169ab0(const char *folder, const char *name, int *freeSlot);  // motion name slot, -1 = none
extern "C" int FUN_101699d0(void);                   // a free motion name slot
extern "C" int FUN_101699f0(int slot, const char *folder, const char *name);       // names a motion slot
extern "C" int FUN_10169be0(TextureRef *texture);    // adds a reference

#define SCAN_MAX_NODES 1024
#define SCAN_NAME_SIZE 256
#define SCAN_PLUG_COUNT 16
#define SCAN_LAST_POSE 15

// 0x101a4a60 (It3_ScanChunks). Walks the chunks of an open model file and keeps the animation of the nodes it names:
// INFO names a node of this tree (only bones, bip, camera, charbase and the scene root are looked up) and drops that
// node's old plugs and frame matrices; PLUG and PLU2 (packed) give its 16 plugs; MANI and MAN2 (packed) give its frame
// matrices, shared through the motion cache unless `arg` is non-zero (then a new cache slot is named); any other
// chunk is skipped. Afterwards the textures of every node found are released with M_1019bca0. Returns 1.
int Node::ScanChunks(GameFile *file, int arg)
{
    Node *nodes[SCAN_MAX_NODES];
    char tag[8];
    char name[SCAN_NAME_SIZE];
    Node *node = 0;
    int chunkSize;
    int slotMani;
    int slotMan2;
    int unused1, unused2, unused3;
    int g;
    if (DAT_10f65538 != 0)
        DAT_10f65538(0, 0);
    int nodeCount = 0;
    Matrix4 current;
    int sawMotion = 0;
    Matrix4 fileMatrix;
    file->Read(tag, 1, 4);
    while (file->AtEnd() == 0)
    {
        if (strncmp(tag, "INFO", 4) == 0)
        {
            if (node != 0 && sawMotion == 0 && node->frameMatrixCount > 0)
            {
                node->frameMatrixCount = 0;
                DAT_11b1ba40(node->frameMatrices);
                node->frameMatrices = 0;
                node->matrix = fileMatrix;
            }
            sawMotion = 0;
            if (node != 0 && node->frameMatrixCount > 0)
                node->poses[SCAN_LAST_POSE] = current;
            file->Read(&chunkSize, 1, 4);
            file->Read(name, 1, 0x40);
            node = 0;
            if (_strnicmp(name, "bone", 4) == 0 || _strnicmp(name, "bip", 3) == 0 ||
                _strnicmp(name, "camera", 6) == 0 || _strnicmp(name, "charbase", 8) == 0 ||
                _strnicmp(name, "\xbc\xb0\xdd \xd9\xb0\xc4", 7) == 0)
                node = Find(name, 0);
            nodes[nodeCount++] = node;
            if (node != 0)
            {
                strcpy(node->m_0050, file->name);
                for (g = 0; g < SCAN_PLUG_COUNT; g++)
                {
                    if (node->plugs[g] != 0)
                        DAT_11b1ba40(node->plugs[g]);
                    node->plugs[g] = 0;
                    node->plugType[g] = 0;
                }
                if (node->frame < 0)
                {
                    int pose = -1 - node->frame;
                    if (pose >= SCAN_LAST_POSE)
                        pose = SCAN_LAST_POSE;
                    current = node->poses[pose];
                }
                else if ((node->flags & NODE_FLAG_ANIMATED_BY_EXTRA) != 0 && node->extra != 0)
                    current = node->extra->AnimatedMatrix(node->frame);
                else if (node->frameMatrices != 0)
                {
                    int index = node->frame;
                    if (node->frameMatrixCount <= index)
                        index = node->frameMatrixCount - 1;
                    current = node->frameMatrices[index];
                }
                else
                    current = node->matrix;
            }
            file->Read(&fileMatrix, 1, 0x40);
            file->Read(&unused1, 1, 4);
            file->Read(&unused2, 1, 4);
            file->Read(&unused3, 1, 4);
        }
        else if (strncmp(tag, "PLUG", 4) == 0 && node != 0)
        {
            file->Read(&chunkSize, 1, 4);
            for (g = 0; g < SCAN_PLUG_COUNT; g++)
            {
                int plugCount;
                file->Read(&plugCount, 1, 4);
                if (plugCount != 0)
                {
                    if (node->plugs[g] != 0)
                        DAT_11b1ba40(node->plugs[g]);
                    node->plugs[g] = 0;
                    node->plugType[g] = 0;
                    int plugSize;
                    file->Read(&plugSize, 1, 4);
                    Channel *data = (Channel *)DAT_11b0b938(plugSize, "c->m_plugdata[g]");
                    node->plugs[g] = data;
                    file->Read(data, 1, plugSize);
                    file->Read(&node->plugType[g], 1, 4);
                }
                else
                {
                    if (node->plugs[g] != 0)
                        DAT_11b1ba40(node->plugs[g]);
                    node->plugs[g] = 0;
                    node->plugType[g] = 0;
                }
            }
        }
        else if (strncmp(tag, "PLU2", 4) == 0 && node != 0)
        {
            file->Read(&chunkSize, 1, 4);
            for (g = 0; g < SCAN_PLUG_COUNT; g++)
            {
                int plugCount2;
                file->Read(&plugCount2, 1, 4);
                if (plugCount2 != 0)
                {
                    if (node->plugs[g] != 0)
                        DAT_11b1ba40(node->plugs[g]);
                    node->plugs[g] = 0;
                    int plugSize2;
                    file->Read(&plugSize2, 1, 4);
                    Channel *data = (Channel *)DAT_11b0b938(plugSize2, "c->m_plugdata[g]");
                    node->plugs[g] = data;
                    file->ReadPacked(data, 1, plugSize2);
                    file->Read(&node->plugType[g], 1, 4);
                }
                else
                {
                    if (node->plugs[g] != 0)
                        DAT_11b1ba40(node->plugs[g]);
                    node->plugs[g] = 0;
                    node->plugType[g] = 0;
                }
            }
        }
        else if (strncmp(tag, "MANI", 4) == 0 && node != 0)
        {
            sawMotion = 1;
            if (node->m_028c != 0)
                node->M_10191210();
            slotMani = -1;
            int found = -1;
            if (arg == 0)
            {
                found = FUN_10169ab0(node->m_0050, node->name, &slotMani);
                if (found >= 0)
                {
                    node->m_028c = DAT_10fdf1d0[found];
                    node->frameMatrices = DAT_1101c820[found];
                    node->frameMatrixCount = DAT_11017600[found];
                    FUN_10169be0(node->m_028c);
                    file->Read(&chunkSize, 1, 4);
                    file->Seek(chunkSize, 1);
                }
            }
            else
            {
                slotMani = FUN_101699d0();
                FUN_101699f0(slotMani, node->m_0050, node->name);
            }
            if (found < 0)
            {
                file->Read(&chunkSize, 1, 4);
                node->m_028c = (TextureRef *)DAT_11b0b938(4, "motion");
                file->Read(&node->frameMatrixCount, 1, 4);
                node->frameMatrices = (Matrix4 *)DAT_11b0b938(node->frameMatrixCount << 6, "tmp[i]->m_animat");
                for (int i = 0; i < node->frameMatrixCount; i++)
                    file->Read(&node->frameMatrices[i], 1, 0x40);
                node->m_028c->slot = slotMani;
                if (slotMani >= 0)
                {
                    DAT_10fdf1d0[slotMani] = node->m_028c;
                    DAT_1101c820[slotMani] = node->frameMatrices;
                    DAT_11017600[slotMani] = node->frameMatrixCount;
                    FUN_10169be0(node->m_028c);
                }
            }
        }
        else if (strncmp(tag, "MAN2", 4) == 0 && node != 0)
        {
            sawMotion = 1;
            if (node->m_028c != 0)
                node->M_10191210();
            slotMan2 = -1;
            int found = -1;
            if (arg == 0)
            {
                found = FUN_10169ab0(node->m_0050, node->name, &slotMan2);
                if (found >= 0)
                {
                    node->m_028c = DAT_10fdf1d0[found];
                    node->frameMatrices = DAT_1101c820[found];
                    node->frameMatrixCount = DAT_11017600[found];
                    FUN_10169be0(node->m_028c);
                    file->Read(&chunkSize, 1, 4);
                    file->Seek(chunkSize, 1);
                }
            }
            else
            {
                slotMan2 = FUN_101699d0();
                FUN_101699f0(slotMan2, node->m_0050, node->name);
            }
            if (found < 0)
            {
                file->Read(&chunkSize, 1, 4);
                node->m_028c = (TextureRef *)DAT_11b0b938(4, "motion");
                file->Read(&node->frameMatrixCount, 1, 4);
                node->frameMatrices = (Matrix4 *)DAT_11b0b938(node->frameMatrixCount << 6, "tmp[i]->m_animat");
                file->ReadPacked(node->frameMatrices, 1, node->frameMatrixCount << 6);
                node->m_028c->slot = slotMan2;
                if (slotMan2 >= 0)
                {
                    DAT_10fdf1d0[slotMan2] = node->m_028c;
                    DAT_1101c820[slotMan2] = node->frameMatrices;
                    DAT_11017600[slotMan2] = node->frameMatrixCount;
                    FUN_10169be0(node->m_028c);
                }
            }
        }
        else
        {
            file->Read(&chunkSize, 1, 4);
            file->Seek(chunkSize, 1);
        }
        file->Read(tag, 1, 4);
    }
    M_1019bca0(nodes, nodeCount);
    return 1;
}
