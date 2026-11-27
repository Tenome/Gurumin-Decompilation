// Building a node's mesh from a triangle list: the vertices are sorted into one vertex pack per material.

#include <stdio.h>

#include "vec4.h"
#include "matrix4.h"
#include "mesh.h"
#include "node.h"
#include "engine_memory.h"
#include "render_vars.h"


// Material numbers a vertex can carry (the size of the per-material tables).
#define MESH_MAX_MATERIALS 0x2000
// Size of a MeshData block (allocated by tag "CVPCObjA").
#define MESH_DATA_SIZE 0x50
// MeshData::bufferSlot of a new mesh: no entry in the renderer's buffer tables yet.
#define MESH_NO_BUFFER_SLOT (-1)

// 0x101a82d0 (Node::BuildMesh). Called by the model loader (0x10016ec0) with a triangle list: `count` vertices
// (`vertices`, three per triangle, each with its material number in `sourceMaterial`) and `count` indices into them.
// Releases the old mesh and sphere lists, gives the node one blank material when it has none, then makes one
// vertex pack and index list per material used, copies each triangle's vertices into its material's pack (the
// index list just counts up) and creates a new MeshData. Then applies the material colours (unless m_1894 is set),
// runs SplitIntoCells when DAT_11b04500 is set, welds the vertices, sorts the packs and computes the bounds.
// Returns 1, or 0 for an empty list.
int Node::BuildMesh(Vertex *vertices, int *indices, int count)
{
    int packVertexCount[MESH_MAX_MATERIALS];      // vertices of each material's pack
    int listOfMaterial[MESH_MAX_MATERIALS];       // index list of each material
    int packOfMaterial[MESH_MAX_MATERIALS];       // vertex pack of each material
    int materialVertexCount[MESH_MAX_MATERIALS];  // vertices that carry each material
    int packFill[MESH_MAX_MATERIALS];             // vertices copied into each material's pack so far

    if (mesh != 0)
        ReleaseMesh();
    M_101911e0(triangleSpheres, packCount);
    if (triangleSpheres != 0)
        DAT_11b1ba40(triangleSpheres);
    triangleSpheres = 0;
    if (count == 0)
        return 0;

    for (int material = 0; material < MESH_MAX_MATERIALS; material++)
        materialVertexCount[material] = 0;
    for (int material = 0; material < MESH_MAX_MATERIALS; material++)
        packFill[material] = 0;

    if (materialCount == 0) {
        // A blank white material without textures.
        materials = (Material *)DAT_11b0b938(sizeof(Material), "m_material");
        materials[0].textures[0][0] = 0;
        materials[0].textures[1][0] = 0;
        materials[0].textures[2][0] = 0;
        materials[0].textures[3][0] = 0;
        materials[0].colour[0] = 1.0f;
        materials[0].colour[1] = 1.0f;
        materials[0].colour[2] = 1.0f;
        materials[0].alpha = 0;
        materialCount = 1;
    }
    // Looks for the first material without a texture (the result is not used).
    int blank;
    for (blank = 0; blank < materialCount; blank++) {
        if (materials[blank].textures[0][0] == 0)
            break;
    }

    // Count the vertices of each material, keeping each vertex's material number as a whole number.
    int i;
    int material;
    for (i = 0; i < count; i++) {
        material = (int)vertices[i].sourceMaterial;
        materialVertexCount[material]++;
        vertices[i].material = (float)material;
    }

    // One pack and one index list per material used.
    int packTotal = 0;
    int listTotal = 0;
    for (material = 0; material < MESH_MAX_MATERIALS; material++) {
        if (materialVertexCount[material] != 0) {
            packOfMaterial[material] = packTotal;
            listOfMaterial[material] = listTotal;
            packTotal++;
            packVertexCount[material] = materialVertexCount[material];
            listTotal++;
        }
    }
    packs = AllocPackHeaders(packTotal);
    indexLists = AllocIndexLists(packTotal);
    packCount = packTotal;
    for (material = 0; material < MESH_MAX_MATERIALS; material++) {
        if (materialVertexCount[material] != 0) {
            int list = listOfMaterial[material];
            packs[list].SetVertexCount(packVertexCount[material]);
            indexLists[list].Resize(packVertexCount[material]);
            packs[list].material = material;
        }
    }

    // Copy each triangle into its material's pack.
    for (i = 0; i < count; i += 3) {
        int vertex = indices[i];   // the triangle's first vertex
        material = (int)vertices[vertex].sourceMaterial;
        int pack = packOfMaterial[material];
        int fill = packFill[material];
        packs[pack].vertices[fill] = vertices[vertex];
        indexLists[pack].indices[fill] = fill;
        int second = indices[i + 1];
        packs[pack].vertices[fill + 1] = vertices[second];
        indexLists[pack].indices[fill + 1] = fill + 1;
        int third = indices[i + 2];
        packs[pack].vertices[fill + 2] = vertices[third];
        indexLists[pack].indices[fill + 2] = fill + 2;
        fill += 3;
        packFill[material] = fill;
        packs[pack].vertexCount = fill;
        indexLists[pack].count = fill;
    }

    if (mesh != 0)
        fread(0, 1, 1, 0);   // the old mesh must be gone: crash on purpose (fread on a null file)
    mesh = (MeshData *)DAT_11b0b938(MESH_DATA_SIZE, "CVPCObjA");
    mesh->Init();
    mesh->bufferSlot = MESH_NO_BUFFER_SLOT;
    if (m_1894 == 0)
        ApplyMaterialColours();
    if (DAT_11b04500 != 0)
        SplitIntoCells();
    WeldVertices();
    SortPacks();
    ComputeBounds();
    return 1;
}
