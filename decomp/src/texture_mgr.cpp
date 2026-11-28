// Texture manager, triangle batches (TriangleBatch, tri_batch.h), model owners and helpers.

#include "texture_object.h"

#include "texture_manager.h"

#include "texture_mgr.h"

#include "matrix4.h"


#include "anim_model.h"

#include "node.h"

#include "mesh.h"

#include "render_vars.h"
#include "engine_memory.h"
extern "C" int FUN_10174c70(int slot, int force);

// Constructor: the member constructors do the work.
Transform::Transform()
{
}

// 0x10172a30. Destructor: only the matrix member has one (tail call of 0x1013eda0 with this + 0x34).
Transform::~Transform()
{
}

// Sets the batch's vertex buffer and capacity.
int TriangleBatch::SetBuffer(void *buffer, int capacity)
{
    vertices = (ScreenVertex *)buffer;
    this->capacity = capacity;
    return 1;
}

// Empties the batch and forgets its texture.
int TriangleBatch::Reset()
{
    count = 0;
    texture = 0;
    blendMode = BLEND_MODE_UNSET;
    return 1;
}

// Switches the batch to blend mode `mode`, flushing the queued triangles when the mode changes.
int TriangleBatch::SetBlend(int mode, int unused)
{
    if (blendMode != BLEND_MODE_UNSET)
    {
        if (blendMode != mode)
            Flush();
        blendMode = mode;
        return 0;
    }
    blendMode = mode;
    return 0;
}

// Unloads every texture slot and clears both name tables. Returns 1.
int TextureManager::M_10173480()
{
    for (int i = 0; i < 1600; i++)
    {
        if (DAT_10ffe5f8[i][0] != 0)
        {
            DAT_118d0ba8[i].Unload();
            DAT_10ffe5f8[i][0] = 0;
            DAT_10f8dbb8[i][0] = 0;
        }
    }
    return 1;
}

// Runs M_10169df0 on every loaded texture (re-create after a device reset).
int TextureManager::M_101734d0()
{
    for (int i = 0; i < 1600; i++)
    {
        if (DAT_10ffe5f8[i][0] != 0)
            DAT_118d0ba8[i].Recreate();
    }
    return 1;
}

// Runs M_10169e10 on every loaded texture (restore the texture data).
int TextureManager::M_10173510()
{
    for (int i = 0; i < 1600; i++)
    {
        if (DAT_10ffe5f8[i][0] != 0)
            DAT_118d0ba8[i].Restore();
    }
    return 1;
}

// Unloads all textures and shuts the renderer down (Shutdown, render_shutdown.cpp).
void TextureManager::M_10175e70()
{
    M_10173480();
    Shutdown();
}

// Returns the playback's current frame, creating the playback object on first use.
int AnimModel::M_1017b1c0()
{
    if (motion == 0)
    {
        motion = (Playback *)DAT_11b0b938(0x7c84, "");
        motion->Init();
        motion->BindModel(this);
    }
    return motion->layer;
}

// After a map change: unloads every texture that was not marked as kept and has no generated alpha.
extern "C" int FUN_10188fb0(void)
{
    for (int i = 0; i < 1600; i++)
    {
        if (DAT_10ffe5f8[i][0] != 0 && DAT_10fa6db8[i] == 0 && DAT_118d0ba8[i].generatedAlpha == 0)
            FUN_10174c70(i, 1);
    }
    return 0;
}

// Frees the vertex data of `count` pack headers (VertexPack::FreeVertices on each).
extern "C" void __stdcall FUN_101911b0(VertexPack *packs, int count)
{
    if (packs != 0)
    {
        for (; count > 0; count--)
        {
            packs->FreeVertices();
            packs++;
        }
    }
}


// Queues `count` vertices for texture `id`, flushing first when the texture changes or the buffer is full.
int TriangleBatch::AddVertices(TextureObject *texture, void *source, int count)
{
    DAT_10ff1dec++;
    if (this->texture != 0 && this->texture != texture)
        Flush();
    this->texture = texture;
    if (this->count + count >= capacity)
        Flush();
    AppendVertices((const ScreenVertex *)source, count);
    return 0;
}
// Queues a textured quad (two triangles, six vertices) with `texture`, flushing first when the texture changes or
// the buffer is full.
int TriangleBatch::AddQuad(TextureObject *texture, float left, float top, float right, float bottom, float u0,
                           float v0, float u1, float v1, float z, float alpha, float r, float g, float b, float r2,
                           float g2, float b2, float m_0078)
{
    DAT_10ff1dec++;
    if (this->texture != 0 && this->texture != texture)
        Flush();
    this->texture = texture;
    if (count + 6 >= capacity)
        Flush();
    AppendQuad(left, top, right, bottom, u0, v0, u1, v1, z, alpha, r, g, b, r2, g2, b2, m_0078);
    return 0;
}

// Releases one reference to texture slot `slot`. The texture is unloaded (and its name cleared, freeing the slot)
// when the count reaches zero or when `force` is set. Returns the remaining count, or -1 for an unused slot.
extern "C" int FUN_10174c70(int slot, int force)
{
    if (slot == -1 || DAT_10ffe5f8[slot][0] == 0)
        return -1;
    char refs = --DAT_10fdeb90[slot];
    if (force != 0 || refs == 0)
    {
        DAT_10fdeb90[slot] = 0;
        DAT_118d0ba8[slot].Unload();
        DAT_10ffe5f8[slot][0] = 0;
    }
    return DAT_10fdeb90[slot];
}


// Starts motion `motion` on this model (0x1017b0e0, CharModel::M_1017b0e0 in other files): creates the playback
// object on first use, then hands the request to it. `a`/`b` are the playback options (loop mode, blend frames).
void AnimModel::PlayMotion(int layer, int a, int b, float speed)
{
    if (motion == 0)
    {
        motion = (Playback *)DAT_11b0b938(0x7c84, "");
        motion->Init();
        motion->BindModel(this);
    }
    if (important != 0)
        DAT_11b04508 = 1;
    motion->Play(layer, a, b, speed);
    DAT_11b04508 = 0;
}

// Loads model file `path` into this model's existing node tree (0x1017aeb0, CharModel::M_1017aeb0 elsewhere).
// Counts the loads, looks up the biped root "Bip01" (the result is unused in the shipped code), resets m_2494 and
// rebinds the motion playback to the new model. Returns 0 when the model has no node tree yet.
int AnimModel::LoadModel(const char *path)
{
    if (root == 0)
        return 0;
    root->PreScanFile(path, 0);
    if (loadCount < 3)
        loadCount++;
    root->Find("Bip01", 0);
    ZeroVec unused1;
    ZeroVec unused2;
    m_2494 = 0;
    if (motion != 0)
        motion->BindModel(this);
    return 1;
}

// Updates this model's collision: releases the saved matrix states of the node tree (Node::ReleaseSavedStates) and collides the model's position and radius
// with an identity-like work matrix (Node::StartProximityPass, scene_node.cpp).
// Returns 0.
int AnimModel::UpdateCollision()
{
    Matrix4 work;
    if (root != 0)
        root->ReleaseSavedStates();
    m_2384 = 0;
    root->StartProximityPass(matrix.m, &work, 0, motionFrame, 0);
    return 0;
}
