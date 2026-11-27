// Vector and matrix math library (APP.dll 0x101ac000-0x101ae000).
//
// All math is single-precision float on the x87 FPU. The game was compiled with /Op (float consistency), so every
// intermediate result is rounded through memory, sqrt/sin/cos/atan2 are real CRT calls (no inline fsqrt), and float
// arguments of inline helpers are spilled to the stack. Matrices are row-major 4x4 with the translation in m[12..14]
// (Direct3D convention: row vectors multiplied on the left).
#include <math.h>
#include "vec4.h"

#include "vec3.h"

#include "plane.h"

#include "matrix4.h"

// x*x + y*y + z*z. An inline helper in the original source: its three float arguments are evaluated right to left
// and spilled to the stack, which is what gives callers their z, y, x copy order.
inline float LengthSquared(float x, float y, float z)
{
    return x * x + y * y + z * z;
}

// Length of the x/y part only (distance on the ground plane from the origin).
float Vec4::Length2D()
{
    return (float)sqrt(x * x + y * y);
}

// Distance between two points on the x/y plane (z ignored). `other` is passed by value (16 bytes on the stack).
float Vec4::Distance2D(Vec4 other)
{
    float dy = y - other.y;
    float dx = x - other.x;
    return (float)sqrt(dx * dx + dy * dy);
}

// Squared length of x/y/z (w ignored). Cheaper than Length when only comparing distances.
float Vec4::LengthSquared3()
{
    return LengthSquared(x, y, z);
}

// Moves the matrix: adds (x, y, z) to its translation row.
void Matrix4::Translate(float x, float y, float z)
{
    m[12] += x;
    m[13] += y;
    m[14] += z;
}

// Matrix4 constructor. It zeroes every element except m[5], m[9] and m[13] (no identity is set); callers that need
// a defined matrix overwrite it. The chained assignment reproduces the original store order (right to left).
Matrix4::Matrix4()
{
    m[12] = m[1] = m[14] = m[15] = m[8] = m[10] = m[11] = m[4] = m[6] = m[7] = m[0] = m[2] = m[3] = 0;
}

// Squared 3D distance to `other`.
float Vec4::DistanceSquared(const Vec4 &other)
{
    float dx = other.x - x;
    float dy = other.y - y;
    float dz = other.z - z;
    return dz * dz + dy * dy + dx * dx;
}

// 3D distance to `other`.
float Vec4::Distance(const Vec4 &other)
{
    float dx = other.x - x;
    float dy = other.y - y;
    float dz = other.z - z;
    return (float)sqrt(dz * dz + dy * dy + dx * dx);
}

// 3D distance with the z difference counted twice, so points stacked vertically count as farther apart than
// points side by side (used for range checks that should prefer the same height).
float Vec4::DistanceZWeighted(const Vec4 &other)
{
    float dx = other.x - x;
    float dy = other.y - y;
    float dz = (other.z - z) * 2;
    return (float)sqrt(dz * dz + dy * dy + dx * dx);
}

// 1 if the point lies inside the box on the x/y plane (edges included), else 0. Written as an early exit so the
// float compares come out NaN-safe like the original (a NaN coordinate counts as inside).
int Vec4::InRect(const Box2 *box)
{
    if (x < box->min.x || x > box->max.x || y < box->min.y || y > box->max.y)
        return 0;
    return 1;
}

// 1 if the point lies inside the box (x, y and z; edges included), else 0. Same NaN-safe early exit as InRect.
int Vec4::InBox(const Box2 *box)
{
    if (x < box->min.x || x > box->max.x || y < box->min.y || y > box->max.y || z < box->min.z || z > box->max.z)
        return 0;
    return 1;
}

// Length of the x/y/z part of a vector (free function, cdecl; declared in vec4.h).
extern "C" float Vec3_Length(const Vec4 *v)
{
    return (float)sqrt(Vec3_LengthSquared(v));
}

// Sets the upper-left 3x3 block to a rotation of `angle` radians about the z axis (m[3], m[7], m[11] and the
// translation row are left as they are). Statements are in the original's store order.
void Matrix4::SetRotationZ(float angle)
{
    float c = (float)cos(angle);
    float s = (float)sin(angle);
    m[2] = 0;
    m[6] = 0;
    m[1] = -s;
    m[9] = 0;
    m[8] = 0;
    m[0] = c;
    m[5] = c;
    m[10] = 1.0f;
    m[4] = s;
}

// 1 if the point lies inside the box grown by `margin` on every side (x, y and z; edges included), else 0.
int Vec4::InBoxMargin(const Box2 *box, float margin)
{
    if (box->min.x - margin > x || margin + box->max.x < x || box->min.y - margin > y || margin + box->max.y < y ||
        box->min.z - margin > z || margin + box->max.z < z)
        return 0;
    return 1;
}

// Intersects the line origin + t * direction with the plane and stores the point in `out` (x/y/z).
// Returns 0 (and leaves `out` alone) when the line is parallel to the plane, else 1. t may be negative.
int Plane::IntersectRay(Vec4 *out, const Vec4 *origin, const Vec4 *direction)
{
    // The grouping y + (x + z) reproduces the original's evaluation order (the compiler orders the terms itself).
    float denominator = direction->y * b + (direction->x * a + direction->z * c);
    if (denominator == 0.0f)
        return 0;
    float t = -((origin->y * b + (origin->x * a + origin->z * c) + d) / denominator);
    out->x = t * direction->x + origin->x;
    out->y = t * direction->y + origin->y;
    out->z = t * direction->z + origin->z;
    return 1;
}

// Interpolates between `a` and `b` (all four components) by `t` and normalises the result: a normalised lerp,
// used to blend rotations stored as quaternions.
Vec4 Nlerp(const Vec4 &a, const Vec4 &b, float t)
{
    Vec4 result;
    result.x = (b.x - a.x) * t + a.x;
    result.y = (b.y - a.y) * t + a.y;
    result.z = (b.z - a.z) * t + a.z;
    result.w = (b.w - a.w) * t + a.w;
    result.Normalize();
    return result;
}

// 0x101aeae0. Spherical interpolation between the rotation quaternions `a` and `b` by `t`. `b` is flipped when the
// two point into opposite hemispheres, so the shorter way round is taken; when they are nearly equal (cosine of the
// angle 0.95 or more) the cheaper Nlerp is used instead.
Vec4 Slerp(const Vec4 &a, const Vec4 &b, float t)
{
    Vec4 target;
    float cosAngle = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    if (cosAngle < 0.0f)
    {
        cosAngle = -cosAngle;
        target.x = b.x * -1.0f;
        target.y = b.y * -1.0f;
        target.z = b.z * -1.0f;
        target.w = b.w * -1.0f;
    }
    else
    {
        target.Copy(b);
    }
    if (cosAngle < 0.95f)
    {
        float angle = (float)acos(cosAngle);
        float sinAngle = (float)sin(angle);
        float weightB = (float)sin(angle * t);
        float weightA = (float)sin((1.0f - t) * angle);
        Vec4 result;
        result.x = (weightA * a.x + weightB * target.x) / sinAngle;
        result.y = (weightA * a.y + weightB * target.y) / sinAngle;
        result.z = (weightA * a.z + weightB * target.z) / sinAngle;
        result.w = (weightB * target.w + weightA * a.w) / sinAngle;
        return result;
    }
    return Nlerp(a, target, t);
}

// Transforms direction `v` by the rotation part of `matrix` (no translation); the result's w is 0.
Vec4 TransformNormal(const Vec4 &v, const Matrix4 &matrix)
{
    float t[4];
    t[0] = matrix.m[8] * v.z + matrix.m[4] * v.y + v.x * matrix.m[0];
    t[1] = matrix.m[9] * v.z + matrix.m[5] * v.y + matrix.m[1] * v.x;
    t[2] = matrix.m[10] * v.z + matrix.m[6] * v.y + matrix.m[2] * v.x;
    Vec4 result;
    result.x = t[0];
    result.y = t[1];
    result.z = t[2];
    return result;
}

// Rotates the vector's x/y part by `angle` radians about the z axis (z and w unchanged).
void Vec4::RotateZ(float angle)
{
    float cosX = (float)cos(angle) * x;
    float nx = cosX - (float)sin(angle) * y;
    float cosY = (float)cos(angle) * y;
    float ny = cosY + (float)sin(angle) * x;
    x = nx;
    y = ny;
}

// Rotates the vector's x/z part by `angle` radians about the y axis (the same 2D rotation as RotateZ applied to
// (x, -z), so a positive angle turns x towards -z).
void Vec4::RotateY(float angle)
{
    float cosX = (float)cos(angle) * x;
    float nx = cosX - (float)sin(angle) * (-z);
    float cosZ = (float)cos(angle) * (-z);
    float nz = cosZ + (float)sin(angle) * x;
    x = nx;
    z = -nz;
}

// Rotates the vector's y/z part by `angle` radians about the x axis (the 2D rotation applied to (y, -z)).
void Vec4::RotateX(float angle)
{
    float cosY = (float)cos(angle) * y;
    float ny = cosY - (float)sin(angle) * (-z);
    float cosZ = (float)cos(angle) * (-z);
    float nz = cosZ + (float)sin(angle) * y;
    y = ny;
    z = -nz;
}

// Snaps an angle in radians to the nearest multiple of `step` degrees (rounding half away from zero) and returns it
// in radians again. pi is approximated as 3.1415 here, as everywhere in the game.
float SnapAngle(float radians, int step)
{
    float degrees = radians * 180.0f / 3.1415f;
    if (degrees >= 0.0f)
    {
        int snapped = ((int)degrees + step / 2) / step * step;
        return snapped * 3.1415f / 180.0f;
    }
    int snapped = ((int)degrees - step / 2) / step * step;
    return snapped * 3.1415f / 180.0f;
}

// Cross product a x b (x/y/z; the result's w stays 0).
Vec4 Cross(const Vec4 &a, const Vec4 &b)
{
    Vec4 result;
    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;
    return result;
}

// Distance from `point` to the plane (always positive; the plane normal need not be unit length).
// The dot product is written y, x, then the z term in its own parentheses: the compiler orders the products itself and
// this grouping gives the original's order (z product loaded first).
float Plane::Distance(const Vec4 *point)
{
    return (float)fabs((point->y * b + point->x * a + (point->z * c) + d) / sqrt(LengthSquared(a, b, c)));
}

// Tilts `v` by `angle` radians about the horizontal axis at right angles to its heading: turns it to heading 0,
// rotates about x, and turns it back. Used for pitching directions up or down.
Vec4 Pitch(const Vec4 &v, float angle)
{
    float heading = (float)atan2(v.x, v.y);
    Vec4 result;
    result.Copy(v);
    result.RotateZ(heading);
    result.RotateX(angle);
    result.RotateZ(-heading);
    return result;
}

// Length of all four components. An inline helper like LengthSquared: its float arguments are spilled to the stack.
inline float Length4(float x, float y, float z, float w)
{
    return (float)sqrt(x * x + y * y + z * z + w * w);
}

// Scales all four components to unit length; a zero vector is left unchanged.
void Vec4::Normalize()
{
    float length = Length4(x, y, z, w);
    if (length > 0.0f)
    {
        x /= length;
        y /= length;
        z /= length;
        w /= length;
    }
}

// Scales x/y/z to unit length in place and returns the normalised vector (w = 0). A zero vector stays as it is and
// the zero vector is returned.
Vec4 Vec4::Normalized3()
{
    Vec4 result;
    float length = Vec3_Length(this);
    if (length == 0.0f)
    {
        float zero = 0;
        result.Fill(zero);
        return result;
    }
    x = result.x = x / length;
    y = result.y = y / length;
    z = result.z = z / length;
    return result;
}

// Quaternion product a * b (x, y, z = vector part, w = scalar part): the vector part is a x b + a.w * b + b.w * a,
// the scalar part a.w * b.w - a . b.
Vec4 QuatMultiply(const Vec4 &a, const Vec4 &b)
{
    Vec4 result;
    result.x = a.y * b.z - a.z * b.y + a.w * b.x + a.x * b.w;
    result.y = a.z * b.x - a.x * b.z + a.y * b.w + a.w * b.y;
    result.z = a.x * b.y - a.y * b.x + a.z * b.w + a.w * b.z;
    result.w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
    return result;
}

