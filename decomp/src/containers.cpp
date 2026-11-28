// Engine containers (0x101ac000-0x101af000): vertex packs, index lists, key-frame tracks and banks, a motion
// list and matrix helpers. Buffers come from the engine allocator (DAT_11b0b938 = allocate(size, tag),
// DAT_11b1ba40 = free) or the CRT heap.
//
// Key frames: a KeyTrack is a growable list of (frame, Vec4 value, flag) kept sorted by frame; a KeyBank is a
// fixed 0x80-key version used while loading. Flag 4 marks a key as a motion frame key.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "vec4.h"

#include "mesh.h"

#include "key_track.h"

#include "matrix4.h"
#include "engine_memory.h"

// Reallocates the vertex buffer for `count` vertices (0x70 bytes each, see vertex.cpp). Returns 1.
int VertexPack::SetVertexCount(int count)
{
    if (vertices != 0)
        DAT_11b1ba40(vertices);
    vertices = 0;
    void *buffer = DAT_11b0b938(count * 0x70, "");
    vertexCount = count;
    vertices = (Vertex *)buffer;
    return 1;
}

// Frees the vertex buffer. Returns 1.
int VertexPack::FreeVertices()
{
    if (vertices != 0)
        DAT_11b1ba40(vertices);
    vertices = 0;
    return 1;
}

// Frees the index buffer and sets the count to 0. Returns 1.
int IndexList::Free()
{
    if (indices != 0)
        DAT_11b1ba40(indices);
    indices = 0;
    count = 0;
    return 1;
}

// Reallocates the index buffer for `newCount` indexes (contents are not kept). Returns 1.
int IndexList::Resize(int newCount)
{
    if (indices != 0)
        DAT_11b1ba40(indices);
    indices = 0;
    int *buffer = (int *)DAT_11b0b938(newCount * 4, "");
    count = newCount;
    indices = buffer;
    return 1;
}

// Makes this list a copy of `source`.
int IndexList::CopyFrom(const IndexList *source)
{
    count = 0;
    if (source == 0)
        return 0;
    if (source->count == 0)
        return 0;
    Resize(source->count);
    memcpy(indices, source->indices, source->count * 4);
    return 1;
}

// Empties the bank, leaving one zero key at frame 0 as the default value.
void KeyBank::Clear()
{
    count = 0;
    frames[0] = 0;
    float zero = 0;
    values[0].Fill(zero);
}

// Frees the three key arrays (only when they were allocated) and empties the track.
void KeyTrack::Free()
{
    if (capacity != 0)
    {
        free(frames);
        frames = 0;
        free(values);
        values = 0;
        free(flags);
        flags = 0;
    }
    capacity = 0;
    count = 0;
}

// Allocates room for 16 keys and empties the track, with a zero default key at frame 0.
void KeyTrack::Init()
{
    capacity = 0;
    Reserve(16);
    count = 0;
    frames[0] = 0;
    float zero = 0;
    values->Fill(zero);
}

// Marks every key as a motion frame key (flag 4).
int KeyTrack::MarkAll()
{
    for (int i = 0; i < count; i++)
        flags[i] = 4;
    return 0;
}

// Returns the highest key frame (0 for an empty track).
int KeyTrack::LastFrame()
{
    int last = 0;
    int n = count;
    for (int i = 0; i < n; i++)
    {
        if (frames[i] > last)
            last = frames[i];
    }
    return last;
}

// 1 when the track has a key exactly at `frame` (the keys are sorted, so the search stops early).
int KeyTrack::HasFrame(int frame)
{
    for (int i = 0; i < count; i++)
    {
        if (frames[i] == frame)
            return 1;
        if (frames[i] > frame)
            return 0;
    }
    return 0;
}

// Sets the translation row to position.x/y/z (position.w is ignored; m[15] is left as it is).
void Matrix4::SetTranslation(Vec4 position)
{
    m[MATRIX_TRANSLATION_X] = position.x;
    m[MATRIX_TRANSLATION_Y] = position.y;
    m[MATRIX_TRANSLATION_Z] = position.z;
}

// Sets the upper-left 3x3 block to identity (translation kept).
void Matrix4::ResetRotation()
{
    m[10] = 1.0f;
    m[5] = 1.0f;
    m[0] = 1.0f;
    m[9] = 0;
    m[8] = 0;
    m[6] = 0;
    m[4] = 0;
    m[2] = 0;
    m[1] = 0;
}

// Sets the identity matrix.
void Matrix4::SetIdentity()
{
    m[12] = m[13] = m[14] = m[8] = m[9] = m[11] = m[4] = m[6] = m[7] = m[1] = m[2] = m[3] = 0.0f;
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

// Copies the transposed rotation part of `source` (the inverse rotation for a pure rotation matrix).
void Matrix4::SetRotationTransposed(const Matrix4 &source)
{
    m[0] = source.m[0];
    m[4] = source.m[1];
    m[8] = source.m[2];
    m[1] = source.m[4];
    m[5] = source.m[5];
    m[9] = source.m[6];
    m[2] = source.m[8];
    m[6] = source.m[9];
    m[10] = source.m[10];
}

// Returns `point` transformed by `matrix`.
Vec4 TransformPoint(const Vec4 &point, const Matrix4 &matrix)
{
    Vec4 result;
    result.Copy(point);
    result.Transform(matrix);
    return result;
}
// Makes this track a copy of `source` (keys, values and flags). Returns 1.
int KeyTrack::CopyFrom(const KeyTrack *source)
{
    Free();
    Init();
    for (int i = 0; i < source->count; i++)
    {
        AddKey(source->frames[i], source->values[i].x, source->values[i].y, source->values[i].z, source->values[i].w);
        flags[i] = source->flags[i];
    }
    return 1;
}

// Copies the bank's keys into `target` (a growable track). Returns 0.
int KeyBank::CopyTo(KeyTrack *target)
{
    target->Free();
    target->Init();
    for (int i = 0; i < count; i++)
    {
        target->AddKey(frames[i], values[i].x, values[i].y, values[i].z, values[i].w);
        target->flags[i] = flags[i];
    }
    return 0;
}
#include "motion_list.h"

// Inserts `value` at `position` (-1 = append) and records `tag`. Returns 0 when the position is past the end or
// the list is full. InsertByName repeats this body with the found entry index as the value.
int MotionList::Insert(int position, int value, int tag)
{
    m_0208 = tag;
    if (count < position)
        return 0;
    if (count > 0x7f)
        return 0;
    if (position == -1)
    {
        items[count] = value;
        count++;
        return 1;
    }
    for (int i = count; i >= position; i--)
        items[i + 1] = items[i];
    items[position] = value;
    count++;
    return 1;
}

// 0x101ad350 (MotionList::FrameAt). The frame shown `time` frames after m_0208 (never below 0). Without flag 1 in
// m_0004 that is just the elapsed time. Otherwise the list plays its items in turn: each item is a named entry whose
// ids..values range is its first and end frame and whose extra bit 1 plays it backwards. Stores the item number in
// *outItem and the entry in *outEntry (when given). Past the end the last item repeats, or with extra bit 2 holds
// its last frame (the backward case tests the extra of entry count - 1 instead of the last item's entry).
int MotionList::FrameAt(int time, int *outItem, int *outEntry)
{
    time -= m_0208;
    if (!(m_0004 & 1))
        return time > 0 ? time : 0;
    int start = 0;
    int length;
    for (int i = 0; i < count; i++)
    {
        length = values[items[i]] - ids[items[i]];
        int end = length + start;
        if (time < end)
        {
            if (!(extras[items[i]] & 1))
            {
                if (outItem != 0)
                    *outItem = i;
                if (outEntry != 0)
                    *outEntry = items[i];
                int frame = ids[items[i]] - start + time;
                return frame > 0 ? frame : 0;
            }
            int back = start + start - time + length;
            if (outItem != 0)
                *outItem = i;
            if (outEntry != 0)
                *outEntry = items[i];
            back += ids[items[i]] - start;
            return back > 0 ? back : 0;
        }
        start = end;
    }
    if (length <= 0)
        return 0;
    if (!(extras[items[count - 1]] & 1))
    {
        if (outItem != 0)
            *outItem = count - 1;
        if (outEntry != 0)
            *outEntry = items[count - 1];
        if (extras[items[count - 1]] & 2)
        {
            int frame = ids[items[count - 1]] + (length - 1) % length;
            return frame > 0 ? frame : 0;
        }
        int frame = ids[items[count - 1]] + (time - start) % length;
        return frame > 0 ? frame : 0;
    }
    if (outItem != 0)
        *outItem = count - 1;
    if (outEntry != 0)
        *outEntry = items[count - 1];
    if (extras[count - 1] & 2)
    {
        int frame = ids[items[count - 1]];
        return frame > 0 ? frame : 0;
    }
    int frame = ids[items[count - 1]] - (time - start) % length + length - 1;
    return frame > 0 ? frame : 0;
}

// 0x101ad690. Empties the motion list (the named entries stay). Returns 1.
int MotionList::Clear()
{
    count = 0;
    return 1;
}

// 0x101ad5c0. Looks up the named entry `name` and inserts its index at `position` (-1 = append), recording `tag`
// (see Insert). Returns 0 when there is no entry of that name or the insert fails. The insert is Insert's body
// written out again, not a call: as an inline call cl 13.00 gives the copied arguments their own register
// priorities and swaps the registers of i and the name pointer (NOTES "Callee-saved register choice").
int MotionList::InsertByName(int position, const char *name, int tag)
{
    int i;
    for (i = 0; i < entryCount; i++)
    {
        if (strcmp(name, names[i]) == 0)
            break;
    }
    if (i == entryCount)
        return 0;
    m_0208 = tag;
    if (count < position)
        return 0;
    if (count > 0x7f)
        return 0;
    if (position == -1)
    {
        items[count] = i;
        count++;
        return 1;
    }
    for (int j = count; j >= position; j--)
        items[j + 1] = items[j];
    items[position] = i;
    count++;
    return 1;
}

// Adds a named entry (`name`, id, value, extra). Returns 0 when all 64 entries are used.
int MotionList::AddEntry(int id, int value, const char *name, int extra)
{
    if (entryCount > 0x3f)
        return 0;
    ids[entryCount] = id;
    values[entryCount] = value;
    strncpy(names[entryCount], name, 0x1f);
    names[entryCount][0x1f] = 0;
    extras[entryCount] = extra;
    entryCount++;
    return 1;
}

// Makes this pack a copy of `source` (header fields and vertices).
int VertexPack::CopyFrom(const VertexPack *source)
{
    vertexCount = 0;
    if (source == 0)
        return 0;
    if (source->vertexCount == 0)
        return 0;
    SetVertexCount(source->vertexCount);
    boundsMin.Copy(source->boundsMin);
    boundsMax.Copy(source->boundsMax);
    center[0] = source->center[0];
    center[1] = source->center[1];
    center[2] = source->center[2];
    radius = source->radius;
    vertexCount = source->vertexCount;
    m_0010 = source->m_0010;
    m_0018 = source->m_0018;
    material = source->material;
    m_0040 = source->m_0040;
    m_0044 = source->m_0044;
    DAT_11b07530(vertices, source->vertices, vertexCount * 0x70);
    return 1;
}
// Same as KeyBank::Insert for a growable track (grows the arrays when needed).
int KeyTrack::Insert(int frame, float x, float y, float z, float w)
{
    int last = count;
    int i;
    for (i = 0; i < count; i++)
    {
        if (frames[i] == frame)
            break;
        if (frames[i] > frame)
            break;
    }
    for (int j = last - 1; j >= i; j--)
    {
        frames[j + 1] = frames[j];
        values[j + 1].Copy(values[j]);
        flags[j + 1] = flags[j];
    }
    if (count + 16 > capacity)
        Reserve(16);
    frames[i] = frame;
    values[i].x = x;
    values[i].y = y;
    values[i].z = z;
    values[i].w = w;
    flags[i] = 0;
    count++;
    if (count + 16 > capacity)
        Reserve(16);
    return i;
}

// Sets the key at `frame` to (x, y, z, w): an existing key at that frame gets the new value (returns 1); otherwise a
// new key is inserted in frame order like Insert (returns its index), growing the arrays when needed.
int KeyTrack::Set(int frame, float x, float y, float z, float w)
{
    int i;
    for (i = 0; i < count; i++)
    {
        if (frames[i] == frame)
        {
            values[i].x = x;
            values[i].y = y;
            values[i].z = z;
            values[i].w = w;
            return 1;
        }
        if (frames[i] > frame)
            break;
    }
    for (int j = count - 1; j >= i; j--)
    {
        frames[j + 1] = frames[j];
        values[j + 1].Copy(values[j]);
        flags[j + 1] = flags[j];
    }
    if (count + 16 > capacity)
        Reserve(16);
    frames[i] = frame;
    values[i].x = x;
    values[i].y = y;
    values[i].z = z;
    values[i].w = w;
    flags[i] = 0;
    count++;
    if (count + 16 > capacity)
        Reserve(16);
    return i;
}

// Reallocates the span buffer for `newCount` 16-byte spans (contents are not kept). The allocation is tagged
// "m_indexSP(count)" for the engine's memory report. Returns 1.
int IndexList::ResizeSpans(int newCount)
{
    char tag[64];
    if (spheres != 0)
        DAT_11b1ba40(spheres);
    spheres = 0;
    sprintf(tag, "m_indexSP(%d)", newCount);
    void *buffer = DAT_11b0b938(newCount * 16, tag);
    count = newCount;
    spheres = (Vec4 *)buffer;
    return 1;
}

// 0x101acfa0. Empties a vertex pack that was allocated without a constructor: no vertices, no count, zero bounds
// fields (the arrays are not freed).
void VertexPack::Init()
{
    vertices = 0;
    center[0] = 0;
    center[1] = 0;
    center[2] = 0;
    radius = 0;
    vertexCount = 0;
    m_0040 = 0;
    m_0044 = 0;
}

// 0x101ad1b0. Empties an index list allocated without a constructor (only the pointer is cleared).
void IndexList::Init()
{
    indices = 0;
}
