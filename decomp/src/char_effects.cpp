// Per-character effect spawning: the effect manager (DAT_10daaa80, EffectManager) starts an effect and writes
// its handle into the character's entry of DAT_10219770.

#include "tri_batch.h"
#include <string.h>

#include <stdio.h>

#include "vec4.h"

#include "effect_manager.h"

#include "char_tables.h"
#include "char_vars.h"
#include "render_vars.h"
#include "render_state.h"
#include "map_vars.h"

// Starts effect `effect` at `position` for character slot `index` and records its handle in that slot.
extern "C" int FUN_1001d8b0(int index, int effect, Vec4 position, int parent, int unused, const Matrix4 *owner, int value, int frame, const Matrix4 *matrix, const EffectOptions *options)
{
    DAT_10daaa80.Start(effect, position, parent, unused, owner, value, frame, matrix, options, &DAT_10219770[index]);
    return 0;
}

// Stops every effect: rebinds the effect batch to its buffer, clears all effect tables and marks their slots free
// (-1), and resets the six group offsets. The offset table has only five values in the original; the sixth group
// gets whatever was on the stack (kept as is).
int EffectManager::Reset()
{
    int offsets[6];
    DAT_1101c804.SetBuffer(DAT_10fc6a90, 0x400);
    m_0050 = 0;
    memset(DAT_118e99f8, 0, sizeof(EffectLight) * EFFECT_LIGHT_COUNT);   // the ribbon lights after them stay
    memset(DAT_1115cfe8, 0, EFFECT_RECORD_COUNT * sizeof(EffectRecord));
    memset(DAT_11134cc8, 0, EFFECT_SPRITE_COUNT * sizeof(EffectSprite));
    memset(DAT_118a8888, 0, EFFECT_SPRITE_COUNT * sizeof(EffectSprite));
    int i;
    for (i = 0; i < 840; i++)
        DAT_1115cfe8[i].id = -1;
    for (i = 0; i < 840; i++)
        DAT_11134cc8[i].type = -1;
    for (i = 0; i < 840; i++)
        DAT_118a8888[i].type = -1;
    // The two five-entry lookup caches.
    DAT_1101d820[0] = -1;
    DAT_1101d820[1] = -1;
    DAT_1101d820[2] = -1;
    DAT_1101d820[3] = -1;
    DAT_1101d820[4] = -1;
    DAT_11018604[0] = -1;
    DAT_11018604[1] = -1;
    DAT_11018604[2] = -1;
    DAT_11018604[3] = -1;
    DAT_11018604[4] = -1;
    offsets[0] = 0;
    offsets[1] = 0x1cc;
    offsets[2] = 0x244;
    offsets[3] = 0x2a8;
    offsets[4] = 0x2f8;
    for (i = 0; i < 6; i++)
    {
        m_0030[i] = offsets[i];
        m_0010[i] = offsets[i];
    }
    return 0;
}

// 0x101883b0. Stops every running effect started by character slot `owner`: each one is marked free, unlinked from
// its list (five lists with heads DAT_1101d820 and tails DAT_11018604) and its light entry is cleared; the
// effects attached to it are stopped too. Returns 0.
int EffectManager::RemoveOwnedBy(int owner)
{
    for (int list = 0; list < 5; list++)
    {
        for (int i = DAT_1101d820[list]; i > -1; i = DAT_1115cfe8[i].next)
        {
            if (DAT_1115cfe8[i].id > -1 && DAT_1115cfe8[i].options.owner == owner)
            {
                DAT_1115cfe8[i].id = -1;
                if (DAT_1115cfe8[i].prev == -1)
                    DAT_1101d820[list] = DAT_1115cfe8[i].next;
                else
                    DAT_1115cfe8[DAT_1115cfe8[i].prev].next = DAT_1115cfe8[i].next;
                if (DAT_1115cfe8[i].next != -1)
                    DAT_1115cfe8[DAT_1115cfe8[i].next].prev = DAT_1115cfe8[i].prev;
                else
                    DAT_11018604[list] = DAT_1115cfe8[i].prev;
                if (DAT_1115cfe8[i].light != 0)
                    DAT_118e99f8[DAT_1115cfe8[i].light].active = 0;
                RemoveChildren(i);
            }
        }
    }
    return 0;
}

// 0x10172260. Stops every running effect attached to record `parent`, and the effects attached to those in turn:
// each one is marked free, unlinked from its list, detached and its light entry cleared. Returns 0.
int EffectManager::RemoveChildren(int parent)
{
    for (int list = 0; list < 5; list++)
    {
        for (int i = DAT_1101d820[list]; i > -1; i = DAT_1115cfe8[i].next)
        {
            if (DAT_1115cfe8[i].id != -1 && DAT_1115cfe8[i].parent == parent)
            {
                DAT_1115cfe8[i].id = -1;
                if (DAT_1115cfe8[i].prev == -1)
                    DAT_1101d820[list] = DAT_1115cfe8[i].next;
                else
                    DAT_1115cfe8[DAT_1115cfe8[i].prev].next = DAT_1115cfe8[i].next;
                if (DAT_1115cfe8[i].next != -1)
                    DAT_1115cfe8[DAT_1115cfe8[i].next].prev = DAT_1115cfe8[i].prev;
                else
                    DAT_11018604[list] = DAT_1115cfe8[i].prev;
                DAT_1115cfe8[i].parent = -1;
                if (DAT_1115cfe8[i].light != 0)
                    DAT_118e99f8[DAT_1115cfe8[i].light].active = 0;
                RemoveChildren(i);
            }
        }
    }
    return 0;
}

// 0x10188490. Stops every running effect whose definition has type `type` (the record's id indexes the definition
// table DAT_1101dfdc), including the effects attached to it. Returns 0.
int EffectManager::RemoveOfType(int type)
{
    for (int list = 0; list < 5; list++)
    {
        for (int i = DAT_1101d820[list]; i > -1; i = DAT_1115cfe8[i].next)
        {
            if (DAT_1115cfe8[i].id > -1 && DAT_1101dfdc[DAT_1115cfe8[i].id].id == type)
            {
                DAT_1115cfe8[i].id = -1;
                if (DAT_1115cfe8[i].prev == -1)
                    DAT_1101d820[list] = DAT_1115cfe8[i].next;
                else
                    DAT_1115cfe8[DAT_1115cfe8[i].prev].next = DAT_1115cfe8[i].next;
                if (DAT_1115cfe8[i].next != -1)
                    DAT_1115cfe8[DAT_1115cfe8[i].next].prev = DAT_1115cfe8[i].prev;
                else
                    DAT_11018604[list] = DAT_1115cfe8[i].prev;
                if (DAT_1115cfe8[i].light != 0)
                    DAT_118e99f8[DAT_1115cfe8[i].light].active = 0;
                RemoveChildren(i);
            }
        }
    }
    return 0;
}

// 0x10188580. Advances all effects by one frame: every running effect is stepped unless it is hidden (flag 2,
// skipped unless `includeHidden`), outside a restricted pass (`restricted` steps only effects with flag 1) or paused
// (unless `ignorePause`). The short-lived records of the two other tables expire when their age reaches their
// lifetime, and effect lights fade out over the effect's light frames plus 10. Returns 0.
int EffectManager::Update(int restricted, int includeHidden, int ignorePause)
{
    for (int list = 0; list < 5; list++)
    {
        for (int i = DAT_1101d820[list]; i > -1; i = DAT_1115cfe8[i].next)
        {
            if (DAT_1115cfe8[i].id != -1 && (includeHidden != 0 || (DAT_1115cfe8[i].options.flags & EFFECT_FLAG_HIDDEN) == 0) &&
                (restricted == 0 || (DAT_1115cfe8[i].options.flags & EFFECT_FLAG_RESTRICTED_PASS) != 0) &&
                (DAT_1115cfe8[i].pause == 0 || *DAT_1115cfe8[i].pause == 0 || ignorePause != 0))
                Step(i, list);
        }
    }
    int j;
    for (j = 0; j < 840; j++)
    {
        if (DAT_11134cc8[j].type != -1)
        {
            DAT_11134cc8[j].age++;
            if (DAT_11134cc8[j].age >= DAT_11134cc8[j].lifetime)
                DAT_11134cc8[j].type = -1;
        }
    }
    for (j = 0; j < 840; j++)
    {
        if (DAT_118a8888[j].type != -1)
        {
            DAT_118a8888[j].age++;
            if (DAT_118a8888[j].age >= DAT_118a8888[j].lifetime)
                DAT_118a8888[j].type = -1;
        }
    }
    for (int k = 0; k < 0x80; k++)
    {
        if (DAT_118e99f8[k].active != 0 && DAT_118e99f8[k].frame != 0)
        {
            int frame = DAT_118e99f8[k].frame + 1;
            int frames = DAT_1101dfdc[DAT_118e99f8[k].type].lightFrames;
            DAT_118e99f8[k].frame = frame;
            DAT_118e99f8[k].fade = 1.0f - (float)frame / (float)(frames + 10);
            if (frame >= frames + 10)
            {
                DAT_118e99f8[k].fade = 0;
                DAT_118e99f8[k].frame = 0;
                DAT_118e99f8[k].active = 0;
            }
        }
    }
    return 0;
}

// 0x1016fb10. Marks the effect definitions (field 0xfc) and both short-lived record tables free (-1). Defined
// before Init: Init relies on this function leaving edx alone. Returns 0.
int EffectManager::ClearTables()
{
    int i;
    for (i = 0; i < EFFECT_DEFINITION_COUNT; i++)
        DAT_1101dfdc[i].texture = -1;
    for (i = 0; i < 840; i++)
        DAT_11134cc8[i].type = -1;
    for (i = 0; i < 840; i++)
        DAT_118a8888[i].type = -1;
    return 0;
}

extern "C" int FUN_1016f900(void);        // adds the default effect definition (effect_defs.cpp)

// 0x101842a0. Full initialisation at load time: Reset plus the effect definitions. The colour becomes white, every
// table is cleared and marked free, the lists emptied, the six group offsets set and the default definition added.
// The sixth offset is uninitialised in the original, as in Reset. Returns 0.
int EffectManager::Init()
{
    int offsets[6];
    colour.Set(1.0f, 1.0f, 1.0f, 1.0f);
    DAT_1101c804.SetBuffer(DAT_10fc6a90, 0x400);
    M_1016fab0();
    m_0050 = 0;
    memset(DAT_118e99f8, 0, sizeof(EffectLight) * EFFECT_LIGHT_COUNT);   // the ribbon lights after them stay
    memset(DAT_1115cfe8, 0, EFFECT_RECORD_COUNT * sizeof(EffectRecord));
    memset(DAT_1101de68, 0, sizeof(DAT_1101de68));
    memset(DAT_11134cc8, 0, EFFECT_SPRITE_COUNT * sizeof(EffectSprite));
    memset(DAT_118a8888, 0, EFFECT_SPRITE_COUNT * sizeof(EffectSprite));
    int i;
    for (i = 0; i < 32; i++)
        DAT_110186a8[i] = -1;
    DAT_1101de64 = 0;
    for (i = 0; i < 840; i++)
        DAT_1115cfe8[i].id = -1;
    for (i = 0; i < 840; i++)
        DAT_11134cc8[i].type = -1;
    for (i = 0; i < 840; i++)
        DAT_118a8888[i].type = -1;
    for (i = 0; i < EFFECT_DEFINITION_COUNT; i++)
    {
        DAT_1101dfdc[i].id = -1;
        DAT_1101dfdc[i].texture = -1;
    }
    DAT_1101d820[0] = -1;
    DAT_1101d820[1] = -1;
    DAT_1101d820[2] = -1;
    DAT_1101d820[3] = -1;
    DAT_1101d820[4] = -1;
    DAT_11018604[0] = -1;
    DAT_11018604[1] = -1;
    DAT_11018604[2] = -1;
    DAT_11018604[3] = -1;
    DAT_11018604[4] = -1;
    ClearTables();
    offsets[0] = 0;
    offsets[1] = 0x1cc;
    offsets[2] = 0x244;
    offsets[3] = 0x2a8;
    offsets[4] = 0x2f8;
    for (i = 0; i < 6; i++)
    {
        m_0030[i] = offsets[i];
        m_0010[i] = offsets[i];
    }
    FUN_1016f900();
    return 0;
}


// Effect records whose slot is free must not be in a list: crash on purpose (fread on a null file).
#define CHECK_RECORD(i)     if (DAT_1115cfe8[i].id == -1)         fread(0, 1, 1, 0)

// 0x10190e30 (EffectManager::Draw). Draws the effects of render pass `pass`: pass 1 draws the list-1 effects whose
// part is in the visible area and in front of its depth limit (+20), marking them drawn; other passes draw their own
// list, pass 0 also the list-1 effects not drawn yet. Then the sprites of both tables in the pass's range, and in
// pass 0 the effect lights. Returns 0.
int EffectManager::Draw(const Matrix4 &view, const Matrix4 *camera, unsigned char pass)
{
    FUN_1013f070(RENDER_ALPHA_TEST, 1, 0xf);
    FUN_1013f070(RENDER_TEXTURE_FILTER, 1, 0);
    FUN_1013f070(RENDER_DEPTH_WRITE, 0, 0);
    int ranges[10] = {0, 0x1cc, 0x244, 0x2a8, 0x2f8, 0x1cc, 0x244, 0x2a8, 0x2f8, 0x348};   // first, then end per pass
    DAT_1101c804.Reset();
    int i;
    if (pass == 1)
    {
        for (i = DAT_1101d820[1]; i > -1; i = DAT_1115cfe8[i].next)
        {
            if (DAT_1115cfe8[i].id == -1)
                fread(0, 1, 1, 0);
            else
            {
                if (DAT_1115cfe8[i].partPositions[DAT_1115cfe8[i].part].InRect(&DAT_10f668a8) != 0 &&
                    DAT_10f668c0 + 20.0f > DAT_1115cfe8[i].partPositions[DAT_1115cfe8[i].part].z)
                {
                    FUN_1013f070(RENDER_CULL_MODE, CULL_NONE, 0);
                    DrawRecord(i, view, camera, 1);
                    DAT_1115cfe8[i].options.drawn = 1;
                }
                else
                    DAT_1115cfe8[i].options.drawn = 0;
                CHECK_RECORD(i);
            }
        }
    }
    else
    {
        for (i = DAT_1101d820[pass]; i > -1; i = DAT_1115cfe8[i].next)
        {
            if (DAT_1115cfe8[i].id != -1)
            {
                FUN_1013f070(RENDER_CULL_MODE, CULL_NONE, 0);
                DrawRecord(i, view, camera, pass);
            }
            CHECK_RECORD(i);
        }
        if (pass == 0)
        {
            for (i = DAT_1101d820[1]; i > -1; i = DAT_1115cfe8[i].next)
            {
                if (DAT_1115cfe8[i].id == -1)
                    fread(0, 1, 1, 0);
                else
                {
                    if (DAT_1115cfe8[i].options.drawn == 0)
                    {
                        FUN_1013f070(RENDER_CULL_MODE, CULL_NONE, 0);
                        DrawRecord(i, view, camera, 0);
                        DAT_1115cfe8[i].options.drawn = 1;
                    }
                    CHECK_RECORD(i);
                }
            }
        }
    }
    FUN_1013f070(RENDER_ALPHA_TEST, 1, 0xf);
    FUN_1013f070(RENDER_TEXTURE_FILTER, 1, 0);
    FUN_1013f070(RENDER_CULL_MODE, CULL_NONE, 0);
    int sprite;
    for (sprite = ranges[pass]; sprite < ranges[pass + 5]; sprite++)
    {
        if (DAT_11134cc8[sprite].type != -1)
            DrawSpriteA(sprite, view, camera);
    }
    FUN_1013f070(RENDER_CULL_MODE, CULL_NONE, 0);
    for (sprite = ranges[pass]; sprite < ranges[pass + 5]; sprite++)
    {
        if (DAT_118a8888[sprite].type != -1)
            DrawSpriteB(sprite, view, camera);
    }
    if (pass == 0)
    {
        for (i = 0; i < 0x80; i++)
        {
            if (DAT_118e99f8[i].active != 0)
                DrawLight(i, view, camera);
        }
    }
    DAT_1101c804.Flush();
    FUN_1013f070(RENDER_BLEND_MODE, BLEND_ALPHA, 0);
    FUN_1013f070(RENDER_DEPTH_WRITE, 1, 0);
    FUN_1013f070(RENDER_DEPTH_TEST, 1, 0);
    FUN_1013f070(RENDER_CULL_MODE, CULL_DEFAULT, 0);
    return 0;
}
