// EffectModel: one slot of the model table at DAT_10edbe08 (EFFECT_MODEL_SLOTS slots, 0xb08 bytes each): models
// placed by scripts (hit sparks, magic effects) and attached models such as held items. A slot is free while
// node == 0. Set up by EffectModel::M_1000b9f0 (effect_model.cpp), drawn by EffectModel::Draw (mesh_slots.cpp) or,
// with its key frames, by EffectModel::DrawKeyed (called from ai_pick.cpp).
#ifndef GURUMIN_EFFECT_MODEL_H
#define GURUMIN_EFFECT_MODEL_H

#include "key_track.h"
#include "node.h"

#define EFFECT_MODEL_SLOTS 0x40

struct EffectModel
{
    int pad_0000;
    Node *node;         // 0x04: the loaded model (0 = slot free)
    int start;          // 0x08: first frame played; the model is drawn once `frame` reaches it
    int end;            // 0x0c: last frame played
    Vec4 color;         // 0x10
    Vec4 rotation;      // 0x20: rotation about x, y and z in degrees
    int visible;        // 0x30: 1 = drawn
    int frame;          // 0x34: frame counter (starts at -start for a negative start)
    int m_0038;         // 0x38: the `value` argument of the setup
    int ownCamera;      // 0x3c: non-zero = drawn with its own fixed camera (see Draw)
    Matrix4 matrix;     // 0x40: placement, copied from the world matrix DAT_10db9210 at setup
    KeyBank keys;       // 0x80
    int changed;        // 0xb04: 1 after a key or the rotation was changed, reset to 0 at setup
    EffectModel();
    ~EffectModel();  // 0x1000bea0: only the matrix member has a destructor (never called, nothing destroys an EffectModel)
    int M_1000b9f0(const char *path, int value, int start, int end);
    int Draw(const Matrix4 &view);                                // 0x1000baf0
    int Free();                                                   // 0x1000be10: releases the model, the slot is free again
    int DrawKeyed(Matrix4 &view, Matrix4 &camera);                // 0x10037b40: draws with the key frames applied

    // Adds a position key and marks the effect as changed.
    void AddKey(int frame, float x, float y, float z)
    {
        changed = 1;
        keys.Insert(frame, x, y, z, 0);
    }

    // Sets the rotation (degrees) and marks the effect as changed.
    void SetRotation(float x, float y, float z)
    {
        changed = 1;
        rotation.Set(x, y, z, 0);
    }
};

extern "C" EffectModel DAT_10edbe08[EFFECT_MODEL_SLOTS];  // the model slots

#endif
