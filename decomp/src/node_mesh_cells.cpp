// Splitting a scene node's mesh into voxel cells: the triangles of each vertex pack are sorted by the position of
// their centre into a grid of cells, and every occupied cell becomes a vertex pack of its own.

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <new>

#include "vec4.h"
#include "matrix4.h"
#include "mesh.h"
#include "node.h"
#include "engine_memory.h"
#include "render_vars.h"

Matrix4 IdentityMatrix();                           // 0x101b0ba0
extern "C" int FUN_10169990(MeshData *mesh);        // MeshCache_Release: drops one reference, returns those left

// Only nodes with 1 to this many packs are split (the pack number is one factor of a cell's number).
#define CELL_SPLIT_MAX_PACKS 0x40
// The grid has 100 x 100 x 80 cells per pack; the last cell of each axis also takes everything beyond it.
#define CELL_COUNT_X 100
#define CELL_COUNT_Y 100
#define CELL_COUNT_Z 80
#define CELL_LAST_X 99
#define CELL_LAST_Y 99
#define CELL_LAST_Z 79
#define CELLS_PER_PACK (CELL_COUNT_X * CELL_COUNT_Y * CELL_COUNT_Z)
// Largest pack count the centre lists are reserved for.
#define CELL_CENTRE_LIST_SLOTS 512
// A triangle whose corners are further apart than the cell size times these factors is not split off: its centre is
// moved to CELL_REJECTED and it goes to the last cell. The z limit is tighter than the x/y ones.
#define CELL_XY_SPREAD 1.5f
#define CELL_Z_SPREAD 1.8f
// Centre of a rejected triangle, and the largest coordinate such a centre can have.
#define CELL_REJECTED (-180000.0f)
#define CELL_REJECTED_LIMIT (-100000.0f)
// Start value of the smallest centre coordinates.
#define CELL_MIN_START 100000.0f

// Offset from the grid's start, never negative.
inline float CellOffset(float offset)
{
    return 0.0f < offset ? offset : 0.0f;
}
// Number of the cell (cellX, cellY, cellZ) among the cells of the pack whose numbers start at cellBase, and the cell an
// offset falls in along one axis (the last cell takes the rest).
#define CELL_NUMBER ((cellY * CELL_COUNT_X + cellX) * CELL_COUNT_Z + cellBase + cellZ)
#define CELL_INDEX(offset, last) ((int)CellOffset(offset) / cellSize < (last) ? (int)CellOffset(offset) / cellSize : (last))

// 0x1019c8d0 (Node::SplitIntoCells). NEAR-MISS: all code matches except that the two induction variables of the copy pass
// (pack * 0x74 and the cell base) swap their stack slots and registers. Run by BuildMesh when DAT_11b04500 is set, for a node with 1 to 64 packs: drops
// the node's mesh, finds the centre of every triangle (the corners taken through an identity matrix; a triangle whose
// corners are far apart gets CELL_REJECTED), then sorts the triangles into cells of DAT_10203478 units, counting
// the cells used. Every occupied cell gets a new pack (and index list) with the triangles' vertices copied in, with
// the same material as the old pack; the last cell of x and y is flagged in m_0040. Frees the old packs, installs
// the new ones and makes a new empty mesh. Returns 0.
int Node::SplitIntoCells()
{
    if (packCount > CELL_SPLIT_MAX_PACKS || packCount == 0)
        return 0;
    if (mesh != 0 && FUN_10169990(mesh) == 0)
    {
        mesh->Release();
        DAT_11b1ba40(mesh);
        mesh = 0;
    }
    int *cellPack = (int *)operator new(packCount * CELLS_PER_PACK * sizeof(int));   // 1 + number of the new pack of each cell
    memset(cellPack, 0, packCount * CELLS_PER_PACK * sizeof(int));
    int *cellCount = (int *)operator new(packCount * CELLS_PER_PACK * sizeof(int));  // triangles still to place in each cell
    memset(cellCount, 0, packCount * CELLS_PER_PACK * sizeof(int));

    Vec4 cell;
    Vec4 point;
    Vec4 corner0;
    Vec4 corner1;
    Vec4 corner2;
    int cellSize = DAT_10203478;
    Matrix4 matrix = worldMatrix;
    matrix = IdentityMatrix();

    Vec4 *centres[CELL_CENTRE_LIST_SLOTS];   // the centre of each triangle, per pack
    float minX = CELL_MIN_START;
    float minY = CELL_MIN_START;
    float minZ = CELL_MIN_START;
    int pack;
    for (pack = 0; pack < packCount; pack++)
    {
        Vec4 *list = (Vec4 *)DAT_11b0b938((unsigned int)(indexLists[pack].count << 4) / 3, "vtx[x]");
        centres[pack] = list;
        for (int triangle = 0; triangle < indexLists[pack].count; triangle += 3)
        {
            float scale = (float)cellSize;
            float xyLimit = scale * CELL_XY_SPREAD;
            int vertex = indexLists[pack].indices[triangle];
            point.x = packs[pack].vertices[vertex].position[0];
            point.y = packs[pack].vertices[vertex].position[1];
            point.z = packs[pack].vertices[vertex].position[2];
            corner0.Copy(Project(point, matrix));
            vertex = indexLists[pack].indices[triangle + 1];
            point.x = packs[pack].vertices[vertex].position[0];
            point.y = packs[pack].vertices[vertex].position[1];
            point.z = packs[pack].vertices[vertex].position[2];
            corner1.Copy(Project(point, matrix));
            vertex = indexLists[pack].indices[triangle + 2];
            point.x = packs[pack].vertices[vertex].position[0];
            point.y = packs[pack].vertices[vertex].position[1];
            point.z = packs[pack].vertices[vertex].position[2];
            corner2.Copy(Project(point, matrix));
            list[triangle / 3].x = (corner0.x + corner1.x + corner2.x) / 3.0f;
            list[triangle / 3].y = (corner0.y + corner1.y + corner2.y) / 3.0f;
            list[triangle / 3].z = (corner0.z + corner1.z + corner2.z) / 3.0f;
            if (fabs(corner0.x - corner1.x) > xyLimit)
                goto rejected;
            {
                double spread = (double)cellSize * 1.5;
                if (fabs(corner0.x - corner2.x) > spread || fabs(corner1.x - corner2.x) > spread
                    || fabs(corner0.y - corner1.y) > xyLimit || fabs(corner0.y - corner2.y) > spread
                    || fabs(corner1.y - corner2.y) > spread)
                    goto rejected;
            }
            {
                float zLimit = scale * CELL_Z_SPREAD;
                if (fabs(corner0.z - corner1.z) > zLimit || fabs(corner0.z - corner2.z) > zLimit
                    || fabs(corner1.z - corner2.z) > zLimit)
                    goto rejected;
            }
            if (list[triangle / 3].x < minX)
                minX = list[triangle / 3].x;
            if (list[triangle / 3].y < minY)
                minY = list[triangle / 3].y;
            if (list[triangle / 3].z < minZ)
                minZ = list[triangle / 3].z;
            continue;
rejected:
            list[triangle / 3].x = CELL_REJECTED;
            list[triangle / 3].y = CELL_REJECTED;
            list[triangle / 3].z = CELL_REJECTED;
        }
    }
    minX = minX - 1.0f;
    minY = minY - 1.0f;
    minZ = minZ - 1.0f;

    int newPackCount = 0;
    for (pack = 0; pack < packCount; pack++)
    {
        int cellBase = pack * CELLS_PER_PACK;
        for (int triangle = 0; triangle < indexLists[pack].count; triangle += 3)
        {
            cell.Copy(centres[pack][triangle / 3]);
            float offset = cell.x - minX;
            int cellX = CELL_INDEX(offset, CELL_LAST_X);
            offset = cell.y - minY;
            int cellY = CELL_INDEX(offset, CELL_LAST_Y);
            offset = cell.z - minZ;
            int cellZ = CELL_INDEX(offset, CELL_LAST_Z);
            if (centres[pack][triangle / 3].x <= CELL_REJECTED_LIMIT)
            {
                cellX = CELL_LAST_X;
                cellY = CELL_LAST_Y;
                cellZ = CELL_LAST_Z;
            }
            cellCount[CELL_NUMBER]++;
            if (cellPack[CELL_NUMBER] == 0)
            {
                cellPack[CELL_NUMBER] = newPackCount + 1;
                newPackCount++;
            }
        }
    }

    VertexPack *newPacks = (VertexPack *)DAT_11b0b938(newPackCount * sizeof(VertexPack), "vp");
    int i;
    for (i = 0; i < newPackCount; i++)
        newPacks[i].Init();
    IndexList *newLists = (IndexList *)DAT_11b0b938(newPackCount * sizeof(IndexList), "vpi");
    for (i = 0; i < newPackCount; i++)
        newLists[i].Init();

    for (pack = 0; pack < packCount; pack++)
    {
        int cellBase = pack * CELLS_PER_PACK;
        for (int triangle = 0; triangle < indexLists[pack].count; triangle += 3)
        {
            cell.Copy(centres[pack][triangle / 3]);
            float offset = cell.x - minX;
            int cellX = CELL_INDEX(offset, CELL_LAST_X);
            offset = cell.y - minY;
            int cellY = CELL_INDEX(offset, CELL_LAST_Y);
            offset = cell.z - minZ;
            int cellZ = CELL_INDEX(offset, CELL_LAST_Z);
            if (centres[pack][triangle / 3].x <= CELL_REJECTED_LIMIT)
            {
                cellX = CELL_LAST_X;
                cellY = CELL_LAST_Y;
                cellZ = CELL_LAST_Z;
            }
            int newPack = cellPack[CELL_NUMBER] - 1;
            if (cellCount[CELL_NUMBER] > 0)
            {
                newLists[newPack].Resize(cellCount[CELL_NUMBER] * 3);
                newPacks[newPack].SetVertexCount(cellCount[CELL_NUMBER] * 3);
                newPacks[newPack].m_0040 = 0;
                if (cellX == CELL_LAST_X && cellY == CELL_LAST_Y)
                    newPacks[newPack].m_0040 = 1;
                newPacks[newPack].material = packs[pack].material;
                newPacks[newPack].center[0] = 0;
                newPacks[newPack].center[1] = 0;
                newPacks[newPack].center[2] = 0;
                newPacks[newPack].radius = 1.0f;
                cellCount[CELL_NUMBER] = 0;
            }
            int fill = -cellCount[CELL_NUMBER];
            newLists[newPack].indices[fill] = fill;
            newLists[newPack].indices[fill + 1] = fill + 1;
            newLists[newPack].indices[fill + 2] = fill + 2;
            newPacks[newPack].vertices[fill] = packs[pack].vertices[indexLists[pack].indices[triangle]];
            newPacks[newPack].vertices[fill + 1] = packs[pack].vertices[indexLists[pack].indices[triangle + 1]];
            int third = indexLists[pack].indices[triangle + 2];
            newPacks[newPack].vertices[fill + 2] = packs[pack].vertices[third];
            cellCount[CELL_NUMBER] -= 3;
        }
    }

    M_101911b0(packs, packCount);
    M_101911e0(indexLists, packCount);
    if (packs != 0)
        DAT_11b1ba40(packs);
    if (indexLists != 0)
        DAT_11b1ba40(indexLists);
    for (i = 0; i < packCount; i++)
        DAT_11b1ba40(centres[i]);
    packs = newPacks;
    indexLists = newLists;
    packCount = newPackCount;
    free(cellCount);
    free(cellPack);
    mesh = (MeshData *)DAT_11b0b938(0x50, "CVPCObjC");
    mesh->Init();
    mesh->bufferSlot = -1;
    return 0;
}
