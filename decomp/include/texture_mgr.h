// ZeroVec, Transform: declared here once (first used by src/texture_mgr.cpp).
#ifndef GURUMIN_TEXTURE_MGR_H
#define GURUMIN_TEXTURE_MGR_H

#include "node.h"
#include "tri_batch.h"

struct ZeroVec
{
    float x;
    float y;
    float z;
    float w;
    ZeroVec();
};
struct Transform
{
    int m_0000;
    ZeroVec position;
    ZeroVec rotation;
    ZeroVec scale;
    Matrix4 matrix;
    Transform();
    ~Transform();  // 0x10172a30: destroys the matrix member (never called)
};
#endif
